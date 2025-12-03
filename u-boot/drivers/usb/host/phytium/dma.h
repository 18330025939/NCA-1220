/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#ifndef __PHYTIUM_USB2_DMA_H__
#define __PHYTIUM_USB2_DMA_H__

#define NUM_OF_TRB 1024
#define TRB_MAP_SIZE ((NUM_OF_TRB + (sizeof(uint8_t) * 8) - 1) / (sizeof(uint8_t) * 8))
#define MAX_DMA_CHANNELS 16
#define TAB_SIZE_OF_DMA_CHAIN (MAX_DMA_CHANNELS * 2)

#define DMARD_EP_TX 0x80ul
#define DMARD_EP_RX 0x00ul

#define DMARF_EP_EPRST	0x00000001ul
#define DMARF_EP_DRDY	0x00000040ul
#define DMARF_EP_DFLUSH	0x00000080ul

#define DMARF_EP_IOC		0x4ul
#define DMARF_EP_ISP		0x8ul
#define DMARF_EP_DESCMIS	0x10ul
#define DMARF_EP_TRBERR		0x80ul
#define DMARF_EP_DBUSY		0x200ul
#define DMARF_EP_CCS		0x800ul
#define DMARF_EP_OUTSMM		0x4000ul
#define DMARF_EP_ISOERR		0x8000ul
#define DMARF_EP_DTRANS		0x80000000ul

#define DMARV_EP_DISABLED	0ul
#define DMARV_EP_ENABLED	1ul
#define DMARV_EP_DSING		0x1000ul
#define DMARV_EP_DMULT		0x2000ul

#define TD_SIZE_MASK		0x00001FFFF

#define DMARF_RESET		0x00000001ul
#define DMARF_DSING		0x00000100ul
#define DMARF_DMULT		0x00000200ul

#define TD_DMULT_MAX_TRB_DATA_SIZE 65536u
#define TD_DMULT_MAX_TD_DATA_SIZE  (~1u)
#define TD_SING_MAX_TRB_DATA_SIZE 65536u
#define TD_SING_MAX_TD_DATA_SIZE 65536u

#define TD_TYPE_NORMAL	0x400L
#define TD_TYPE_LINK	0x1800L
#define TDF_CYCLE_BIT	0x1L
#define TDF_TOGGLE_CYCLE_BIT	0x2L
#define TDF_INT_ON_SHORT_PACKET	0x4L
#define TDF_FIFO_MODE		0x8L
#define TDF_CHAIN_BIT		0x10L
#define TDF_INT_ON_COMPLECTION	0x20L
#define TDF_STREAMID_VALID	0x200L

struct dma_trb {
	uint32_t dma_addr;
	uint32_t dma_size;
	uint32_t ctrl;
};

enum dma_status {
	DMA_STATUS_UNKNOW,
	DMA_STATUS_FREE,
	DMA_STATUS_ABORT,
	DMA_STATUS_BUSY,
	DMA_STATUS_ARMED
};

struct dma_cfg {
	uintptr_t reg_base;
	uint16_t dma_mode_tx;
	uint16_t dma_mode_rx;
	void *trb_addr;
	uintptr_t trb_dma_addr;
};

struct dma_sysreq {
	uint32_t priv_data_size;
	uint32_t trb_mem_size;
};

struct dma_callbacks {
	void (*complete)(void *pd, uint8_t ep_num, uint8_t dir);
};

struct dma_ctrl;

struct dma_channel {
	struct dma_ctrl *controller;
	uint16_t w_max_packet_size;
	uint8_t hw_usb_ep_pnum;
	uint8_t is_dir_tx;
	uint32_t max_td_len;
	uint32_t max_trb_len;
	enum dma_status status;
	void *priv;
	uint32_t dmult_guard;
	uint8_t dmult_enabled;
	uint8_t num_of_trb_chain;
	struct list_head trb_chain_desc_list;
	uint32_t last_transfer_length;
	uint8_t is_isoc;
};

struct dma_obj {
	int32_t (*dma_channel_program)(struct dma_ctrl *priv, u8 ep_num,
			u8 is_in, u32 trb_addr);
	int32_t (*dma_channel_release)(struct dma_ctrl *priv, u8 ep_num, u8 is_in);
	bool (*dma_check_interrupt)(struct dma_ctrl *priv, u8 ep_num, u8 is_in);
	void (*dma_controller_reset)(struct dma_ctrl  *priv);
	void (*dma_set_host_mode)(struct dma_ctrl  *priv);
};

enum dma_mode {
	DMA_MODE_GLOBAL_DMULT,
	DMA_MODE_GLOBAL_DSING,
	DMA_MODE_CHANNEL_INDIVIDUAL,
};

struct dma_trb_frame_desc {
	uint32_t length;
	uint32_t offset;
};

struct dma_trb_chain_desc {
	uint8_t reserved;
	struct dma_channel *channel;
	struct dma_trb	*trb_pool;
	uint32_t trb_dma_addr;
	uint32_t len;
	uint32_t dw_start_address;
	uint32_t actual_len;
	uint8_t iso_error;
	uint8_t last_trb_is_link;
	uint32_t map_size;
	uint32_t num_of_trbs;
	uint32_t start;
	uint32_t end;
	struct dma_trb_frame_desc *frames_desc;
	struct list_head chain_node;
};

struct dma_ctrl {
	struct dma_regs *regs;
	struct dma_obj *dma_drv;
	struct dma_cfg dma_cfg;
	enum dma_mode dma_mode;
	uint8_t is_host_ctrl_mode;
};

struct dma_obj *dma_get_instance(void);

#endif
