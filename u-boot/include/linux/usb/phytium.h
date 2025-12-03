/* SPDX-License-Identifier: GPL-2.0+ */
/* include/linux/usb/phytium.h
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 * Phytium SuperSpeed USB 3.0 DRD Controller global and OTG registers
 */

#ifndef __PHYTIUM_H_
#define __PHYTIUM_H_

/* Global constants */
#define FT_ENDPOINTS_NUM			32

#define FT_EVENT_BUFFERS_SIZE			PAGE_SIZE
#define FT_EVENT_TYPE_MASK			0xfe

#define FT_EVENT_TYPE_DEV			0
#define FT_EVENT_TYPE_CARKIT			3
#define FT_EVENT_TYPE_I2C			4

#define FT_DEVICE_EVENT_DISCONNECT		0
#define FT_DEVICE_EVENT_RESET			1
#define FT_DEVICE_EVENT_CONNECT_DONE		2
#define FT_DEVICE_EVENT_LINK_STATUS_CHANGE	3
#define FT_DEVICE_EVENT_WAKEUP		4
#define FT_DEVICE_EVENT_EOPF			6
#define FT_DEVICE_EVENT_SOF			7
#define FT_DEVICE_EVENT_ERRATIC_ERROR		9
#define FT_DEVICE_EVENT_CMD_CMPL		10
#define FT_DEVICE_EVENT_OVERFLOW		11

#define FT_GEVNTCOUNT_MASK			0xfffc
#define FT_GSNPSID_MASK			0xffff0000
#define FT_GSNPSID_SHIFT			16
#define FT_GSNPSREV_MASK			0xffff

#define FT_REVISION_MASK			0xffff

#define FT_REG_OFFSET				0xC100

struct g_event_buffer {
	u32 g_evntadrlo;
	u32 g_evntadrhi;
	u32 g_evntsiz;
	u32 g_evntcount;
};

struct d_physical_endpoint {
	u32 d_depcmdpar2;
	u32 d_depcmdpar1;
	u32 d_depcmdpar0;
	u32 d_depcmd;
};

struct ft {					/* offset: 0xC100 */
	u32 g_sbuscfg0;
	u32 g_sbuscfg1;
	u32 g_txthrcfg;
	u32 g_rxthrcfg;
	u32 g_ctl;

	u32 reserved1;

	u32 g_sts;

	u32 reserved2;

	u32 g_snpsid;
	u32 g_gpio;
	u32 g_uid;
	u32 g_uctl;
	u64 g_buserraddr;
	u64 g_prtbimap;

	u32 g_hwparams0;
	u32 g_hwparams1;
	u32 g_hwparams2;
	u32 g_hwparams3;
	u32 g_hwparams4;
	u32 g_hwparams5;
	u32 g_hwparams6;
	u32 g_hwparams7;

	u32 g_dbgfifospace;
	u32 g_dbgltssm;
	u32 g_dbglnmcc;
	u32 g_dbgbmu;
	u32 g_dbglspmux;
	u32 g_dbglsp;
	u32 g_dbgepinfo0;
	u32 g_dbgepinfo1;

	u64 g_prtbimap_hs;
	u64 g_prtbimap_fs;

	u32 reserved3[28];

	u32 g_usb2phycfg[16];
	u32 g_usb2i2cctl[16];
	u32 g_usb2phyacc[16];
	u32 g_usb3pipectl[16];

	u32 g_txfifosiz[32];
	u32 g_rxfifosiz[32];

	struct g_event_buffer g_evnt_buf[32];

	u32 g_hwparams8;

	u32 reserved4[11];

	u32 g_fladj;

	u32 reserved5[51];

	u32 d_cfg;
	u32 d_ctl;
	u32 d_evten;
	u32 d_sts;
	u32 d_gcmdpar;
	u32 d_gcmd;

	u32 reserved6[2];

	u32 d_alepena;

	u32 reserved7[55];

	struct d_physical_endpoint d_phy_ep_cmd[32];

	u32 reserved8[128];

	u32 o_cfg;
	u32 o_ctl;
	u32 o_evt;
	u32 o_evten;
	u32 o_sts;

	u32 reserved9[3];

	u32 adp_cfg;
	u32 adp_ctl;
	u32 adp_evt;
	u32 adp_evten;

	u32 bc_cfg;

	u32 reserved10;

	u32 bc_evt;
	u32 bc_evten;
};

/* Global Configuration Register */
#define FT_GCTL_PWRDNSCALE(n)			((n) << 19)
#define FT_GCTL_U2RSTECN			(1 << 16)
#define FT_GCTL_RAMCLKSEL(x)			\
		(((x) & FT_GCTL_CLK_MASK) << 6)
#define FT_GCTL_CLK_BUS			(0)
#define FT_GCTL_CLK_PIPE			(1)
#define FT_GCTL_CLK_PIPEHALF			(2)
#define FT_GCTL_CLK_MASK			(3)
#define FT_GCTL_PRTCAP(n)			(((n) & (3 << 12)) >> 12)
#define FT_GCTL_PRTCAPDIR(n)			((n) << 12)
#define FT_GCTL_PRTCAP_HOST			1
#define FT_GCTL_PRTCAP_DEVICE			2
#define FT_GCTL_PRTCAP_OTG			3
#define FT_GCTL_CORESOFTRESET			(1 << 11)
#define FT_GCTL_SCALEDOWN(n)			((n) << 4)
#define FT_GCTL_SCALEDOWN_MASK		FT_GCTL_SCALEDOWN(3)
#define FT_GCTL_DISSCRAMBLE			(1 << 3)
#define FT_GCTL_DSBLCLKGTNG			(1 << 0)

/* Global HWPARAMS1 Register */
#define FT_GHWPARAMS1_EN_PWROPT(n)		(((n) & (3 << 24)) >> 24)
#define FT_GHWPARAMS1_EN_PWROPT_NO		0
#define FT_GHWPARAMS1_EN_PWROPT_CLK		1

/* Global USB2 PHY Configuration Register */
#define FT_GUSB2PHYCFG_PHYSOFTRST		(1 << 31)
#define FT_GUSB2PHYCFG_U2_FREECLK_EXISTS	(1 << 30)
#define FT_GUSB2PHYCFG_ENBLSLPM		(1 << 8)
#define FT_GUSB2PHYCFG_SUSPHY			(1 << 6)
#define FT_GUSB2PHYCFG_PHYIF			(1 << 3)

/* Global USB2 PHY Configuration Mask */
#define FT_GUSB2PHYCFG_USBTRDTIM_MASK		(0xf << 10)

/* Global USB2 PHY Configuration Offset */
#define FT_GUSB2PHYCFG_USBTRDTIM_OFFSET	10

#define FT_GUSB2PHYCFG_USBTRDTIM_16BIT (0x5 << \
		FT_GUSB2PHYCFG_USBTRDTIM_OFFSET)
#define FT_GUSB2PHYCFG_USBTRDTIM_8BIT (0x9 << \
		FT_GUSB2PHYCFG_USBTRDTIM_OFFSET)

/* Global USB3 PIPE Control Register */
#define FT_GUSB3PIPECTL_PHYSOFTRST		(1 << 31)
#define FT_GUSB3PIPECTL_DISRXDETP3		(1 << 28)
#define FT_GUSB3PIPECTL_SUSPHY		(1 << 17)

/* Global TX Fifo Size Register */
#define FT_GTXFIFOSIZ_TXFDEF(n)		((n) & 0xffff)
#define FT_GTXFIFOSIZ_TXFSTADDR(n)		((n) & 0xffff0000)

/* Device Control Register */
#define FT_DCTL_RUN_STOP			(1 << 31)
#define FT_DCTL_CSFTRST			(1 << 30)
#define FT_DCTL_LSFTRST			(1 << 29)

/* Global Frame Length Adjustment Register */
#define GFLADJ_30MHZ_REG_SEL			(1 << 7)
#define GFLADJ_30MHZ(n)				((n) & 0x3f)
#define GFLADJ_30MHZ_DEFAULT			0x20

#ifdef CONFIG_USB_XHCI_FT
void ft_set_mode(struct ft *ft_reg, u32 mode);
void ft_core_soft_reset(struct ft *ft_reg);
int ft_core_init(struct ft *ft_reg);
void ft_set_fladj(struct ft *ft_reg, u32 val);
#endif
#endif /* __PHYTIUM_H_ */
