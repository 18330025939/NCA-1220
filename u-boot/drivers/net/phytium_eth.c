// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium ETH ethernet driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */

#include <common.h>
#include <clk.h>
#include <cpu_func.h>
#include <dm.h>
#include <log.h>
#include <asm/global_data.h>
#include <linux/delay.h>

/*
 * The u-boot networking stack is a little weird.  It seems like the
 * networking core allocates receive buffers up front without any
 * regard to the hardware that's supposed to actually receive those
 * packets.
 *
 * The FT receives packets into 128-byte receive buffers, so the
 * buffers allocated by the core isn't very practical to use.  We'll
 * allocate our own, but we need one such buffer in case a packet
 * wraps around the DMA ring so that we have to copy it.
 *
 * Therefore, define CONFIG_SYS_RX_ETH_BUFFER to 1 in the board-specific
 * configuration header.  This way, the core allocates one RX buffer
 * and one TX buffer, each of which can hold a ethernet packet of
 * maximum size.
 *
 * For some reason, the networking core unconditionally specifies a
 * 32-byte packet "alignment" (which really should be called
 * "padding").  FT shouldn't need that, but we'll refrain from any
 * core modifications here...
 */

#include <net.h>
#ifndef CONFIG_DM_ETH
#include <netdev.h>
#endif
#include <malloc.h>
#include <miiphy.h>

#include <linux/mii.h>
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>

#include "phytium_eth.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * These buffer sizes must be power of 2 and divisible
 * by RX_BUFFER_MULTIPLE
 */
#define FT_RX_BUFFER_SIZE		128
#define FT_ETH_RX_BUFFER_SIZE	2048
#define RX_BUFFER_MULTIPLE		64

#define FT_RX_RING_SIZE			32
#define FT_TX_RING_SIZE			16

#define FT_TX_TIMEOUT			1000
#define FT_AUTONEG_TIMEOUT		5000000

struct ft_dma_desc {
	u32	addr;
	u32	ctrl;
};

struct ft_dma_desc_64 {
	u32 addrh;
	u32 unused;
};

#define HW_DMA_CAP_32B		0
#define HW_DMA_CAP_64B		1

#define DMA_DESC_SIZE		16
#define DMA_DESC_BYTES(n)	((n) * DMA_DESC_SIZE)
#define FT_TX_DMA_DESC_SIZE	(DMA_DESC_BYTES(FT_TX_RING_SIZE))
#define FT_RX_DMA_DESC_SIZE	(DMA_DESC_BYTES(FT_RX_RING_SIZE))
#define FT_TX_DUMMY_DMA_DESC_SIZE	(DMA_DESC_BYTES(1))

#define RXBUF_FRMLEN_MASK	0x00000fff
#define TXBUF_FRMLEN_MASK	0x000007ff

struct ft_device {
	void			*regs;

	bool			is_big_endian;

	const struct ft_config *config;

	unsigned int		rx_tail;
	unsigned int		tx_head;
	unsigned int		tx_tail;
	unsigned int		next_rx_tail;
	bool			wrapped;

	void			*rx_buffer;
	void			*tx_buffer;
	struct ft_dma_desc	*rx_ring;
	struct ft_dma_desc	*tx_ring;
	size_t			rx_buffer_size;

	unsigned long		rx_buffer_dma;
	unsigned long		rx_ring_dma;
	unsigned long		tx_ring_dma;

	struct ft_dma_desc	*dummy_desc;
	unsigned long		dummy_desc_dma;

	const struct device	*dev;
#ifndef CONFIG_DM_ETH
	struct eth_device	netdev;
#endif
	unsigned short		phy_addr;
	struct mii_dev		*bus;
#ifdef CONFIG_PHYLIB
	struct phy_device	*phydev;
#endif

#ifdef CONFIG_DM_ETH
#ifdef CONFIG_CLK
	unsigned long		pclk_rate;
#endif
	unsigned long		pclk;
	unsigned int		autoneg;
	phy_interface_t		phy_interface;
#endif
};

struct ft_usrio_cfg {
	unsigned int		mii;
	unsigned int		rmii;
	unsigned int		rgmii;
	unsigned int		clken;
};

struct ft_config {
	unsigned int		dma_burst_length;
	unsigned int		hw_dma_cap;
	unsigned int		caps;

	int			(*clk_init)(struct udevice *dev, ulong rate);
	const struct ft_usrio_cfg	*usrio;
};

#ifndef CONFIG_DM_ETH
#define to_ft(_nd) container_of(_nd, struct ft_device, netdev)
#endif

static int ft_is_eth(struct ft_device *ft)
{
	return FT_BFEXT(IDNUM, ft_readl(ft, MID)) >= 0x2;
}

#ifndef cpu_is_sama5d2
#define cpu_is_sama5d2() 0
#endif

#ifndef cpu_is_sama5d4
#define cpu_is_sama5d4() 0
#endif

static int ft_eth_is_gigabit_capable(struct ft_device *ft)
{
	/*
	 * The FT_ETH controllers embedded in SAMA5D2 and SAMA5D4 are
	 * configured to support only 10/100.
	 */
	return ft_is_eth(ft) && !cpu_is_sama5d2() && !cpu_is_sama5d4();
}

static void ft_mdio_write(struct ft_device *ft, u8 phy_adr, u8 reg,
			    u16 value)
{
	unsigned long netctl;
	unsigned long netstat;
	unsigned long frame;

	netctl = ft_readl(ft, NCR);
	netctl |= FT_BIT(MPE);
	ft_writel(ft, NCR, netctl);

	frame = (FT_BF(SOF, 1)
		 | FT_BF(RW, 1)
		 | FT_BF(PHYA, phy_adr)
		 | FT_BF(REGA, reg)
		 | FT_BF(CODE, 2)
		 | FT_BF(DATA, value));
	ft_writel(ft, MAN, frame);

	do {
		netstat = ft_readl(ft, NSR);
	} while (!(netstat & FT_BIT(IDLE)));

	netctl = ft_readl(ft, NCR);
	netctl &= ~FT_BIT(MPE);
	ft_writel(ft, NCR, netctl);
}

static u16 ft_mdio_read(struct ft_device *ft, u8 phy_adr, u8 reg)
{
	unsigned long netctl;
	unsigned long netstat;
	unsigned long frame;

	netctl = ft_readl(ft, NCR);
	netctl |= FT_BIT(MPE);
	ft_writel(ft, NCR, netctl);

	frame = (FT_BF(SOF, 1)
		 | FT_BF(RW, 2)
		 | FT_BF(PHYA, phy_adr)
		 | FT_BF(REGA, reg)
		 | FT_BF(CODE, 2));
	ft_writel(ft, MAN, frame);

	do {
		netstat = ft_readl(ft, NSR);
	} while (!(netstat & FT_BIT(IDLE)));

	frame = ft_readl(ft, MAN);

	netctl = ft_readl(ft, NCR);
	netctl &= ~FT_BIT(MPE);
	ft_writel(ft, NCR, netctl);

	return FT_BFEXT(DATA, frame);
}

void __weak arch_get_mdio_control(const char *name)
{
	return;
}

#if defined(CONFIG_CMD_MII) || defined(CONFIG_PHYLIB)

int ft_miiphy_read(struct mii_dev *bus, int phy_adr, int devad, int reg)
{
	u16 value = 0;
#ifdef CONFIG_DM_ETH
	struct udevice *dev = eth_get_dev_by_name(bus->name);
	struct ft_device *ft = dev_get_priv(dev);
#else
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	struct ft_device *ft = to_ft(dev);
#endif

	arch_get_mdio_control(bus->name);
	value = ft_mdio_read(ft, phy_adr, reg);

	return value;
}

int ft_miiphy_write(struct mii_dev *bus, int phy_adr, int devad, int reg,
		      u16 value)
{
#ifdef CONFIG_DM_ETH
	struct udevice *dev = eth_get_dev_by_name(bus->name);
	struct ft_device *ft = dev_get_priv(dev);
#else
	struct eth_device *dev = eth_get_dev_by_name(bus->name);
	struct ft_device *ft = to_ft(dev);
#endif

	arch_get_mdio_control(bus->name);
	ft_mdio_write(ft, phy_adr, reg, value);

	return 0;
}
#endif

#define RX	1
#define TX	0
static inline void ft_invalidate_ring_desc(struct ft_device *ft, bool rx)
{
	if (rx)
		invalidate_dcache_range(ft->rx_ring_dma,
			ALIGN(ft->rx_ring_dma + FT_RX_DMA_DESC_SIZE,
			      PKTALIGN));
	else
		invalidate_dcache_range(ft->tx_ring_dma,
			ALIGN(ft->tx_ring_dma + FT_TX_DMA_DESC_SIZE,
			      PKTALIGN));
}

static inline void ft_flush_ring_desc(struct ft_device *ft, bool rx)
{
	if (rx)
		flush_dcache_range(ft->rx_ring_dma, ft->rx_ring_dma +
				   ALIGN(FT_RX_DMA_DESC_SIZE, PKTALIGN));
	else
		flush_dcache_range(ft->tx_ring_dma, ft->tx_ring_dma +
				   ALIGN(FT_TX_DMA_DESC_SIZE, PKTALIGN));
}

static inline void ft_flush_rx_buffer(struct ft_device *ft)
{
	flush_dcache_range(ft->rx_buffer_dma, ft->rx_buffer_dma +
			   ALIGN(ft->rx_buffer_size * FT_RX_RING_SIZE,
				 PKTALIGN));
}

static inline void ft_invalidate_rx_buffer(struct ft_device *ft)
{
	invalidate_dcache_range(ft->rx_buffer_dma, ft->rx_buffer_dma +
				ALIGN(ft->rx_buffer_size * FT_RX_RING_SIZE,
				      PKTALIGN));
}

#if defined(CONFIG_CMD_NET)

static struct ft_dma_desc_64 *ft_64b_desc(struct ft_dma_desc *desc)
{
	return (struct ft_dma_desc_64 *)((void *)desc
		+ sizeof(struct ft_dma_desc));
}

static void ft_set_addr(struct ft_device *ft, struct ft_dma_desc *desc,
			  ulong addr)
{
	struct ft_dma_desc_64 *desc_64;

	if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
		desc_64 = ft_64b_desc(desc);
		desc_64->addrh = upper_32_bits(addr);
	}
	desc->addr = lower_32_bits(addr);
}

static int _ft_send(struct ft_device *ft, const char *name, void *packet,
		      int length)
{
	unsigned long paddr, ctrl;
	unsigned int tx_head = ft->tx_head;
	int i;

	paddr = dma_map_single(packet, length, DMA_TO_DEVICE);

	ctrl = length & TXBUF_FRMLEN_MASK;
	ctrl |= FT_BIT(TX_LAST);
	if (tx_head == (FT_TX_RING_SIZE - 1)) {
		ctrl |= FT_BIT(TX_WRAP);
		ft->tx_head = 0;
	} else {
		ft->tx_head++;
	}

	if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
		tx_head = tx_head * 2;

	ft->tx_ring[tx_head].ctrl = ctrl;
	ft_set_addr(ft, &ft->tx_ring[tx_head], paddr);

	barrier();
	ft_flush_ring_desc(ft, TX);
	ft_writel(ft, NCR, FT_BIT(TE) | FT_BIT(RE) | FT_BIT(TSTART));

	/*
	 * I guess this is necessary because the networking core may
	 * re-use the transmit buffer as soon as we return...
	 */
	for (i = 0; i <= FT_TX_TIMEOUT; i++) {
		barrier();
		ft_invalidate_ring_desc(ft, TX);
		ctrl = ft->tx_ring[tx_head].ctrl;
		if (ctrl & FT_BIT(TX_USED))
			break;
		udelay(1);
	}

	dma_unmap_single(paddr, length, DMA_TO_DEVICE);

	if (i <= FT_TX_TIMEOUT) {
		if (ctrl & FT_BIT(TX_UNDERRUN))
			printf("%s: TX underrun\n", name);
		if (ctrl & FT_BIT(TX_BUF_EXHAUSTED))
			printf("%s: TX buffers exhausted in mid frame\n", name);
	} else {
		printf("%s: TX timeout\n", name);
	}

	/* No one cares anyway */
	return 0;
}

static void reclaim_rx_buffers(struct ft_device *ft,
			       unsigned int new_tail)
{
	unsigned int i;
	unsigned int count;

	i = ft->rx_tail;

	ft_invalidate_ring_desc(ft, RX);
	while (i > new_tail) {
		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
			count = i * 2;
		else
			count = i;
		ft->rx_ring[count].addr &= ~FT_BIT(RX_USED);
		i++;
		if (i > FT_RX_RING_SIZE)
			i = 0;
	}

	while (i < new_tail) {
		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
			count = i * 2;
		else
			count = i;
		ft->rx_ring[count].addr &= ~FT_BIT(RX_USED);
		i++;
	}

	barrier();
	ft_flush_ring_desc(ft, RX);
	ft->rx_tail = new_tail;
}

static int _ft_recv(struct ft_device *ft, uchar **packetp)
{
	unsigned int next_rx_tail = ft->next_rx_tail;
	void *buffer;
	int length;
	u32 status;
	u8 flag = false;

	ft->wrapped = false;
	for (;;) {
		ft_invalidate_ring_desc(ft, RX);

		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
			next_rx_tail = next_rx_tail * 2;

		if (!(ft->rx_ring[next_rx_tail].addr & FT_BIT(RX_USED)))
			return -EAGAIN;

		status = ft->rx_ring[next_rx_tail].ctrl;
		if (status & FT_BIT(RX_SOF)) {
			if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
				next_rx_tail = next_rx_tail / 2;
				flag = true;
			}

			if (next_rx_tail != ft->rx_tail)
				reclaim_rx_buffers(ft, next_rx_tail);
			ft->wrapped = false;
		}

		if (status & FT_BIT(RX_EOF)) {
			buffer = ft->rx_buffer +
				ft->rx_buffer_size * ft->rx_tail;
			length = status & RXBUF_FRMLEN_MASK;

			ft_invalidate_rx_buffer(ft);
			if (ft->wrapped) {
				unsigned int headlen, taillen;

				headlen = ft->rx_buffer_size *
					(FT_RX_RING_SIZE - ft->rx_tail);
				taillen = length - headlen;
				memcpy((void *)net_rx_packets[0],
				       buffer, headlen);
				memcpy((void *)net_rx_packets[0] + headlen,
				       ft->rx_buffer, taillen);
				*packetp = (void *)net_rx_packets[0];
			} else {
				*packetp = buffer;
			}

			if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
				if (!flag)
					next_rx_tail = next_rx_tail / 2;
			}

			if (++next_rx_tail >= FT_RX_RING_SIZE)
				next_rx_tail = 0;
			ft->next_rx_tail = next_rx_tail;
			return length;
		} else {
			if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
				if (!flag)
					next_rx_tail = next_rx_tail / 2;
				flag = false;
			}

			if (++next_rx_tail >= FT_RX_RING_SIZE) {
				ft->wrapped = true;
				next_rx_tail = 0;
			}
		}
		barrier();
	}
}

static void ft_phy_reset(struct ft_device *ft, const char *name)
{
	int i;
	u16 status, adv;

	adv = ADVERTISE_CSMA | ADVERTISE_ALL;
	ft_mdio_write(ft, ft->phy_addr, MII_ADVERTISE, adv);
	printf("%s: Starting autonegotiation...\n", name);
	ft_mdio_write(ft, ft->phy_addr, MII_BMCR, (BMCR_ANENABLE
					 | BMCR_ANRESTART));

	for (i = 0; i < FT_AUTONEG_TIMEOUT / 100; i++) {
		status = ft_mdio_read(ft, ft->phy_addr, MII_BMSR);
		if (status & BMSR_ANEGCOMPLETE)
			break;
		udelay(100);
	}

	if (status & BMSR_ANEGCOMPLETE) {
		ft->autoneg = 1;
		printf("%s: Autonegotiation complete\n", name);
	} else {
		ft->autoneg = 0;
		printf("%s: Autonegotiation timed out (status=0x%04x)\n", name, status);
	}
}

static int ft_phy_find(struct ft_device *ft, const char *name)
{
	int i;
	u16 phy_id;

	phy_id = ft_mdio_read(ft, ft->phy_addr, MII_PHYSID1);
	if (phy_id != 0xffff) {
		printf("%s: PHY present at %d\n", name, ft->phy_addr);
		return 0;
	}

	/* Search for PHY... */
	for (i = 0; i < 32; i++) {
		ft->phy_addr = i;
		phy_id = ft_mdio_read(ft, ft->phy_addr, MII_PHYSID1);
		if (phy_id != 0xffff) {
			printf("%s: PHY present at %d\n", name, i);
			return 0;
		}
	}

	/* PHY isn't up to snuff */
	printf("%s: PHY not found\n", name);

	return -ENODEV;
}

/**
 * ft_linkspd_cb - Linkspeed change callback function
 * @dev/@regs:	FT udevice (DM version) or
 *		Base Register of FT devices (non-DM version)
 * @speed:	Linkspeed
 * Returns 0 when operation success and negative errno number
 * when operation failed.
 */
#ifdef CONFIG_DM_ETH
static int ft_clk_init(struct udevice *dev, ulong rate)
{
	const char *pclk;
	struct ft_device *ft = dev_get_priv(dev);

	pclk = dev_read_prop(dev, "clock", NULL);
	ft->pclk = *pclk;

	return 0;
}

static int ft_eth_sel_clk(struct ft_device *bp, int speed)
{
	u32 hs_ctrl = 0;

	if(bp->phy_interface == PHY_INTERFACE_MODE_USXGMII){
		if(speed == SPEED_10000){
			speed = FT_ETH_SPEED_10000;
			ft_eth_writel(bp, SRC_SEL_LN, 0x1); /*0x1c04*/
			ft_eth_writel(bp, DIV_SEL0_LN, 0x4); /*0x1c08*/
			ft_eth_writel(bp, DIV_SEL1_LN, 0x1); /*0x1c0c*/
			ft_eth_writel(bp, PMA_XCVR_POWER_STATE, 0x1); /*0x1c10*/
		}
	}else if(bp->phy_interface == PHY_INTERFACE_MODE_SGMII){
		if(speed == SPEED_2500){
			printf("ft sgmii speed 2500M!\n");
			speed = FT_ETH_SPEED_2500;
			ft_eth_writel(bp, DIV_SEL0_LN, 0x1); /*0x1c08*/
			ft_eth_writel(bp, DIV_SEL1_LN, 0x2); /*0x1c0c*/
			ft_eth_writel(bp, PMA_XCVR_POWER_STATE, 0x1); /*0x1c10*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x1); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x1); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x1); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x1); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x0); /*0x1c34*/
			ft_eth_writel(bp, TX_CLK_SEL3_0, 0x0); /*0x1c70*/
			ft_eth_writel(bp, TX_CLK_SEL4_0, 0x0); /*0x1c74*/
			ft_eth_writel(bp, RX_CLK_SEL3_0, 0x0); /*0x1c78*/
			ft_eth_writel(bp, RX_CLK_SEL4_0, 0x0); /*0x1c7c*/
		}else if(speed == SPEED_1000){
			speed = FT_ETH_SPEED_1000;
			printf("ft sgmii speed 1000M!\n");
			ft_eth_writel(bp, DIV_SEL0_LN, 0x4); /*0x1c08*/
			ft_eth_writel(bp, DIV_SEL1_LN, 0x8); /*0x1c0c*/
			ft_eth_writel(bp, PMA_XCVR_POWER_STATE, 0x1); /*0x1c10*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x0); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x0); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x1); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x1); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x0); /*0x1c34*/
			ft_eth_writel(bp, TX_CLK_SEL3_0, 0x0); /*0x1c70*/
			ft_eth_writel(bp, TX_CLK_SEL4_0, 0x0); /*0x1c74*/
			ft_eth_writel(bp, RX_CLK_SEL3_0, 0x0); /*0x1c78*/
			ft_eth_writel(bp, RX_CLK_SEL4_0, 0x0); /*0x1c7c*/
		}else if(speed == SPEED_100 || speed == SPEED_10){
			printf("ft sgmii speed 100/10M!\n");
			speed = FT_ETH_SPEED_100;
			ft_eth_writel(bp, DIV_SEL0_LN, 0x4); /*0x1c08*/
			ft_eth_writel(bp, DIV_SEL1_LN, 0x8); /*0x1c0c*/
			ft_eth_writel(bp, PMA_XCVR_POWER_STATE, 0x1); /*0x1c10*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x0); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x1); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x1); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x1); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x0); /*0x1c34*/
			ft_eth_writel(bp, TX_CLK_SEL3_0, 0x1); /*0x1c70*/
			ft_eth_writel(bp, TX_CLK_SEL4_0, 0x0); /*0x1c74*/
			ft_eth_writel(bp, RX_CLK_SEL3_0, 0x0); /*0x1c78*/
			ft_eth_writel(bp, RX_CLK_SEL4_0, 0x1); /*0x1c7c*/
		}
	}else if(bp->phy_interface == PHY_INTERFACE_MODE_RGMII){
		if (speed == SPEED_1000) {
			printf("ft rgmii speed 1000M!\n");
			speed = FT_ETH_SPEED_1000;
			ft_eth_writel(bp, MII_SELECT, 0x1); /*0x1c18*/
			ft_eth_writel(bp, SEL_MII_ON_RGMII, 0x0); /*0x1c1c*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x1); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x0); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x0); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x0); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x1); /*0x1c34*/
			ft_eth_writel(bp, CLK_250M_DIV10_DIV100_SEL, 0x0); /*0x1c38*/
			ft_eth_writel(bp, RX_CLK_SEL5, 0x1); /*0x1c48*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL0, 0x1); /*0x1c80*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL1, 0x0); /*0x1c84*/
		} else if (speed == SPEED_100) {
			printf("ft rgmii speed 100M!\n");
			ft_eth_writel(bp, MII_SELECT, 0x1); /*0x1c18*/
			ft_eth_writel(bp, SEL_MII_ON_RGMII, 0x0); /*0x1c1c*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x1); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x0); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x0); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x0); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x1); /*0x1c34*/
			ft_eth_writel(bp, CLK_250M_DIV10_DIV100_SEL, 0x0); /*0x1c38*/
			ft_eth_writel(bp, RX_CLK_SEL5, 0x1); /*0x1c48*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL0, 0x0); /*0x1c80*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL1, 0x0); /*0x1c84*/
			speed = FT_ETH_SPEED_100;
		} else {
			printf("ft rgmii speed 10M!\n");
			ft_eth_writel(bp, MII_SELECT, 0x1); /*0x1c18*/
			ft_eth_writel(bp, SEL_MII_ON_RGMII, 0x0); /*0x1c1c*/
			ft_eth_writel(bp, TX_CLK_SEL0, 0x0); /*0x1c20*/
			ft_eth_writel(bp, TX_CLK_SEL1, 0x1); /*0x1c24*/
			ft_eth_writel(bp, TX_CLK_SEL2, 0x0); /*0x1c28*/
			ft_eth_writel(bp, TX_CLK_SEL3, 0x0); /*0x1c2c*/
			ft_eth_writel(bp, RX_CLK_SEL0, 0x0); /*0x1c30*/
			ft_eth_writel(bp, RX_CLK_SEL1, 0x1); /*0x1c34*/
			ft_eth_writel(bp, CLK_250M_DIV10_DIV100_SEL, 0x1); /*0x1c38*/
			ft_eth_writel(bp, RX_CLK_SEL5, 0x1); /*0x1c48*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL0, 0x0); /*0x1c80*/
			ft_eth_writel(bp, RGMII_TX_CLK_SEL1, 0x0); /*0x1c84*/
			speed = FT_ETH_SPEED_100;
		}
	}else if(bp->phy_interface == PHY_INTERFACE_MODE_RMII){
		speed = FT_ETH_SPEED_100;
		ft_eth_writel(bp, RX_CLK_SEL5, 0x1); /*0x1c48*/
	}

	/*FT_ETH_HSMAC(0x0050) provide rate to the external*/
	hs_ctrl = ft_eth_readl(bp, HSMAC);
	hs_ctrl = FT_ETH_BFINS(HSMACSPEED, speed, hs_ctrl);
	ft_eth_writel(bp, HSMAC, hs_ctrl);

	return 0;
}

static int ft_eth_init_interface(struct ft_device *bp, int speed, int duplex)
{
	u32 ctrl = 0;

	ft_eth_sel_clk(bp, speed);
	if (bp->phy_interface == PHY_INTERFACE_MODE_XGMII) {
		ctrl = ft_readl(bp, NCFGR);
		ctrl &=~ FT_ETH_BIT(PCSSEL);
		ft_writel(bp, NCFGR, ctrl);

		/*NCR bit[31] enable_hs_mac*/
		ctrl = ft_readl(bp, NCR);
		ctrl |=  FT_BIT(HSMAC);
		ft_writel(bp, NCR, ctrl);
	}else if (bp->phy_interface == PHY_INTERFACE_MODE_USXGMII) {
		ctrl = ft_readl(bp, NCFGR);
		ctrl |= FT_BIT(FD);
		ctrl |= FT_ETH_BIT(PCSSEL);
		ft_writel(bp, NCFGR, ctrl);

		/*NCR bit[31] enable_hs_mac*/
		ctrl &= ~(FT_BIT(RE) | FT_BIT(TE));
		ctrl = ft_readl(bp, NCR);
		ctrl |=  FT_BIT(HSMAC);
		ft_writel(bp, NCR, ctrl);

		ctrl = ft_eth_readl(bp, USX_CONTROL);
		ctrl &= ~(FT_ETH_BIT(TX_SCR_BYPASS) | FT_ETH_BIT(RX_SCR_BYPASS));
		ctrl |= FT_ETH_BIT(RX_SYNC_RESET);

		if(speed == SPEED_10000){
			ctrl = FT_ETH_BFINS(USX_CTRL_SPEED, FT_ETH_SPEED_10000, ctrl);
			ctrl |= FT_ETH_BIT(SERDES_RATE);
		}
		printf("init interface usxgmii: ctrl=0x%x", ctrl);
		ft_eth_writel(bp, USX_CONTROL, ctrl);
		ctrl = ft_eth_readl(bp, USX_CONTROL);
		ctrl &= ~(FT_ETH_BIT(RX_SYNC_RESET));
		ctrl |= FT_ETH_BIT(TX_EN);
		ctrl |= FT_ETH_BIT(SIGNAL_OK);
		printf("init interface usxgmii: ctrl=0x%x", ctrl);
		ft_eth_writel(bp, USX_CONTROL, ctrl);
	}else if (bp->phy_interface == PHY_INTERFACE_MODE_SGMII) {
		u32 pcsctrl;

		ctrl = ft_readl(bp, NCFGR);
		ctrl |= FT_ETH_BIT(PCSSEL) | FT_ETH_BIT(SGMIIEN);
		ctrl &= ~(FT_BIT(SPD) | FT_BIT(FD));
		if (ft_is_eth(bp))
			ctrl &= ~FT_ETH_BIT(GBE);

		if (duplex)
			ctrl |= FT_BIT(FD);

		if (speed == SPEED_100)
			ctrl |= FT_BIT(SPD);

		if (speed == SPEED_1000)
			ctrl |= FT_ETH_BIT(GBE);

		if (speed == SPEED_2500) {
			u32 network_ctrl;
			network_ctrl = ft_readl(bp, NCR);
		    network_ctrl |= FT_BIT(2PT5G);
			ft_writel(bp, NCR, network_ctrl);
			ctrl |= FT_ETH_BIT(GBE);
			ctrl &=~ FT_ETH_BIT(SGMIIEN);
		}

		ft_writel(bp, NCFGR, ctrl);

		ctrl = ft_readl(bp, NCR);
		ctrl &=~  FT_BIT(HSMAC);
		ft_writel(bp, NCR, ctrl);

		if (bp->autoneg) {
		    pcsctrl = ft_eth_readl(bp, PCSCTRL);
			pcsctrl |= FT_ETH_BIT(AUTONEG);
            ft_eth_writel(bp, PCSCTRL, pcsctrl);
		} else {
			pcsctrl = ft_eth_readl(bp, PCSCTRL);
			pcsctrl &=~ FT_ETH_BIT(AUTONEG);
            ft_eth_writel(bp, PCSCTRL, pcsctrl);
		}
	}else {
		ctrl = ft_readl(bp, NCFGR);

		ctrl &=~ FT_ETH_BIT(PCSSEL);
		ctrl &= ~(FT_BIT(SPD) | FT_BIT(FD));
		if (ft_is_eth(bp))
			ctrl &= ~FT_ETH_BIT(GBE);

		if (duplex)
			ctrl |= FT_BIT(FD);
		if (speed == SPEED_100)
			ctrl |= FT_BIT(SPD);
		if (speed == SPEED_1000)
			ctrl |= FT_ETH_BIT(GBE);

		ft_writel(bp, NCFGR, ctrl);

		ctrl = ft_readl(bp, NCR);
		ctrl &=~  FT_BIT(HSMAC);
		ft_writel(bp, NCR, ctrl);
	}

	return 0;
}

int __weak ft_linkspd_cb(struct udevice *dev, unsigned int speed)
{
#ifdef CONFIG_CLK
	struct ft_device *ft = dev_get_priv(dev);
	struct clk tx_clk;
	ulong rate;
	int ret;

	switch (speed) {
	case _10BASET:
		rate = 2500000;		/* 2.5 MHz */
		break;
	case _100BASET:
		rate = 25000000;	/* 25 MHz */
		break;
	case _1000BASET:
		rate = 125000000;	/* 125 MHz */
		break;
	default:
		/* does not change anything */
		return 0;
	}

	if (ft->config->clk_init)
		return ft->config->clk_init(dev, rate);

	/*
	 * "tx_clk" is an optional clock source for FT.
	 * Ignore if it does not exist in DT.
	 */
	ret = clk_get_by_name(dev, "tx_clk", &tx_clk);
	if (ret)
		return 0;

	if (tx_clk.dev) {
		ret = clk_set_rate(&tx_clk, rate);
		if (ret < 0)
			return ret;
	}
#endif

	return 0;
}
#else
int __weak ft_linkspd_cb(void *regs, unsigned int speed)
{
	return 0;
}
#endif

#ifdef CONFIG_DM_ETH
static int ft_phy_init(struct udevice *dev, const char *name)
#else
static int ft_phy_init(struct ft_device *ft, const char *name)
#endif
{
#ifdef CONFIG_DM_ETH
	struct ft_device *ft = dev_get_priv(dev);
#endif
	u32 ncfgr;
	u16 phy_id, status, adv, lpa;
	int media, speed, duplex;
	int ret;
	int i;

	arch_get_mdio_control(name);
	/* Auto-detect phy_addr */
	ret = ft_phy_find(ft, name);
	if (ret)
		return ret;

	/* Check if the PHY is up to snuff... */
	phy_id = ft_mdio_read(ft, ft->phy_addr, MII_PHYSID1);
	if (phy_id == 0xffff) {
		printf("%s: No PHY present\n", name);
		return -ENODEV;
	}

#ifdef CONFIG_PHYLIB
#ifdef CONFIG_DM_ETH
	ft->phydev = phy_connect(ft->bus, ft->phy_addr, dev,
			     ft->phy_interface);
#else
	/* need to consider other phy interface mode */
	ft->phydev = phy_connect(ft->bus, ft->phy_addr, &ft->netdev,
			     PHY_INTERFACE_MODE_RGMII);
#endif
	if (!ft->phydev) {
		printf("phy_connect failed\n");
		return -ENODEV;
	}

	phy_config(ft->phydev);
#endif

	status = ft_mdio_read(ft, ft->phy_addr, MII_BMSR);
	if (!(status & BMSR_LSTATUS)) {
		/* Try to re-negotiate if we don't have link already. */
		ft_phy_reset(ft, name);

		for (i = 0; i < FT_AUTONEG_TIMEOUT / 100; i++) {
			status = ft_mdio_read(ft, ft->phy_addr, MII_BMSR);
			if (status & BMSR_LSTATUS) {
				/*
				 * Delay a bit after the link is established,
				 * so that the next xfer does not fail
				 */
				mdelay(10);
				break;
			}
			udelay(100);
		}
	}

	if (!(status & BMSR_LSTATUS)) {
		printf("%s: link down (status: 0x%04x)\n",
		       name, status);
		return -ENETDOWN;
	}

	/* First check for GMAC and that it is GiB capable */
	if (ft_eth_is_gigabit_capable(ft)) {
		lpa = ft_mdio_read(ft, ft->phy_addr, MII_STAT1000);

		if (lpa & (LPA_1000FULL | LPA_1000HALF | LPA_1000XFULL |
					LPA_1000XHALF)) {
			duplex = ((lpa & (LPA_1000FULL | LPA_1000XFULL)) ?
					1 : 0);

			printf("%s: link up, 1000Mbps %s-duplex (lpa: 0x%04x)\n",
			       name,
			       duplex ? "full" : "half",
			       lpa);

			ncfgr = ft_readl(ft, NCFGR);
			ncfgr &= ~(FT_BIT(SPD) | FT_BIT(FD));
			ncfgr |= FT_ETH_BIT(GBE);

			if (duplex)
				ncfgr |= FT_BIT(FD);

			ft_writel(ft, NCFGR, ncfgr);

#ifdef CONFIG_DM_ETH
			ft_eth_init_interface(ft, SPEED_1000, duplex);
			ret = ft_linkspd_cb(dev, _1000BASET);
#else
			ret = ft_linkspd_cb(ft->regs, _1000BASET);
#endif
			if (ret)
				return ret;

			return 0;
		}
	}

	/* fall back for EMAC checking */
	adv = ft_mdio_read(ft, ft->phy_addr, MII_ADVERTISE);
	lpa = ft_mdio_read(ft, ft->phy_addr, MII_LPA);
	media = mii_nway_result(lpa & adv);
	speed = (media & (ADVERTISE_100FULL | ADVERTISE_100HALF)
		 ? 1 : 0);
	duplex = (media & ADVERTISE_FULL) ? 1 : 0;
	printf("%s: link up, %sMbps %s-duplex (lpa: 0x%04x)\n",
	       name,
	       speed ? "100" : "10",
	       duplex ? "full" : "half",
	       lpa);

	ncfgr = ft_readl(ft, NCFGR);
	ncfgr &= ~(FT_BIT(SPD) | FT_BIT(FD) | FT_ETH_BIT(GBE));
	if (speed) {
		ncfgr |= FT_BIT(SPD);
#ifdef CONFIG_DM_ETH
		ft_eth_init_interface(ft, SPEED_100, duplex);
		ret = ft_linkspd_cb(dev, _100BASET);
#else
		ret = ft_linkspd_cb(ft->regs, _100BASET);
#endif
	} else {
#ifdef CONFIG_DM_ETH
		ft_eth_init_interface(ft, SPEED_10, duplex);
		ret = ft_linkspd_cb(dev, _10BASET);
#else
		ret = ft_linkspd_cb(ft->regs, _10BASET);
#endif
	}

	if (ret)
		return ret;

	if (duplex)
		ncfgr |= FT_BIT(FD);
	ft_writel(ft, NCFGR, ncfgr);

	return 0;
}

static int gmac_init_multi_queues(struct ft_device *ft)
{
	int i, num_queues = 1;
	u32 queue_mask;
	unsigned long paddr;

	/* bit 0 is never set but queue 0 always exists */
	queue_mask = ft_eth_readl(ft, DCFG6) & 0xff;
	queue_mask |= 0x1;

	for (i = 1; i < FT_MAX_QUEUES; i++)
		if (queue_mask & (1 << i))
			num_queues++;

	ft->dummy_desc->ctrl = FT_BIT(TX_USED);
	ft->dummy_desc->addr = 0;
	flush_dcache_range(ft->dummy_desc_dma, ft->dummy_desc_dma +
			ALIGN(FT_TX_DUMMY_DMA_DESC_SIZE, PKTALIGN));
	paddr = ft->dummy_desc_dma;

	for (i = 1; i < num_queues; i++) {
		ft_eth_writel_queue_TBQP(ft, lower_32_bits(paddr), i - 1);
		ft_eth_writel_queue_RBQP(ft, lower_32_bits(paddr), i - 1);
		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
			ft_eth_writel_queue_TBQPH(ft, upper_32_bits(paddr),
					       i - 1);
			ft_eth_writel_queue_RBQPH(ft, upper_32_bits(paddr),
					       i - 1);
		}
	}
	return 0;
}

static void gmac_configure_dma(struct ft_device *ft)
{
	u32 buffer_size;
	u32 dmacfg;

	buffer_size = ft->rx_buffer_size / RX_BUFFER_MULTIPLE;
	dmacfg = ft_eth_readl(ft, DMACFG) & ~FT_ETH_BF(RXBS, -1L);
	dmacfg |= FT_ETH_BF(RXBS, buffer_size);

	if (ft->config->dma_burst_length)
		dmacfg = FT_ETH_BFINS(FBLDO,
				   ft->config->dma_burst_length, dmacfg);

	dmacfg |= FT_ETH_BIT(TXPBMS) | FT_ETH_BF(RXBMS, -1L);
	dmacfg &= ~FT_ETH_BIT(ENDIA_PKT);

	if (ft->is_big_endian)
		dmacfg |= FT_ETH_BIT(ENDIA_DESC); /* CPU in big endian */
	else
		dmacfg &= ~FT_ETH_BIT(ENDIA_DESC);

	dmacfg &= ~FT_ETH_BIT(ADDR64);
	if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
		dmacfg |= FT_ETH_BIT(ADDR64);

	ft_eth_writel(ft, DMACFG, dmacfg);
}

#ifdef CONFIG_DM_ETH
static int _ft_init(struct udevice *dev, const char *name)
#else
static int _ft_init(struct ft_device *ft, const char *name)
#endif
{
#ifdef CONFIG_DM_ETH
	struct ft_device *ft = dev_get_priv(dev);
	unsigned int val = 0;
#endif
	unsigned long paddr;
	int ret;
	int i;
	int count;

	/*
	 * ft_halt should have been called at some point before now,
	 * so we'll assume the controller is idle.
	 */

	/* initialize DMA descriptors */
	paddr = ft->rx_buffer_dma;
	for (i = 0; i < FT_RX_RING_SIZE; i++) {
		if (i == (FT_RX_RING_SIZE - 1))
			paddr |= FT_BIT(RX_WRAP);
		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
			count = i * 2;
		else
			count = i;
		ft->rx_ring[count].ctrl = 0;
		ft_set_addr(ft, &ft->rx_ring[count], paddr);
		paddr += ft->rx_buffer_size;
	}
	ft_flush_ring_desc(ft, RX);
	ft_flush_rx_buffer(ft);

	for (i = 0; i < FT_TX_RING_SIZE; i++) {
		if (ft->config->hw_dma_cap & HW_DMA_CAP_64B)
			count = i * 2;
		else
			count = i;
		ft_set_addr(ft, &ft->tx_ring[count], 0);
		if (i == (FT_TX_RING_SIZE - 1))
			ft->tx_ring[count].ctrl = FT_BIT(TX_USED) |
				FT_BIT(TX_WRAP);
		else
			ft->tx_ring[count].ctrl = FT_BIT(TX_USED);
	}
	ft_flush_ring_desc(ft, TX);

	ft->rx_tail = 0;
	ft->tx_head = 0;
	ft->tx_tail = 0;
	ft->next_rx_tail = 0;

	ft_writel(ft, RBQP, lower_32_bits(ft->rx_ring_dma));
	ft_writel(ft, TBQP, lower_32_bits(ft->tx_ring_dma));
	if (ft->config->hw_dma_cap & HW_DMA_CAP_64B) {
		ft_writel(ft, RBQPH, upper_32_bits(ft->rx_ring_dma));
		ft_writel(ft, TBQPH, upper_32_bits(ft->tx_ring_dma));
	}

	if (ft_is_eth(ft)) {
		/* Initialize DMA properties */
		gmac_configure_dma(ft);
		/* Check the multi queue and initialize the queue for tx */
		gmac_init_multi_queues(ft);

		/*
		 * When the GMAC IP with GE feature, this bit is used to
		 * select interface between RGMII and GMII.
		 * When the GMAC IP without GE feature, this bit is used
		 * to select interface between RMII and MII.
		 */
#ifdef CONFIG_DM_ETH
		if (ft->phy_interface == PHY_INTERFACE_MODE_RGMII ||
		    ft->phy_interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    ft->phy_interface == PHY_INTERFACE_MODE_RGMII_RXID ||
		    ft->phy_interface == PHY_INTERFACE_MODE_RGMII_TXID)
			val = ft->config->usrio->rgmii;
		else if (ft->phy_interface == PHY_INTERFACE_MODE_RMII)
			val = ft->config->usrio->rmii;
		else if (ft->phy_interface == PHY_INTERFACE_MODE_MII)
			val = ft->config->usrio->mii;

		if (ft->config->caps & FT_CAPS_USRIO_HAS_CLKEN)
			val |= ft->config->usrio->clken;

		ft_eth_writel(ft, USRIO, val);

		if (ft->phy_interface == PHY_INTERFACE_MODE_SGMII) {
			unsigned int ncfgr = ft_readl(ft, NCFGR);

			ncfgr |= FT_ETH_BIT(SGMIIEN) | FT_ETH_BIT(PCSSEL);
			ft_writel(ft, NCFGR, ncfgr);
		}
#else
#if defined(CONFIG_RGMII) || defined(CONFIG_RMII)
		ft_eth_writel(ft, USRIO, ft->config->usrio->rgmii);
#else
		ft_eth_writel(ft, USRIO, 0);
#endif
#endif
	} else {
	/* choose RMII or MII mode. This depends on the board */
#ifdef CONFIG_DM_ETH
#ifdef CONFIG_AT91FAMILY
		if (ft->phy_interface == PHY_INTERFACE_MODE_RMII) {
			ft_writel(ft, USRIO,
				    ft->config->usrio->rmii |
				    ft->config->usrio->clken);
		} else {
			ft_writel(ft, USRIO, ft->config->usrio->clken);
		}
#else
		if (ft->phy_interface == PHY_INTERFACE_MODE_RMII)
			ft_writel(ft, USRIO, 0);
		else
			ft_writel(ft, USRIO, ft->config->usrio->mii);
#endif
#else
#ifdef CONFIG_RMII
#ifdef CONFIG_AT91FAMILY
	ft_writel(ft, USRIO, ft->config->usrio->rmii |
		    ft->config->usrio->clken);
#else
	ft_writel(ft, USRIO, 0);
#endif
#else
#ifdef CONFIG_AT91FAMILY
	ft_writel(ft, USRIO, ft->config->usrio->clken);
#else
	ft_writel(ft, USRIO, ft->config->usrio->mii);
#endif
#endif /* CONFIG_RMII */
#endif
	}

#ifdef CONFIG_DM_ETH
	ret = ft_phy_init(dev, name);
#else
	ret = ft_phy_init(ft, name);
#endif
	if (ret)
		return ret;

	/* Enable TX and RX */
	ft_writel(ft, NCR, FT_BIT(TE) | FT_BIT(RE));

	return 0;
}

static void _ft_halt(struct ft_device *ft)
{
	u32 ncr, tsr;

	/* Halt the controller and wait for any ongoing transmission to end. */
	ncr = ft_readl(ft, NCR);
	ncr |= FT_BIT(THALT);
	ft_writel(ft, NCR, ncr);

	do {
		tsr = ft_readl(ft, TSR);
	} while (tsr & FT_BIT(TGO));

	/* Disable TX and RX, and clear statistics */
	ft_writel(ft, NCR, FT_BIT(CLRSTAT));
}

static int _ft_write_hwaddr(struct ft_device *ft, unsigned char *enetaddr)
{
	u32 hwaddr_bottom;
	u16 hwaddr_top;
	
	memcpy(enetaddr, ft->regs + FT_ETH_SA1B, ARP_HLEN);

	/* set hardware address */
	
	hwaddr_bottom = enetaddr[0] | enetaddr[1] << 8 |
			enetaddr[2] << 16 | enetaddr[3] << 24;
	ft_eth_writel(ft, SA1B, hwaddr_bottom);
	
	hwaddr_top = enetaddr[4] | enetaddr[5] << 8;
	ft_eth_writel(ft, SA1T, hwaddr_top);

	return 0;
}

static u32 ft_mdc_clk_div(int id, struct ft_device *ft)
{
	u32 config;
#if defined(CONFIG_DM_ETH) && defined(CONFIG_CLK)
	unsigned long ft_hz = ft->pclk_rate;
#else
	unsigned long ft_hz = ft->pclk;
#endif

	if (ft_hz < 20000000)
		config = FT_BF(CLK, FT_CLK_DIV8);
	else if (ft_hz < 40000000)
		config = FT_BF(CLK, FT_CLK_DIV16);
	else if (ft_hz < 80000000)
		config = FT_BF(CLK, FT_CLK_DIV32);
	else
		config = FT_BF(CLK, FT_CLK_DIV64);

	return config;
}

static u32 ft_eth_mdc_clk_div(int id, struct ft_device *ft)
{
	u32 config;

#if defined(CONFIG_DM_ETH) && defined(CONFIG_CLK)
	unsigned long ft_hz = ft->pclk_rate;
#else
	unsigned long ft_hz = ft->pclk;
#endif

	if (ft_hz < 20000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV8);
	else if (ft_hz < 40000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV16);
	else if (ft_hz < 80000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV32);
	else if (ft_hz < 120000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV48);
	else if (ft_hz < 160000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV64);
	else if (ft_hz < 240000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV96);
	else if (ft_hz < 320000000)
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV128);
	else
		config = FT_ETH_BF(CLK, FT_ETH_CLK_DIV224);

	return config;
}

/*
 * Get the DMA bus width field of the network configuration register that we
 * should program. We find the width from decoding the design configuration
 * register to find the maximum supported data bus width.
 */
static u32 ft_dbw(struct ft_device *ft)
{
	switch (FT_ETH_BFEXT(DBWDEF, ft_eth_readl(ft, DCFG1))) {
	case 4:
		return FT_ETH_BF(DBW, FT_ETH_DBW128);
	case 2:
		return FT_ETH_BF(DBW, FT_ETH_DBW64);
	case 1:
	default:
		return FT_ETH_BF(DBW, FT_ETH_DBW32);
	}
}

static void _ft_eth_initialize(struct ft_device *ft)
{
	int id = 0;	/* This is not used by functions we call */
	u32 ncfgr;

	if (ft_is_eth(ft))
		ft->rx_buffer_size = FT_ETH_RX_BUFFER_SIZE;
	else
		ft->rx_buffer_size = FT_RX_BUFFER_SIZE;

	/* TODO: we need check the rx/tx_ring_dma is dcache line aligned */
	ft->rx_buffer = dma_alloc_coherent(ft->rx_buffer_size *
					     FT_RX_RING_SIZE,
					     &ft->rx_buffer_dma);
	ft->rx_ring = dma_alloc_coherent(FT_RX_DMA_DESC_SIZE,
					   &ft->rx_ring_dma);
	ft->tx_ring = dma_alloc_coherent(FT_TX_DMA_DESC_SIZE,
					   &ft->tx_ring_dma);
	ft->dummy_desc = dma_alloc_coherent(FT_TX_DUMMY_DMA_DESC_SIZE,
					   &ft->dummy_desc_dma);

	/*
	 * Do some basic initialization so that we at least can talk
	 * to the PHY
	 */
	if (ft_is_eth(ft)) {
		ncfgr = ft_eth_mdc_clk_div(id, ft);
		ncfgr |= ft_dbw(ft);
	} else {
		ncfgr = ft_mdc_clk_div(id, ft);
	}

	ft_writel(ft, NCFGR, ncfgr);
}

#ifndef CONFIG_DM_ETH
static int ft_send(struct eth_device *netdev, void *packet, int length)
{
	struct ft_device *ft = to_ft(netdev);

	return _ft_send(ft, netdev->name, packet, length);
}

static int ft_recv(struct eth_device *netdev)
{
	struct ft_device *ft = to_ft(netdev);
	uchar *packet;
	int length;

	ft->wrapped = false;
	for (;;) {
		ft->next_rx_tail = ft->rx_tail;
		length = _ft_recv(ft, &packet);
		if (length >= 0) {
			net_process_received_packet(packet, length);
			reclaim_rx_buffers(ft, ft->next_rx_tail);
		} else {
			return length;
		}
	}
}

static int ft_init(struct eth_device *netdev, struct bd_info *bd)
{
	struct ft_device *ft = to_ft(netdev);

	return _ft_init(ft, netdev->name);
}

static void ft_halt(struct eth_device *netdev)
{
	struct ft_device *ft = to_ft(netdev);

	return _ft_halt(ft);
}

static int ft_write_hwaddr(struct eth_device *netdev)
{
	struct ft_device *ft = to_ft(netdev);

	return _ft_write_hwaddr(ft, netdev->enetaddr);
}

int ft_eth_initialize(int id, void *regs, unsigned int phy_addr)
{
	struct ft_device *ft;
	struct eth_device *netdev;

	ft = malloc(sizeof(struct ft_device));
	if (!ft) {
		printf("Error: Failed to allocate memory for FT%d\n", id);
		return -1;
	}
	memset(ft, 0, sizeof(struct ft_device));

	netdev = &ft->netdev;

	ft->regs = regs;
	ft->phy_addr = phy_addr;

	if (ft_is_eth(ft))
		sprintf(netdev->name, "gmac%d", id);
	else
		sprintf(netdev->name, "eth%d", id);

	netdev->init = ft_init;
	netdev->halt = ft_halt;
	netdev->send = ft_send;
	netdev->recv = ft_recv;
	netdev->write_hwaddr = ft_write_hwaddr;

	_ft_eth_initialize(ft);

	eth_register(netdev);

#if defined(CONFIG_CMD_MII) || defined(CONFIG_PHYLIB)
	int retval;
	struct mii_dev *mdiodev = mdio_alloc();
	if (!mdiodev)
		return -ENOMEM;
	strlcpy(mdiodev->name, netdev->name, MDIO_NAME_LEN);
	mdiodev->read = ft_miiphy_read;
	mdiodev->write = ft_miiphy_write;

	retval = mdio_register(mdiodev);
	if (retval < 0)
		return retval;
	ft->bus = miiphy_get_dev_by_name(netdev->name);
#endif
	return 0;
}
#endif /* !CONFIG_DM_ETH */

#ifdef CONFIG_DM_ETH

static int ft_start(struct udevice *dev)
{
	return _ft_init(dev, dev->name);
}

static int ft_send(struct udevice *dev, void *packet, int length)
{
	struct ft_device *ft = dev_get_priv(dev);

	return _ft_send(ft, dev->name, packet, length);
}

static int ft_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct ft_device *ft = dev_get_priv(dev);

	ft->next_rx_tail = ft->rx_tail;
	ft->wrapped = false;

	return _ft_recv(ft, packetp);
}

static int ft_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct ft_device *ft = dev_get_priv(dev);

	reclaim_rx_buffers(ft, ft->next_rx_tail);

	return 0;
}

static void ft_stop(struct udevice *dev)
{
	struct ft_device *ft = dev_get_priv(dev);

	_ft_halt(ft);
}

static int ft_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *plat = dev_get_plat(dev);
	struct ft_device *ft = dev_get_priv(dev);

	return _ft_write_hwaddr(ft, plat->enetaddr);
}

static const struct eth_ops ft_eth_ops = {
	.start	= ft_start,
	.send	= ft_send,
	.recv	= ft_recv,
	.stop	= ft_stop,
	.free_pkt	= ft_free_pkt,
	.write_hwaddr	= ft_write_hwaddr,
};

#ifdef CONFIG_CLK
static int ft_enable_clk(struct udevice *dev)
{
	struct ft_device *ft = dev_get_priv(dev);
	struct clk clk;
	ulong clk_rate;
	int ret;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret)
		return -EINVAL;

	/*
	 * If clock driver didn't support enable or disable then
	 * we get -ENOSYS from clk_enable(). To handle this, we
	 * don't fail for ret == -ENOSYS.
	 */
	ret = clk_enable(&clk);
	if (ret && ret != -ENOSYS)
		return ret;

	clk_rate = clk_get_rate(&clk);
	if (!clk_rate)
		return -EINVAL;

	ft->pclk_rate = clk_rate;

	return 0;
}
#endif

static const struct ft_usrio_cfg ft_default_usrio = {
	.mii = FT_BIT(MII),
	.rmii = FT_BIT(RMII),
	.rgmii = FT_ETH_BIT(RGMII),
	.clken = FT_BIT(CLKEN),
};

static struct ft_config default_ft_eth_config = {
	.dma_burst_length = 16,
	.hw_dma_cap = HW_DMA_CAP_32B,
	.clk_init = NULL,
	.usrio = &ft_default_usrio,
};
#endif

static int ft_eth_probe(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);
	struct ft_device *ft = dev_get_priv(dev);
	struct ofnode_phandle_args phandle_args;
	const char *phy_mode;
	int ret;

	phy_mode = dev_read_prop(dev, "phy-mode", NULL);

	if (phy_mode)
		ft->phy_interface = phy_get_interface_by_name(phy_mode);
	if (ft->phy_interface == -1) {
		debug("%s: Invalid PHY interface '%s'\n", __func__, phy_mode);
		return -EINVAL;
	}

	/* Read phyaddr from DT */
	if (!dev_read_phandle_with_args(dev, "phy-handle", NULL, 0, 0,
					&phandle_args))
		ft->phy_addr = ofnode_read_u32_default(phandle_args.node,
							 "reg", -1);

	ft->regs = (void *)(uintptr_t)pdata->iobase;

	ft->is_big_endian = (cpu_to_be32(0x12345678) == 0x12345678);

	ft->config = (struct ft_config *)dev_get_driver_data(dev);
	if (!ft->config) {
		if (IS_ENABLED(CONFIG_DMA_ADDR_T_64BIT)) {
			if (FT_ETH_BFEXT(DAW64, ft_eth_readl(ft, DCFG6)))
				default_ft_eth_config.hw_dma_cap = HW_DMA_CAP_64B;
		}
		ft->config = &default_ft_eth_config;
	}

#ifdef CONFIG_CLK
	ret = ft_enable_clk(dev);
	if (ret)
		return ret;
#endif

	_ft_eth_initialize(ft);

#if defined(CONFIG_CMD_MII) || defined(CONFIG_PHYLIB)
	ft->bus = mdio_alloc();
	if (!ft->bus)
		return -ENOMEM;
	strlcpy(ft->bus->name, dev->name, MDIO_NAME_LEN);
	ft->bus->read = ft_miiphy_read;
	ft->bus->write = ft_miiphy_write;

	ret = mdio_register(ft->bus);
	if (ret < 0)
		return ret;
	ft->bus = miiphy_get_dev_by_name(dev->name);
#endif

	return 0;
}

static int ft_eth_remove(struct udevice *dev)
{
	struct ft_device *ft = dev_get_priv(dev);

#ifdef CONFIG_PHYLIB
	free(ft->phydev);
#endif
	mdio_unregister(ft->bus);
	mdio_free(ft->bus);

	return 0;
}

/**
 * ft_late_eth_of_to_plat
 * @dev:	udevice struct
 * Returns 0 when operation success and negative errno number
 * when operation failed.
 */
int __weak ft_late_eth_of_to_plat(struct udevice *dev)
{
	return 0;
}

static int ft_eth_of_to_plat(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);

	pdata->iobase = (uintptr_t)dev_remap_addr(dev);
	if (!pdata->iobase)
		return -EINVAL;

	return ft_late_eth_of_to_plat(dev);
}

static const struct ft_config ft_config = {
	.caps = FT_CAPS_GIGABIT_MODE_AVAILABLE | FT_CAPS_JUMBO | FT_CAPS_FT_ETH_HAS_PTP | FT_CAPS_BD_RD_PREFETCH,
	.dma_burst_length = 16,
	.hw_dma_cap = HW_DMA_CAP_32B,
	.clk_init = ft_clk_init,
	.usrio = &ft_default_usrio,
};

static const struct udevice_id ft_eth_ids[] = {
	{ .compatible = "phytium,phytium-eth",
	  .data = (ulong)&ft_config},
	{ }
};

U_BOOT_DRIVER(eth_ft) = {
	.name	= "eth_phytium",
	.id	= UCLASS_ETH,
	.of_match = ft_eth_ids,
	.of_to_plat = ft_eth_of_to_plat,
	.probe	= ft_eth_probe,
	.remove	= ft_eth_remove,
	.ops	= &ft_eth_ops,
	.priv_auto	= sizeof(struct ft_device),
	.plat_auto	= sizeof(struct eth_pdata),
};
#endif
