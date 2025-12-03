// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <usb.h>
#include <linux/delay.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include "core.h"
#include "hw-regs.h"
#include "host_api.h"

#define USB2_2_BASE_ADDRESS 0x31800000

void print_desc_dev(struct usb_device *dev, u32 type)
{
	if (type == 0) {//device desc
		struct usb_device_descriptor *bt = &dev->descriptor;
	    printf("usb device desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bcdUSB = 0x%x\n", bt->bcdUSB);
	    printf("buffer->bDeviceClass = 0x%x\n", bt->bDeviceClass);
	    printf("buffer->bDeviceSubClass = 0x%x\n", bt->bDeviceSubClass);
	    printf("buffer->bDeviceProtocol = 0x%x\n", bt->bDeviceProtocol);
	    printf("buffer->bMaxPacketSize0 = 0x%x\n", bt->bMaxPacketSize0);
	    printf("buffer->idVendor = 0x%x\n", bt->idVendor);
	    printf("buffer->idProduct = 0x%x\n", bt->idProduct);
	    printf("buffer->bcdDevice = 0x%x\n", bt->bcdDevice);
	    printf("buffer->iManufacturer = 0x%x\n", bt->iManufacturer);
	    printf("buffer->iProduct = 0x%x\n", bt->iProduct);
	    printf("buffer->iSerialNumber = 0x%x\n", bt->iSerialNumber);
	    printf("buffer->bNumConfigurations = 0x%x\n", bt->bNumConfigurations);
	} else if (type == 1) {//config desc
		struct usb_config_descriptor *bt = &dev->config.desc;
	    printf("usb config desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->wTotalLength = 0x%x\n", bt->wTotalLength);
	    printf("buffer->bNumInterfaces = 0x%x\n", bt->bNumInterfaces);
	    printf("buffer->bConfigurationValue = 0x%x\n", bt->bConfigurationValue);
	    printf("buffer->iConfiguration = 0x%x\n", bt->iConfiguration);
	    printf("buffer->bmAttributes = 0x%x\n", bt->bmAttributes);
	    printf("buffer->MaxPower = 0x%x\n", bt->bMaxPower);
	} else if (type == 2) {//interface desc
		struct usb_interface_descriptor *bt = &dev->config.if_desc[0].desc;
	    printf("usb interface desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bInterfaceNumber = 0x%x\n", bt->bInterfaceNumber);
	    printf("buffer->bAlternateSetting = 0x%x\n", bt->bAlternateSetting);
	    printf("buffer->bNumEndpoints = 0x%x\n", bt->bNumEndpoints);
	    printf("buffer->bInterfaceClass = 0x%x\n", bt->bInterfaceClass);
	    printf("buffer->bInterfaceSubClass = 0x%x\n", bt->bInterfaceSubClass);
	    printf("buffer->bInterfaceProtocol = 0x%x\n", bt->bInterfaceProtocol);
	    printf("buffer->iInterface = 0x%x\n", bt->iInterface);
	} else if (type == 3) {//endpoint desc
		struct usb_endpoint_descriptor *bt = &dev->config.if_desc->ep_desc[0];
	    printf("usb endpoint desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bEndpointAddress = 0x%x\n", bt->bEndpointAddress);
	    printf("buffer->bmAttributes = 0x%x\n", bt->bmAttributes);
	    printf("buffer->wMaxPacketSize = 0x%x\n", bt->wMaxPacketSize);
	    printf("buffer->bInterval = 0x%x\n", bt->bInterval);
	} else {
		printf("unknown desc type!!!\n");
	}
}

void print_desc_buff(void *buffer, u32 type)
{
	if (type == 0) {//device desc
		struct usb_device_descriptor *bt = (struct usb_device_descriptor *)buffer;
	    printf("usb device desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bcdUSB = 0x%x\n", bt->bcdUSB);
	    printf("buffer->bDeviceClass = 0x%x\n", bt->bDeviceClass);
	    printf("buffer->bDeviceSubClass = 0x%x\n", bt->bDeviceSubClass);
	    printf("buffer->bDeviceProtocol = 0x%x\n", bt->bDeviceProtocol);
	    printf("buffer->bMaxPacketSize0 = 0x%x\n", bt->bMaxPacketSize0);
	    printf("buffer->idVendor = 0x%x\n", bt->idVendor);
	    printf("buffer->idProduct = 0x%x\n", bt->idProduct);
	    printf("buffer->bcdDevice = 0x%x\n", bt->bcdDevice);
	    printf("buffer->iManufacturer = 0x%x\n", bt->iManufacturer);
	    printf("buffer->iProduct = 0x%x\n", bt->iProduct);
	    printf("buffer->iSerialNumber = 0x%x\n", bt->iSerialNumber);
	    printf("buffer->bNumConfigurations = 0x%x\n", bt->bNumConfigurations);
	} else if (type == 1) {//config desc
		struct usb_config_descriptor *bt = (struct usb_config_descriptor *)buffer;
	    printf("usb config desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->wTotalLength = 0x%x\n", bt->wTotalLength);
	    printf("buffer->bNumInterfaces = 0x%x\n", bt->bNumInterfaces);
	    printf("buffer->bConfigurationValue = 0x%x\n", bt->bConfigurationValue);
	    printf("buffer->iConfiguration = 0x%x\n", bt->iConfiguration);
	    printf("buffer->bmAttributes = 0x%x\n", bt->bmAttributes);
	    printf("buffer->MaxPower = 0x%x\n", bt->bMaxPower);
	} else if (type == 2) {//interface desc
		struct usb_interface_descriptor *bt = (struct usb_interface_descriptor *)buffer;
	    printf("usb interface desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bInterfaceNumber = 0x%x\n", bt->bInterfaceNumber);
	    printf("buffer->bAlternateSetting = 0x%x\n", bt->bAlternateSetting);
	    printf("buffer->bNumEndpoints = 0x%x\n", bt->bNumEndpoints);
	    printf("buffer->bInterfaceClass = 0x%x\n", bt->bInterfaceClass);
	    printf("buffer->bInterfaceSubClass = 0x%x\n", bt->bInterfaceSubClass);
	    printf("buffer->bInterfaceProtocol = 0x%x\n", bt->bInterfaceProtocol);
	    printf("buffer->iInterface = 0x%x\n", bt->iInterface);
	} else if (type == 3) {//endpoint desc
		struct usb_endpoint_descriptor *bt = (struct usb_endpoint_descriptor *)buffer;
	    printf("usb endpoint desc------------------\n");
	    printf("buffer->bLength = 0x%x\n", bt->bLength);
	    printf("buffer->bDescriptorType = 0x%x\n", bt->bDescriptorType);
	    printf("buffer->bEndpointAddress = 0x%x\n", bt->bEndpointAddress);
	    printf("buffer->bmAttributes = 0x%x\n", bt->bmAttributes);
	    printf("buffer->wMaxPacketSize = 0x%x\n", bt->wMaxPacketSize);
	    printf("buffer->bInterval = 0x%x\n", bt->bInterval);
	} else {
		printf("unknown desc type!!!\n");
	}
}

static int platform_usb_irq(struct phytium_cusb *config)
{
	config->host_obj->host_isr(config->host_priv);

	return 0;
}

/* Direction: In ; request: status */
static int ft_otg_submit_rh_msg_in_status(struct ft_priv *priv,
		struct usb_device *dev, void *buffer, int txlen, struct devrequest *cmd)
{
	uint32_t port_status = 0;
	uint32_t port_change = 0;
	int len = 0;
	int stat = 0;
	struct host_ctrl *host_priv = (struct host_ctrl *)(priv->config->host_priv);

	platform_usb_irq(priv->config);

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint16_t *)buffer = cpu_to_le16(1);
		len = 2;
		break;
	case USB_RECIP_INTERFACE:
	case USB_RECIP_ENDPOINT:
		*(uint16_t *)buffer = cpu_to_le16(0);
		len = 2;
		break;
	case USB_TYPE_CLASS:
		*(uint32_t *)buffer = cpu_to_le32(0);
		len = 4;
		break;
	case USB_RECIP_OTHER | USB_TYPE_CLASS:
		if (host_priv->port_status & USB_PORT_STAT_CONNECTION)
			port_status |= USB_PORT_STAT_CONNECTION;

		if (host_priv->port_status & USB_PORT_STAT_ENABLE)
			port_status |= USB_PORT_STAT_ENABLE;

		if (host_priv->port_status & USB_PORT_STAT_SUSPEND)
			port_status |= USB_PORT_STAT_SUSPEND;

		if (host_priv->port_status & USB_PORT_STAT_OVERCURRENT)
			port_status |= USB_PORT_STAT_OVERCURRENT;

		if (host_priv->port_status & USB_PORT_STAT_RESET)
			port_status |= USB_PORT_STAT_RESET;

		if (host_priv->port_status & USB_PORT_STAT_POWER)
			port_status |= USB_PORT_STAT_POWER;

		if (host_priv->port_status & USB_PORT_STAT_LOW_SPEED)
			port_status |= USB_PORT_STAT_LOW_SPEED;

		if (host_priv->port_status & USB_PORT_STAT_HIGH_SPEED)
			port_status |= USB_PORT_STAT_HIGH_SPEED;


		if (host_priv->port_status & (USB_PORT_STAT_C_CONNECTION << 16))
			port_change |= USB_PORT_STAT_C_CONNECTION;

		if (host_priv->port_status & (USB_PORT_STAT_C_ENABLE << 16))
			port_change |= USB_PORT_STAT_C_ENABLE;

		if (host_priv->port_status & (USB_PORT_STAT_C_OVERCURRENT << 16))
			port_change |= USB_PORT_STAT_C_OVERCURRENT;

		if (host_priv->port_status & (USB_PORT_STAT_C_RESET << 16))
			port_change |= USB_PORT_STAT_C_RESET;

		debug("port_status = 0x%x, port_change = 0x%x\n", port_status, port_change);
		*(uint32_t *)buffer = cpu_to_le32(port_status | (port_change << 16));
		len = 4;
		break;
	default:
		puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In ; request: Descriptor */
static int ft_otg_submit_rh_msg_in_descriptor(struct usb_device *dev,
		void *buffer, int txlen, struct devrequest *cmd)
{
	dev_descriptor(buffer, dev, cmd, txlen);

	return 0;
}

/* Direction: In ; request: Configuration */
static int ft_otg_submit_rh_msg_in_configuration(struct usb_device *dev,
		void *buffer, int txlen, struct devrequest *cmd)
{
	int len = 0;
	int stat = 0;

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		*(uint8_t *)buffer = 0x01;
		len = 1;
		break;
	default:
		puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

/* Direction: In */
static int ft_otg_submit_rh_msg_in(struct ft_priv *priv, struct usb_device *dev,
		void *buffer, int txlen, struct devrequest *cmd)
{
	switch (cmd->request) {
	case USB_REQ_GET_STATUS:
		return ft_otg_submit_rh_msg_in_status(priv, dev, buffer,
						       txlen, cmd);
	case USB_REQ_GET_DESCRIPTOR:
		return ft_otg_submit_rh_msg_in_descriptor(dev, buffer,
							   txlen, cmd);
	case USB_REQ_GET_CONFIGURATION:
		return ft_otg_submit_rh_msg_in_configuration(dev, buffer,
							      txlen, cmd);
	default:
		puts("unsupported root hub command\n");
		return USB_ST_STALLED;
	}

	return 0;
}

/* Direction: Out */
static int ft_otg_submit_rh_msg_out(struct ft_priv *priv, struct usb_device *dev,
		void *buffer, int txlen, struct devrequest *cmd)
{
	struct host_ctrl *priv_t;
	struct phytium_cusb *config;

	config = priv->config;
	if (!config)
		return -EINVAL;

	priv_t = (struct host_ctrl *)config->host_priv;
	if (!priv)
		return -EINVAL;

	int len = 0;
	int stat = 0;
	uint16_t bmrtype_breq = cmd->requesttype | (cmd->request << 8);
	uint16_t wValue = cpu_to_le16(cmd->value);

	switch (bmrtype_breq & ~USB_DIR_IN) {
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_ENDPOINT:
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_TYPE_CLASS:
		break;
	case (USB_REQ_CLEAR_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue) {
		case USB_PORT_FEAT_CONNECTION:
			break;
		case USB_PORT_FEAT_ENABLE:
			break;
		case USB_PORT_FEAT_SUSPEND:
			hub_port_suspend(priv_t, 0);
			break;
		case USB_PORT_FEAT_OVER_CURRENT:
			break;
		case USB_PORT_FEAT_RESET:
			break;
		case USB_PORT_FEAT_L1:
			break;
		case USB_PORT_FEAT_POWER:
			break;
		case USB_PORT_FEAT_LOWSPEED:
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			break;
		case USB_PORT_FEAT_C_ENABLE:
			break;
		case USB_PORT_FEAT_C_SUSPEND:
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			break;
		case USB_PORT_FEAT_INDICATOR:
			break;
		case USB_PORT_FEAT_C_PORT_L1:
			break;
		default:
			break;
		}
		priv_t->port_status &= ~(1 << wValue);
		break;
	case (USB_REQ_SET_FEATURE << 8) | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue) {
		case USB_PORT_FEAT_SUSPEND:
			hub_port_suspend(priv_t, 1);
			break;
		case USB_PORT_FEAT_RESET:
			hub_port_reset(priv_t, 1);
		    mdelay(50);
		    priv_t->port_status |= 1 << USB_PORT_FEAT_ENABLE;
			break;
		case USB_PORT_FEAT_POWER:
			_host_start(priv_t);
			break;
		case USB_PORT_FEAT_ENABLE:
			break;
		}
		break;
	case (USB_REQ_SET_ADDRESS << 8):
		priv->root_hub_devnum = wValue;
		break;
	case (USB_REQ_SET_CONFIGURATION << 8):
		break;
	default:
		puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	len = min(len, txlen);
	dev->act_len = len;
	dev->status = stat;

	return stat;
}

static int ft_otg_submit_rh_msg(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *buffer, int txlen, struct devrequest *cmd)
{
	int stat = 0;

	if (usb_pipeint(pipe)) {
		puts("Root-Hub submit IRQ: NOT implemented\n");
		return 0;
	}

	if (cmd->requesttype & USB_DIR_IN)
		stat = ft_otg_submit_rh_msg_in(priv, dev, buffer, txlen, cmd);
	else
		stat = ft_otg_submit_rh_msg_out(priv, dev, buffer, txlen, cmd);

	return stat;
}

static int transfer_data_in(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *buffer, void *setup, int len)
{
	u32 status;
	u32 times;
	struct dma_trb     *trb_addr;
	struct dma_trb     *trb;
	struct host_req    req;
	struct phytium_cusb *config;
	struct host_ctrl *priv_t;
	u32 transfer_result;
	int devnum = usb_pipedevice(pipe);
	u8 device_address = devnum;
	u32 maximum_packet_length = 64;

	times = 500;
	status = 0;

	config = priv->config;
	if (!config)
		return -EINVAL;

	priv_t = (struct host_ctrl *)config->host_priv;
	if (!priv_t)
		return -EINVAL;

	if (setup == NULL)
		return -EINVAL;

	if ((buffer == NULL) || (len == 0))
		return -EINVAL;

	trb = (struct dma_trb *)malloc(sizeof(struct dma_trb) * 3);
	if (trb == NULL) {
		debug("Not enough memory!\n");
		return -ENOMEM;
	}

	memset(&req, 0, sizeof(struct host_req));
	//stage 1 : Send token setup
	req.type = usb_request_control;
	req.frame_interval = 0;
	req.device_speed = USB_SPEED_HIGH;
	req.device_address = device_address;
	req.maximum_packet_length = maximum_packet_length;
	req.ep_num = 0;
	req.stage = HOST_EP0_STAGE_SETUP;
	req.is_in = TRANSFER_OUT;
	host_request_program(priv_t, &req);
	//Dma config
	trb_addr = (struct dma_trb *)trb;
	trb_addr->dma_addr = (u32)(u64)(setup);
	trb_addr->dma_size = 8;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 tx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);

	//stage 2 : Data In
	trb_addr = (struct dma_trb *)(trb + 1);
	trb_addr->dma_addr = (u32)(u64)buffer;
	trb_addr->dma_size = len;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(2) | BIT(5);
	req.stage = HOST_EP0_STAGE_IN;
	req.is_in = TRANSFER_IN;
	host_request_program(priv_t, &req);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 rx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);

	//stage 3 : send ACK
	trb_addr = (struct dma_trb *) (trb + 2);
	trb_addr->dma_addr = (u32)(u64)setup;
	trb_addr->dma_size = 0;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	req.stage = HOST_EP0_STAGE_STATUSOUT;
	req.is_in = TRANSFER_OUT;
	host_request_program(priv_t, &req);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 rx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	transfer_result = USB_NOERROR;
	dev->act_len = len;
	dev->status = status;

proc_error:
	 priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);
	if (trb != NULL)
		free(trb);

	return 0;
}

static int transfer_data_out(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *buffer, void *setup, int len)
{
	u32 status;
	u32 times;
	struct dma_trb  *trb_addr;
	struct dma_trb  *trb;
	struct host_req req;
	struct phytium_cusb *config;
	struct host_ctrl *priv_t;
	u32 transfer_result;
	int devnum = usb_pipedevice(pipe);
	u8 device_address = devnum;
	u32 maximum_packet_length = 64;

	times = 500;
	status = 0;

	config = priv->config;
	if (!config)
		return -EINVAL;

	priv_t = (struct host_ctrl *)config->host_priv;
	if (!priv_t)
		return -EINVAL;

	if (setup == NULL)
		return -EINVAL;

	if ((buffer == NULL) || (len == 0))
		return -EINVAL;

	trb = (struct dma_trb *)malloc(sizeof(struct dma_trb) * 3);
	if (trb == NULL) {
		debug("Not enough memory!\n");
		return -ENOMEM;
	}

	//stage 1 : Send token setup
	req.type = usb_request_control;
	req.frame_interval = 0;
	req.device_speed = USB_SPEED_HIGH;
	req.device_address = device_address;
	req.maximum_packet_length = maximum_packet_length;
	req.ep_num = 0;
	req.stage = HOST_EP0_STAGE_SETUP;
	req.is_in = TRANSFER_OUT;
	host_request_program(priv_t, &req);
	//Dma config
	trb_addr = (struct dma_trb *)trb;
	trb_addr->dma_addr = (u32)(u64)(setup);
	trb_addr->dma_size = 8;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 tx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);

	//stage 2 : Data Out
	//send in packet
	trb_addr = (struct dma_trb *)(trb + 1);
	trb_addr->dma_addr = (u32)(u64)buffer;
	trb_addr->dma_size = len;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	req.stage = HOST_EP0_STAGE_OUT;
	req.is_in = TRANSFER_OUT;
	host_request_program(priv_t, &req);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 tx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	 priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);

	 //stage 3 : Rx ACK
	 trb_addr = (struct dma_trb *)(trb + 2);
	 trb_addr->dma_addr = (u32)(u64)(setup);
	 trb_addr->dma_size = 0;
	 trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	 req.stage = HOST_EP0_STAGE_STATUSIN;
	 req.is_in = TRANSFER_IN;
	 host_request_program(priv_t, &req);
	 priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 rx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	transfer_result = USB_NOERROR;
	dev->act_len = len;
	dev->status = status;

proc_error:
	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);
	if (trb != NULL)
		free(trb);

	return 0;
}

static int transfer_no_data(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *setup, int len)
{
	uint32_t status;
	uint32_t times;
	struct dma_trb *trb_addr;
	struct host_req req;
	struct phytium_cusb *config;
	struct host_ctrl *priv_t;
	u32 transfer_result;
	int devnum = usb_pipedevice(pipe);
	u8 device_address = devnum;
	u32 maximum_packet_length = 64;

	times = 500;
	status = 0;

	config = priv->config;
	if (!config)
		return -EINVAL;

	priv_t = (struct host_ctrl *)config->host_priv;
	if (!priv_t)
		return -EINVAL;

	if (setup == NULL)
		return -EINVAL;

	trb_addr = (struct dma_trb *)malloc(sizeof(struct dma_trb));
	if (!trb_addr) {
		debug("Not enough memory!\n");
		return -ENOMEM;
	}

	//stage 1 : Send token setup
	req.type = usb_request_control;
	req.frame_interval = 0;
	req.device_speed = USB_SPEED_HIGH;
	req.device_address = device_address;
	req.maximum_packet_length = maximum_packet_length;
	req.ep_num = 0;
	req.stage = HOST_EP0_STAGE_SETUP;
	req.is_in = TRANSFER_OUT;
	host_request_program(priv_t, &req);
	//Dma config
	trb_addr->dma_addr = (u32)(u64) (setup);
	trb_addr->dma_size = 8;
	//trb_addr->Ctrl = BIT10 | BIT0 | BIT1 | BIT2 | BIT5;
	trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 tx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}

	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);
	//stage 2 : ACK
	//send in packet
	trb_addr->dma_size = 0;
	req.stage = HOST_EP0_STAGE_IN;
	req.is_in = HOST_EP0_STAGE_STATUSIN;
	host_request_program(priv_t, &req);
	priv_t->dma_drv->dma_channel_program(priv_t->dma_controller,
		req.ep_num, req.is_in, (u32)(u64)trb_addr);
	//wait for ep0 rx irq
	while (times != 0) {
		mdelay(1);
		if (host_check_transfer_complete(priv_t, req.ep_num,
				req.is_in, &transfer_result)) {
			if ((transfer_result) != USB_NOERROR) {
				dev->status = USB_ST_BIT_ERR;
				goto proc_error;
			} else {
				break;
			}
		}
		times--;
		if (times == 0) {
			transfer_result = USB_ERR_TIMEOUT;
			dev->status = USB_ST_CRC_ERR;
			goto proc_error;
		}
	}
	transfer_result = USB_NOERROR;
	dev->act_len = len;
	dev->status = status;

proc_error:
	priv_t->dma_drv->dma_channel_release(priv_t->dma_controller,
		req.ep_num, req.is_in);
	if (trb_addr != NULL)
		free(trb_addr);

	return status;
}

static void cal_dma_packet_num(u32 size, u32 *num, u32 *len)
{
	u32  num_temp;
	u32  len_temp;

	len_temp = size % SIZE_PER_DMA_PACKET;
	num_temp = size / SIZE_PER_DMA_PACKET;
	if (len_temp == 0) {
		*num = num_temp;
		*len = SIZE_PER_DMA_PACKET;
	} else {
		*num = num_temp + 1;
		*len = len_temp;
	}
	debug("DMA Packet Num : %d, Last Packet Length : %d\n", *num, *len);
}

static uint8_t host_get_toggle(struct host_ctrl *priv, u8 ep_num)
{
	u8  toggle;

	toggle = (phytium_read8(&priv->regs->endprst) & BIT(7)) >> 7;

	return toggle;
}

static int phytium_host_bulk_transfer_data_out(struct usb_device *dev,
		struct host_ctrl *priv, u8 device_address, u8 ep_num,
		u32 maximum_packet_length, void *data, u32 *data_length,
		u8 *data_toggle)
{
	int status;
	u32 times;
	struct dma_trb *trb;
	struct dma_trb *trb_addr;
	u32 index;
	u32 p_num;
	u32 l_size;
	struct host_req	 req;
	void *buffer;
	u32 transfer_result;

	if ((data_length == NULL) || (*data_length == 0) || (data == NULL))
		return -EINVAL;

	status = 0;
	times = 3000000;
	cal_dma_packet_num(*data_length, &p_num, &l_size);
	trb = (struct dma_trb *)malloc(sizeof(struct dma_trb) * p_num);
	if (trb == NULL) {
		debug("Not enough memory!\n");
		return -EINVAL;
	}

	buffer = malloc(*data_length);
	if (buffer == NULL) {
		debug("Not enough memory!\n");
		return -EINVAL;
	}

	memcpy(buffer, data, *data_length);
	//Host Config
	req.type = usb_request_bulk;
	req.frame_interval = 0;
	req.device_address = device_address;
	req.maximum_packet_length = maximum_packet_length;
	req.ep_num = ep_num;
	req.is_in = TRANSFER_OUT;
	req.toggle = *data_toggle;
	host_request_program(priv, &req);
	//DMA transfer
	debug("%s, p_num = %d\n", __func__, p_num);
	for (index = 0; index < p_num; index++) {
		trb_addr = (struct dma_trb *) (trb + index);
		trb_addr->dma_addr = (u32)(u64)(buffer) + index * SIZE_PER_DMA_PACKET;
		if (index < (p_num - 1))
			trb_addr->dma_size = SIZE_PER_DMA_PACKET;
		else
			trb_addr->dma_size = l_size;

		trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
		priv->dma_drv->dma_channel_program(priv->dma_controller,
			req.ep_num, req.is_in, (u32)(u64)trb_addr);
		//Wait Transfer Complete
		while (times != 0) {
			udelay(1);
			if (host_check_transfer_complete(priv, req.ep_num,
					req.is_in, &transfer_result)) {
				if ((transfer_result) != USB_NOERROR) {
					status = -1;
					goto proc_error;
				} else {
					break;
				}
			}
			times--;
			if (times == 0) {
				transfer_result = USB_ERR_TIMEOUT;
				status = -2;
				goto proc_error;
			}
		}
		priv->dma_drv->dma_channel_release(priv->dma_controller,
			req.ep_num, req.is_in);
	}
	transfer_result = USB_NOERROR;
	dev->status = 0;
	//dev->act_len = *data_length;

proc_error:
	//Update data_toggle
	*data_toggle = host_get_toggle(priv, req.ep_num);
	//Release DMA Channel
	priv->dma_drv->dma_channel_release(priv->dma_controller, req.ep_num, req.is_in);
	if (trb != NULL)
		free(trb);

	if (buffer != NULL)
		free(buffer);

	return status;
}

static int phytium_host_bulk_transfer_data_in(struct usb_device *dev,
		struct host_ctrl *priv, u8 device_address, u8 ep_num,
		u32	maximum_packet_length, void *data, u32 *data_length,
		u8 *data_toggle)
{
	int status;
	u32 times;
	struct dma_trb *trb;
	struct dma_trb *trb_addr;
	u32 index;
	u32 p_num;
	u32 l_size;
	struct host_req	 req;
	void *buffer;
	u32 transfer_result;

	if ((data_length == NULL) || (*data_length == 0) || (data == NULL))
		return -EINVAL;

	status = 0;
	times = 5000000;
	cal_dma_packet_num(*data_length, &p_num, &l_size);
	trb = (struct dma_trb *)malloc(sizeof(struct dma_trb) * p_num);
	if (trb == NULL) {
		debug("Not enough memory!\n");
		return -ENOMEM;
	}

	buffer = malloc(*data_length);
	if (buffer == NULL) {
		debug("Not enough memory!\n");
		return -ENOMEM;
	}

	//Host Config
	req.type = usb_request_bulk;
	req.frame_interval = 0;
	req.device_address = device_address;
	req.maximum_packet_length = maximum_packet_length;
	req.ep_num = ep_num;
	req.is_in = TRANSFER_IN;
	req.toggle = *data_toggle;
	host_request_program(priv, &req);
	//Dma Config
	debug("%s, p_num = %d\n", __func__, p_num);
	for (index = 0; index < p_num; index++) {
		trb_addr = (struct dma_trb *) (trb + index);
		trb_addr->dma_addr = (u32)(u64)(buffer) + index * SIZE_PER_DMA_PACKET;
		if (index < (p_num - 1))
			trb_addr->dma_size = SIZE_PER_DMA_PACKET;
		else
			trb_addr->dma_size = l_size;

		trb_addr->ctrl = BIT(10) | BIT(0) | BIT(5);
		priv->dma_drv->dma_channel_program(priv->dma_controller,
			req.ep_num, req.is_in, (u32)(u64)trb_addr);

		//Wait Transfer Complete
		while (times != 0) {
			udelay(1);
			if (host_check_transfer_complete(priv, req.ep_num,
					req.is_in, &transfer_result)) {
				if ((transfer_result) != USB_NOERROR) {
					status = -1;
					goto proc_error;
				} else {
					break;
				}
			}
			times--;
			if (times == 0) {
				transfer_result = USB_ERR_TIMEOUT;
				status = -2;
				goto proc_error;
			}
		}
		priv->dma_drv->dma_channel_release(priv->dma_controller,
			req.ep_num, req.is_in);
	}
	//Release DMA Channel
	transfer_result = USB_NOERROR;
	memcpy(data, buffer, *data_length);
	dev->status = 0;
	//dev->act_len = *data_length;

proc_error:
	//Update data_toggle
	*data_toggle = host_get_toggle(priv, req.ep_num);
	//Release DMA Channel
	priv->dma_drv->dma_channel_release(priv->dma_controller,
		req.ep_num, req.is_in);
	if (trb != NULL)
		free(trb);

	if (buffer != NULL)
		free(buffer);

	return status;
}

 /* U-Boot USB transmission interface */
static int _submit_bulk_msg(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *buffer, int len)
{
	int devnum = usb_pipedevice(pipe);
	int ep = usb_pipeendpoint(pipe);
	int transfer_direction  = usb_pipein(pipe);
	int status;
	struct phytium_cusb *config;
	struct host_ctrl *priv_t;
	struct usb_config *usb_config;
	struct usb_interface *ifdesc;
	struct usb_endpoint_descriptor *epdesc;
	int i, ii;
	u8 data_toggle;

	if ((devnum >= MAX_SUPPORTED_DEVICES) || (devnum == priv->root_hub_devnum)) {
		dev->status = 0;
		return -EINVAL;
	}

	if ((len == 0) || (buffer == NULL))
		return -EINVAL;

	config = priv->config;
	if (!config)
		return -EINVAL;

	priv_t = (struct host_ctrl *)config->host_priv;
	if (!priv_t)
		return -EINVAL;

	usb_config = &dev->config;
	for (i = 0; i < usb_config->no_of_if; i++) {
		ifdesc = &usb_config->if_desc[i];
		for (ii = 0; ii < ifdesc->no_of_ep; ii++)
			epdesc = &ifdesc->ep_desc[ii];
	}

	if (transfer_direction) {
		data_toggle = priv->in_data_toggle[devnum][ep];
		status = phytium_host_bulk_transfer_data_in(dev, priv_t, devnum,
			ep, epdesc->wMaxPacketSize, buffer, &len, &data_toggle);
		priv->in_data_toggle[devnum][ep] = data_toggle;
	} else {
		data_toggle = priv->out_data_toggle[devnum][ep];
		status = phytium_host_bulk_transfer_data_out(dev, priv_t, devnum,
			ep, epdesc->wMaxPacketSize, buffer, &len, &data_toggle);
		priv->out_data_toggle[devnum][ep] = data_toggle;
	}

	return status;
}

static int _submit_control_msg(struct ft_priv *priv, struct usb_device *dev,
		unsigned long pipe, void *buffer, int len, struct devrequest *setup)
{
	int devnum = usb_pipedevice(pipe);
	int ret;

	if (devnum == priv->root_hub_devnum) {
		dev->status = 0;
		dev->speed = USB_SPEED_HIGH;

		return ft_otg_submit_rh_msg(priv, dev, pipe, buffer, len, setup);
	}

	if (buffer) {
		if (setup->requesttype & USB_DIR_IN)
			ret = transfer_data_in(priv, dev, pipe, buffer, setup, len);
		else if (setup->requesttype & USB_DIR_OUT)
			ret = transfer_data_out(priv, dev, pipe, buffer, setup, len);
		else
			printf("ERROR:transfer direction error!\n");
	} else {
		transfer_no_data(priv, dev, pipe, setup, len);
	}

	return 0;
}

static int ft_submit_control_msg(struct udevice *dev, struct usb_device *udev,
		unsigned long pipe, void *buffer, int length, struct devrequest *setup)
{
	struct ft_priv *priv = dev_get_priv(dev);

	debug("%s: dev='%s', udev=%p, udev->dev='%s', portnr=%d\n", __func__,
		dev->name, udev, udev->dev->name, udev->portnr);

	return _submit_control_msg(priv, udev, pipe, buffer, length, setup);
}

static int ft_submit_bulk_msg(struct udevice *dev, struct usb_device *udev,
		unsigned long pipe, void *buffer, int length)
{
	struct ft_priv *priv = dev_get_priv(dev);

	return _submit_bulk_msg(priv, udev, pipe, buffer, length);
}

static int ft_usb2_of_to_plat(struct udevice *dev)
{
	struct ft_priv *priv = dev_get_priv(dev);

	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs)
		return -EINVAL;

	priv->phy_regs = devfdt_get_addr_index_ptr(dev, 1);
	if (!priv->regs)
		return -EINVAL;

	debug("regs = 0x%p, phy_regs = 0x%p\n", priv->regs, priv->phy_regs);
	return 0;
}

static int ft_usb2_probe(struct udevice *dev)
{
	struct ft_priv *priv = dev_get_priv(dev);
	struct usb_bus_priv *bus_priv = dev_get_uclass_priv(dev);

	priv->config = malloc(sizeof(struct phytium_cusb));
	bus_priv->desc_before_addr = true;

	priv->config->is_vhub_host = false;
	priv->config->irq = 46;
	priv->config->regs = priv->regs;
	priv->config->phy_regs = priv->phy_regs;
	priv->config->dr_mode = usb_get_dr_mode(dev_ofnode(dev));
	debug("\nusb2 dr_mode =%d\n", priv->config->dr_mode);
	for (int i = 0; i < MAX_DEVICE; i++) {
		for (int j = 0; j < MAX_ENDPOINT; j++) {
			priv->in_data_toggle[i][j] = 0;
			priv->out_data_toggle[i][j] = 0;
		}
	}

	phytium_core_reset(priv->config, false);

	if (priv->config->dr_mode == USB_DR_MODE_HOST ||
	    priv->config->dr_mode == USB_DR_MODE_OTG) {
		if ((uintptr_t)(priv->regs) == USB2_2_BASE_ADDRESS)
			priv->config->is_vhub_host = true;

		phytium_host_init(priv->config);
	}

	return 0;
}

static int ft_usb2_remove(struct udevice *dev)
{
	struct ft_priv *priv = dev_get_priv(dev);

	priv->config->host_obj->host_stop(priv->config->host_priv);

	return 0;
}

struct dm_usb_ops ft_usb2_ops = {
	.control = ft_submit_control_msg,
	.bulk = ft_submit_bulk_msg,
	.interrupt = NULL,
};

static const struct udevice_id ft_usb2_ids[] = {
	{ .compatible = "phytium,phytium-usb2" },
	{ }
};

U_BOOT_DRIVER(usb2_ft) = {
	.name	= "phytium-usb2",
	.id	= UCLASS_USB,
	.of_match = ft_usb2_ids,
	.of_to_plat = ft_usb2_of_to_plat,
	.probe	= ft_usb2_probe,
	.remove = ft_usb2_remove,
	.ops	= &ft_usb2_ops,
	.priv_auto	= sizeof(struct ft_priv),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};
