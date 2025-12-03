// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium xHCI USB platform driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <generic-phy.h>
#include <log.h>
#include <reset.h>
#include <usb.h>
#include <phytium-uboot.h>
#include <linux/delay.h>

#include <usb/xhci.h>
#include <asm/io.h>
#include <linux/usb/phytium.h>

struct xhci_ft_plat {
	struct clk_bulk clks;
	struct phy_bulk phys;
	struct reset_ctl_bulk resets;
};

void ft_set_mode(struct ft *ft_reg, u32 mode)
{
	clrsetbits_le32(&ft_reg->g_ctl,
			FT_GCTL_PRTCAPDIR(FT_GCTL_PRTCAP_OTG),
			FT_GCTL_PRTCAPDIR(mode));
}

static void ft_phy_reset(struct ft *ft_reg)
{
	/* Assert USB3 PHY reset */
	setbits_le32(&ft_reg->g_usb3pipectl[0], FT_GUSB3PIPECTL_PHYSOFTRST);

	/* Assert USB2 PHY reset */
	setbits_le32(&ft_reg->g_usb2phycfg, FT_GUSB2PHYCFG_PHYSOFTRST);

	mdelay(100);

	/* Clear USB3 PHY reset */
	clrbits_le32(&ft_reg->g_usb3pipectl[0], FT_GUSB3PIPECTL_PHYSOFTRST);

	/* Clear USB2 PHY reset */
	clrbits_le32(&ft_reg->g_usb2phycfg, FT_GUSB2PHYCFG_PHYSOFTRST);
}

void ft_core_soft_reset(struct ft *ft_reg)
{
	/* Before Resetting PHY, put Core in Reset */
	setbits_le32(&ft_reg->g_ctl, FT_GCTL_CORESOFTRESET);

	/* reset USB3 phy - if required */
	ft_phy_reset(ft_reg);

	mdelay(100);

	/* After PHYs are stable we can take Core out of reset state */
	clrbits_le32(&ft_reg->g_ctl, FT_GCTL_CORESOFTRESET);
}

int ft_core_init(struct ft *ft_reg)
{
	u32 reg;
	u32 revision;
	unsigned int ft_hwparams1;

	revision = readl(&ft_reg->g_snpsid);

	ft_core_soft_reset(ft_reg);

	ft_hwparams1 = readl(&ft_reg->g_hwparams1);

	reg = readl(&ft_reg->g_ctl);
	reg &= ~FT_GCTL_SCALEDOWN_MASK;
	reg &= ~FT_GCTL_DISSCRAMBLE;
	switch (FT_GHWPARAMS1_EN_PWROPT(ft_hwparams1)) {
	case FT_GHWPARAMS1_EN_PWROPT_CLK:
		reg &= ~FT_GCTL_DSBLCLKGTNG;
		break;
	default:
		debug("No power optimization available\n");
	}

	/*
	 * WORKAROUND: FT revisions <1.90a have a bug
	 * where the device can fail to connect at SuperSpeed
	 * and falls back to high-speed mode which causes
	 * the device to enter a Connect/Disconnect loop
	 */
	if ((revision & FT_REVISION_MASK) < 0x190a)
		reg |= FT_GCTL_U2RSTECN;

	writel(reg, &ft_reg->g_ctl);

	return 0;
}

void ft_set_fladj(struct ft *ft_reg, u32 val)
{
	setbits_le32(&ft_reg->g_fladj, GFLADJ_30MHZ_REG_SEL |
			GFLADJ_30MHZ(val));
}

#if CONFIG_IS_ENABLED(DM_USB)
static int xhci_ft_reset_init(struct udevice *dev,
				struct xhci_ft_plat *plat)
{
	int ret;

	ret = reset_get_bulk(dev, &plat->resets);
	if (ret == -ENOTSUPP || ret == -ENOENT)
		return 0;
	else if (ret)
		return ret;

	ret = reset_deassert_bulk(&plat->resets);
	if (ret) {
		reset_release_bulk(&plat->resets);
		return ret;
	}

	return 0;
}

static int xhci_ft_clk_init(struct udevice *dev,
			      struct xhci_ft_plat *plat)
{
	int ret;

	ret = clk_get_bulk(dev, &plat->clks);
	if (ret == -ENOSYS || ret == -ENOENT)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&plat->clks);
	if (ret) {
		clk_release_bulk(&plat->clks);
		return ret;
	}

	return 0;
}

static int xhci_ft_probe(struct udevice *dev)
{
	struct xhci_hcor *hcor;
	struct xhci_hccr *hccr;
	struct ft *ft_reg;
	enum usb_dr_mode dr_mode;
	struct xhci_ft_plat *plat = dev_get_plat(dev);
	const char *phy;
	u32 reg;
	int ret;

	ret = xhci_ft_reset_init(dev, plat);
	if (ret)
		return ret;

	ret = xhci_ft_clk_init(dev, plat);
	if (ret)
		return ret;

	hccr = (struct xhci_hccr *)((uintptr_t)dev_remap_addr(dev));
	hcor = (struct xhci_hcor *)((uintptr_t)hccr +
			HC_LENGTH(xhci_readl(&(hccr)->cr_capbase)));

	ret = ft_setup_phy(dev, &plat->phys);
	if (ret && (ret != -ENOTSUPP))
		return ret;

	ft_reg = (struct ft *)((char *)(hccr));
    ft_core_init(ft_reg);

	/* Set ft usb2 phy config */
	reg = readl(&ft_reg->g_usb2phycfg[0]);

	phy = dev_read_string(dev, "phy_type");
	if (phy && strcmp(phy, "utmi_wide") == 0) {
		reg |= FT_GUSB2PHYCFG_PHYIF;
		reg &= ~FT_GUSB2PHYCFG_USBTRDTIM_MASK;
		reg |= FT_GUSB2PHYCFG_USBTRDTIM_16BIT;
	}

	writel(reg, &ft_reg->g_usb2phycfg[0]);

	dr_mode = usb_get_dr_mode(dev_ofnode(dev));
	if (dr_mode == USB_DR_MODE_UNKNOWN)
		/* by default set dual role mode to HOST */
		dr_mode = USB_DR_MODE_HOST;

	ft_set_mode(ft_reg, dr_mode);

	return xhci_register(dev, hccr, hcor);
}

static int xhci_ft_remove(struct udevice *dev)
{
	struct xhci_ft_plat *plat = dev_get_plat(dev);

	ft_shutdown_phy(dev, &plat->phys);

	clk_release_bulk(&plat->clks);

	reset_release_bulk(&plat->resets);

	return xhci_deregister(dev);
}

static const struct udevice_id xhci_ft_ids[] = {
	{ .compatible = "phytium,phytium-xhci" },
	{ }
};

U_BOOT_DRIVER(xhci_ft) = {
	.name = "xhci-phytium",
	.id = UCLASS_USB,
	.of_match = xhci_ft_ids,
	.probe = xhci_ft_probe,
	.remove = xhci_ft_remove,
	.ops = &xhci_usb_ops,
	.priv_auto	= sizeof(struct xhci_ctrl),
	.plat_auto	= sizeof(struct xhci_ft_plat),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
#endif
