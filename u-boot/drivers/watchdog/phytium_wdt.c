// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium WDT controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#include <asm/global_data.h>
#include <asm/io.h>
#include <common.h>
#include <dm/device.h>
#include <dm/fdtaddr.h>
#include <dm/read.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <watchdog.h>
#include <wdt.h>

DECLARE_GLOBAL_DATA_PTR;

/* Phytium Generic Watchdog register definitions */
/* control frame */
#define FT_GWDT_WCS		0x1000
#define FT_GWDT_WOR		0x1008
#define FT_GWDT_WCV		0x1010

/* Watchdog Control and Status Register */
#define FT_GWDT_WCS_EN	BIT(0)
#define FT_GWDT_WCS_WS0	BIT(1)
#define FT_GWDT_WCS_WS1	BIT(2)

struct ft_gwdt_priv {
	void __iomem *reg_refresh;
	void __iomem *reg_control;
};

static int ft_gwdt_reset(struct udevice *dev)
{
	struct ft_gwdt_priv *priv = dev_get_priv(dev);
	writel(0, priv->reg_control + FT_GWDT_WOR);

    return 0;
}

static int ft_gwdt_start(struct udevice *dev, u64 timeout, ulong flags)
{
	struct ft_gwdt_priv *priv = dev_get_priv(dev);
	u32 clk;

	/*
	 * it work in the single stage mode in u-boot,
	 * The first signal (WS0) is ignored,
	 * the timeout is (WOR * 2), so the WOR should be configured
	 * to half value of timeout.
	 */
	clk = get_tbclk();
	writel(clk / (2 * 1000) * timeout, priv->reg_control + FT_GWDT_WOR);

	/* writing WCS will cause an explicit watchdog refresh */
	writel(FT_GWDT_WCS_EN, priv->reg_control + FT_GWDT_WCS);

	return 0;
}

static int ft_gwdt_stop(struct udevice *dev)
{
	struct ft_gwdt_priv *priv = dev_get_priv(dev);

	writel(0, priv->reg_control + FT_GWDT_WCS);

	return 0;
}

static int ft_gwdt_expire_now(struct udevice *dev, ulong flags)
{
	ft_gwdt_start(dev, 0, flags);

	return 0;
}

static int ft_gwdt_probe(struct udevice *dev)
{
	debug("%s: Probing wdt%u (ft-gwdt)\n", __func__, dev_seq(dev));

	return 0;
}

static int ft_gwdt_of_to_plat(struct udevice *dev)
{
	struct ft_gwdt_priv *priv = dev_get_priv(dev);

	priv->reg_control = (void __iomem *)dev_read_addr_index(dev, 0);
	if (IS_ERR(priv->reg_control))
		return PTR_ERR(priv->reg_control);

	priv->reg_refresh = (void __iomem *)dev_read_addr_index(dev, 1);
	if (IS_ERR(priv->reg_refresh))
		return PTR_ERR(priv->reg_refresh);

	return 0;
}

static const struct wdt_ops ft_gwdt_ops = {
	.start = ft_gwdt_start,
	.reset = ft_gwdt_reset,
	.stop = ft_gwdt_stop,
	.expire_now = ft_gwdt_expire_now,
};

static const struct udevice_id ft_gwdt_ids[] = {
	{ .compatible = "phytium,phytium-wdt" },
	{}
};

U_BOOT_DRIVER(ft_gwdt) = {
	.name = "phytium_wdt",
	.id = UCLASS_WDT,
	.of_match = ft_gwdt_ids,
	.probe = ft_gwdt_probe,
	.priv_auto	= sizeof(struct ft_gwdt_priv),
	.of_to_plat = ft_gwdt_of_to_plat,
	.ops = &ft_gwdt_ops,
};
