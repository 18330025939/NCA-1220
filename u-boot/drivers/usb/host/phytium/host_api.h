/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#ifndef __PHYTIUM_USB2_HOST_API_H_
#define __PHYTIUM_USB2_HOST_API_H_

#include <linux/usb/ch9.h>
#include "dma.h"

#define MAX_SUPPORTED_DEVICES 16
#define USB_PORT_STAT_RESUME (1 << 31)
#define MAX_INSTANCE_EP_NUM 6

#define SIZE_PER_DMA_PACKET	512

#define USB_PORT_FEAT_L1	5	/*L1 suspend*/
#define USB_PORT_FEAT_INDICATOR	22
#define USB_PORT_FEAT_C_PORT_L1	23

#define MAX_DEVICE		16
#define MAX_ENDPOINT	16

// USB Transfer Results
#define USB_NOERROR             0x00
#define USB_ERR_NOTEXECUTE      0x01
#define USB_ERR_STALL           0x02
#define USB_ERR_BUFFER          0x04
#define USB_ERR_BABBLE          0x08
#define USB_ERR_NAK             0x10
#define USB_ERR_CRC             0x20
#define USB_ERR_TIMEOUT         0x40
#define USB_ERR_BITSTUFF        0x80
#define USB_ERR_SYSTEM          0x100

enum host_otg_state {
	HOST_OTG_STATE_A_IDLE,
	HOST_OTG_STATE_A_WAIT_VRISE,
	HOST_OTG_STATE_A_WAIT_BCON,
	HOST_OTG_STATE_A_HOST,
	HOST_OTG_STATE_A_SUSPEND,
	HOST_OTG_STATE_A_PERIPHERAL,
	HOST_OTG_STATE_A_VBUS_ERR,
	HOST_OTG_STATE_A_WAIT_VFALL,
	HOST_OTG_STATE_B_IDLE = 0x10,
	HOST_OTG_STATE_B_PERIPHERAL,
	HOST_OTG_STATE_B_WAIT_ACON,
	HOST_OTG_STATE_B_HOST,
	HOST_OTG_STATE_B_HOST_2,
	HOST_OTG_STATE_B_SRP_INT1,
	HOST_OTG_STATE_B_SRP_INT2,
	HOST_OTG_STATE_B_DISCHRG1,
	HOST_OTG_STATE_B_DISCHRG2,
	HOST_OTG_STATE_UNKNOWN,
};

enum HOST_EP0_STAGE {
	HOST_EP0_STAGE_IDLE,
	HOST_EP0_STAGE_SETUP,
	HOST_EP0_STAGE_IN,
	HOST_EP0_STAGE_OUT,
	HOST_EP0_STAGE_STATUSIN,
	HOST_EP0_STAGE_STATUSOUT,
	HOST_EP0_STAGE_ACK,
};

enum HOST_EP_STATE {
	HOST_EP_FREE,
	HOST_EP_ALLOCATED,
	HOST_EP_BUSY,
	HOST_EP_NOT_IMPLEMENTED
};

struct host_device {
	uint8_t devnum;
	uint8_t hub_port;
	unsigned int speed;
	struct host_device *parent;
	void *hc_priv;
	void *user_ext;
};

struct host_ep_t {
	struct usb_endpoint_descriptor desc;
	struct list_head req_list;
	void *user_ext;
	uint8_t *hc_priv;
};

struct host_usb_device {
	struct host_ep_t ep0_hep;
	struct host_ep_t *in_ep[16];
	struct host_ep_t *out_ep[16];
	struct host_device udev;
	struct usb_device *ld_udev;
};

struct host_ep {
	uint8_t name[255];
	uint8_t hw_ep_num;
	uint8_t hw_buffers;
	uint16_t hw_max_packet_size;
	uint8_t is_in_ep;
	void *channel;
	uint8_t usbe_ep_num;
	uint8_t type;
	uint8_t usb_packet_size;
	enum HOST_EP_STATE state;
	struct host_ep_t *scheduled_usb_h_ep;
	uint8_t ref_count;
};

struct host_iso_frams_desc {
	uint32_t length;
	uint32_t offset;
};

struct host_ep_priv {
	struct list_head node;
	struct host_ep *generic_hw_ep;
	struct host_ep *current_hw_ep;
	uint8_t ep_is_ready;
	uint8_t is_in;
	uint8_t type;
	uint8_t interval;
	uint8_t ep_num;
	uint8_t faddress;
	uint16_t max_packet_size;
	uint32_t frame;
	uint8_t hub_address;
	uint8_t port_address;
	uint8_t split;
	struct host_ep_t *usb_h_ep;
	uint8_t isoc_ep_configured;
	uint8_t transfer_finished;
};

#define HOST_EP_NUM                  16
#define TRANSFER_OUT                 0
#define TRANSFER_IN                  1

enum usb_request_type {
	usb_request_control = 0,
	usb_request_bulk,
	usb_request_int,
};

enum usb_data_direction {
	usb_data_in,
	usb_data_out,
	usb_no_data
};

struct host_req {
	enum usb_request_type		type;
	u32		frame_interval;
	u8        device_speed;
	u8        device_address;
	u32		maximum_packet_length;
	enum usb_request_type  transfer_direction;
	u8        ep_num;
	u32       transfer_result;
	u8        toggle;
	enum HOST_EP0_STAGE  stage;
	u8        is_in;
};

struct host_sysreq {
	uint32_t priv_data_size;
	uint32_t trb_mem_size;
};

struct host_ep_cfg {
	uint8_t buffering_value;
	uint16_t start_buf;
	uint16_t max_packet_size;
};

struct host_cfg {
	uintptr_t reg_base;
	uintptr_t phy_reg_base;
	struct host_ep_cfg ep_in[16];
	struct host_ep_cfg ep_out[16];
	uint8_t dmult_enabled;
	uint8_t memory_alignment;
	uint8_t dma_support;
	uint8_t is_embedded_host;
	void *trb_addr;
	uintptr_t trb_dma_addr;
};

struct host_ctrl;

struct host_obj {
	int32_t (*host_probe)(struct host_cfg *config, struct host_sysreq *sys_req);

	int32_t (*host_init)(struct host_ctrl *priv, struct host_cfg *config,
			struct dma_ctrl *dma_controller, bool is_vhub_host);

	void (*host_destroy)(struct host_ctrl *priv);

	void (*host_start)(struct host_ctrl *priv);

	void (*host_stop)(struct host_ctrl *priv);

	void (*host_isr)(struct host_ctrl *priv);

	int32_t (*host_ep_disable)(struct host_ctrl *priv, struct host_ep *ep);

	int32_t (*host_req_queue)(struct host_ctrl *priv, struct host_ep *req);

	int32_t (*host_req_dequeue)(struct host_ctrl *priv,
			struct host_req *req, uint32_t status);

	int32_t (*host_vhub_status_data)(struct host_ctrl *priv, uint8_t *status);

	int32_t (*host_vhub_control)(struct host_ctrl *priv,
			struct usb_ctrlrequest *setup, uint8_t *buff);

	int32_t (*host_get_device_pd)(struct host_ctrl *priv);

	int32_t (*host_ep_get_private_data_size)(struct host_ctrl *priv);
};

struct host_ctrl {
	struct device *dev;
	struct hw_regs	*regs;
	struct host_obj *host_drv;
	struct host_cfg host_cfg;
	struct host_ep in[16];
	struct host_ep out[16];
	uint32_t port_status;
	struct list_head ctrl_h_ep_queue;
	struct list_head iso_in_h_ep_queue[MAX_INSTANCE_EP_NUM];
	struct list_head iso_out_h_ep_queue[MAX_INSTANCE_EP_NUM];
	struct list_head int_in_h_ep_queue[MAX_INSTANCE_EP_NUM];
	struct list_head int_out_h_ep_queue[MAX_INSTANCE_EP_NUM];
	struct list_head bulk_in_h_ep_queue[MAX_INSTANCE_EP_NUM];
	struct list_head bulk_out_h_ep_queue[MAX_INSTANCE_EP_NUM];
	uint8_t hw_ep_in_count;
	uint8_t hw_ep_out_count;
	unsigned int speed;
	enum host_otg_state otg_state;
	enum HOST_EP0_STAGE ep0_state;
	uint8_t vbus_err_cnt;
	uint8_t is_remote_wakeup;
	uint8_t is_self_powered;
	uint8_t device_address;
	struct dma_obj *dma_drv;
	void *dma_controller;
	struct dma_cfg dma_cfg;
	uint8_t port_resetting;
	struct host_usb_device *host_devices_table[MAX_SUPPORTED_DEVICES];
	struct custom_regs *custom_regs;
	struct vhub_regs *vhub_regs;
};

struct host_obj *host_get_instance(void);

void print_desc_dev(struct usb_device *dev, u32 type);

void print_desc_buff(void *buffer, u32 type);

int32_t host_vhub_control(struct host_ctrl *priv,
		struct usb_ctrlrequest *setup, uint8_t *buff);

bool host_check_transfer_complete(struct host_ctrl  *priv,
		u8 ep_num, u8 is_in, u32 *transfer_result);

int host_request_program(struct host_ctrl *priv, struct host_req  *req);

int dev_descriptor(void *buf, struct usb_device *dev,
		struct devrequest *cmd, int txlen);

void hub_port_reset(struct host_ctrl *priv, uint8_t on);

void _host_start(struct host_ctrl *priv);

int hub_port_suspend(struct host_ctrl *priv, u16 on);

#endif
