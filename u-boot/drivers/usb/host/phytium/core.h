/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#ifndef __PHYTIUM_USB2_CORE_H__
#define __PHYTIUM_USB2_CORE_H__

#include <linux/usb/gadget.h>
#include <linux/usb/otg.h>
#include "host_api.h"
#include "hw-regs.h"

#define MAX_EPS_CHANNELS	16
//#define USB2_DEBUG

struct ft_priv {
	struct hw_regs	*regs;
	void __iomem	*phy_regs;
	struct phytium_cusb *config;
	int root_hub_devnum;
	u8 in_data_toggle[16][16];
	u8 out_data_toggle[16][16];
};

struct phytium_ep {
	struct phytium_cusb	*config;
	u16			max_packet;
	u8			ep_num;
	struct list_head	req_list;
	struct usb_ep		end_point;
	char			name[12];
	u8 is_tx;
	const struct usb_endpoint_descriptor	*desc;
	u8			busy;
};

struct phytium_request {
	struct usb_request request;
	struct list_head	list;
	struct phytium_ep	*ep;
	struct phytium_cusb	*config;
	u8			is_tx;
	u8			epnum;
};

struct phytium_cusb {
	struct udevice		*dev;
	void __iomem		*regs;
	void __iomem		*phy_regs;
	int			irq;
	spinlock_t		lock;
	enum usb_dr_mode	dr_mode;
	struct phytium_ep	endpoints_tx[MAX_EPS_CHANNELS];
	struct phytium_ep	endpoints_rx[MAX_EPS_CHANNELS];
	u8			ep0_data_stage_is_tx;

	struct host_obj		*host_obj;
	struct host_cfg		host_cfg;
	struct host_sysreq	host_sysreq;
	void			*host_priv;
	struct usb_hcd          *hcd;

	struct dma_obj		*dma_obj;
	struct dma_cfg		dma_cfg;
	struct dma_callbacks	dma_callbacks;
	struct dma_sysreq	dma_sysreq;
	bool	is_vhub_host;
	struct dma_ctrl dma_controller;
};

int phytium_core_reset(struct phytium_cusb *config, bool skip_wait);

int phytium_host_init(struct phytium_cusb *config);

int phytium_host_uninit(struct phytium_cusb *config);

#ifdef CONFIG_PM
int phytium_host_resume(void *priv);
int phytium_host_suspend(void *priv);
int phytium_gadget_resume(void *priv);
int phytium_gadget_suspend(void *priv);
#endif

uint32_t phytium_read32(uint32_t *address);

void phytium_write32(uint32_t *address, uint32_t value);

uint16_t phytium_read16(uint16_t *address);

void phytium_write16(uint16_t *address, uint16_t value);

uint8_t phytium_read8(uint8_t *address);

void phytium_write8(uint8_t *address, uint8_t value);

#endif
