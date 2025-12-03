// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#include <dm/device.h>
#include <usbroothubdes.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include "core.h"
#include "dma.h"
#include "hw-regs.h"
#include "host_api.h"

#define DRV_NAME "phytium_usb"

#define HOST_GENERIC_EP_CONTROLL	0x00
#define HOST_GENERIC_EP_ISOC		0x01
#define HOST_GENERIC_EP_BULK		0x02
#define HOST_GENERIC_EP_INT			0x03

#define HOST_ESTALL		1
#define HOST_EUNHANDLED	2
#define HOST_EAUTOACK	3
#define HOST_ESHUTDOWN	4

#define HOST_EP_NUM		16

#define HUB_CLASS_REQ(dir, type, request) ((((dir) | (type)) << 8) | (request))
/* GetBusState and SetHubDescriptor are optional, omitted */
#define CLEAR_HUB_FEATURE     HUB_CLASS_REQ(USB_DIR_OUT, USB_RT_HUB, USB_REQ_CLEAR_FEATURE)
#define CLEAR_PORT_FEATURE    HUB_CLASS_REQ(USB_DIR_OUT, USB_RT_PORT, USB_REQ_CLEAR_FEATURE)
#define GET_HUB_DESCRIPTOR    HUB_CLASS_REQ(USB_DIR_IN, USB_RT_HUB, USB_REQ_GET_DESCRIPTOR)
#define GET_HUB_STATUS        HUB_CLASS_REQ(USB_DIR_IN, USB_RT_HUB, USB_REQ_GET_STATUS)
#define GET_PORT_STATUS       HUB_CLASS_REQ(USB_DIR_IN, USB_RT_PORT, USB_REQ_GET_STATUS)
#define SET_HUB_FEATURE       HUB_CLASS_REQ(USB_DIR_OUT, USB_RT_HUB, USB_REQ_SET_FEATURE)
#define SET_PORT_FEATURE      HUB_CLASS_REQ(USB_DIR_OUT, USB_RT_PORT, USB_REQ_SET_FEATURE)

/* roothub.b masks */
#define RH_B_DR		0x0000ffff	/* device removable flags */
#define RH_B_PPCM	0xffff0000	/* port power control mask */

/* roothub.a masks */
#define RH_A_NDP	(0xff << 0)	/* number of downstream ports */
#define RH_A_PSM	(1 << 8)	/* power switching mode */
#define RH_A_NPS	(1 << 9)	/* no power switching */
#define RH_A_DT		(1 << 10)	/* device type (mbz) */
#define RH_A_OCPM	(1 << 11)	/* over current protection mode */
#define RH_A_NOCP	(1 << 12)	/* no over current protection */
#define RH_A_POTPGT	(0xff << 24)	/* power on to power good time */

static void host_set_vbus(struct host_ctrl *priv, uint8_t is_on)
{
	uint8_t otgctrl = phytium_read8(&priv->regs->otgctrl);

	if (is_on) {
		if (!(otgctrl & OTGCTRL_BUSREQ) || (otgctrl & OTGCTRL_ABUSDROP)) {
			otgctrl &= ~OTGCTRL_ABUSDROP;
			otgctrl |= OTGCTRL_BUSREQ;
			phytium_write8(&priv->regs->otgctrl, otgctrl);
		}
		priv->otg_state = HOST_OTG_STATE_A_WAIT_BCON;
	} else {
		if ((otgctrl & OTGCTRL_BUSREQ) || (otgctrl & OTGCTRL_ABUSDROP)) {
			otgctrl |= OTGCTRL_ABUSDROP;
			otgctrl &= ~OTGCTRL_BUSREQ;
			phytium_write8(&priv->regs->otgctrl, otgctrl);
		}
		priv->otg_state = HOST_OTG_STATE_A_IDLE;
	}
}

static inline void connect_host_detect(struct host_ctrl *priv, uint8_t otg_state)
{
	uint32_t gen_cfg;

	if (!priv)
		return;

	if (priv->custom_regs) {
		phytium_write32(&priv->custom_regs->wakeup, 0);
	} else {
		gen_cfg = phytium_read32(&priv->vhub_regs->gen_cfg);
		gen_cfg &= ~BIT(1);
		phytium_write32(&priv->vhub_regs->gen_cfg, gen_cfg);
	}

	phytium_write8(&priv->regs->otgirq, OTGIRQ_CONIRQ);

	if ((otg_state != HOST_OTG_STATE_A_HOST) && (otg_state != HOST_OTG_STATE_B_HOST))
		return;

	if ((priv->otg_state == HOST_OTG_STATE_A_PERIPHERAL)
			|| (priv->otg_state == HOST_OTG_STATE_B_PERIPHERAL))
		priv->otg_state = otg_state;

	priv->ep0_state = HOST_EP0_STAGE_IDLE;

	priv->port_status &= ~(USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED |
			USB_PORT_STAT_ENABLE);

	priv->port_status |= USB_PORT_STAT_C_CONNECTION | (USB_PORT_STAT_C_CONNECTION << 16);
	priv->port_resetting = 1;
	host_set_vbus(priv, 1);

	switch (phytium_read8(&priv->regs->speedctrl)) {
	case SPEEDCTRL_HS:
		priv->port_status |= USB_PORT_STAT_HIGH_SPEED;
		debug("detect High speed device\n");
		break;
	case SPEEDCTRL_FS:
		priv->port_status &= ~(USB_PORT_STAT_HIGH_SPEED | USB_PORT_STAT_LOW_SPEED);
		debug("detect Full speed device\n");
		break;
	case SPEEDCTRL_LS:
		priv->port_status |= USB_PORT_STAT_LOW_SPEED;
		debug("detect Low speed device\n");
		break;
	}

	priv->vbus_err_cnt = 0;
	priv->otg_state = otg_state;
}

static inline void a_Idle_detect(struct host_ctrl *priv, uint8_t otg_state)
{
	uint8_t otgctrl;

	if (!priv)
		return;

	phytium_write8(&priv->regs->otgirq, OTGIRQ_IDLEIRQ);

	if (otg_state != HOST_OTG_STATE_A_IDLE) {
		pr_info("IRQ OTG: A_IDLE Babble\n");
		return;
	}

	priv->port_status = 0;
	otgctrl = phytium_read8(&priv->regs->otgctrl);
	otgctrl &= ~OTGCTRL_ASETBHNPEN;
	phytium_write8(&priv->regs->otgctrl, otgctrl);

	host_set_vbus(priv, 1);

	priv->otg_state = HOST_OTG_STATE_A_IDLE;
}

static inline void b_Idle_detect(struct host_ctrl *priv, uint8_t otg_state)
{
	uint8_t otgctrl, usbcs;

	if (!priv)
		return;

	phytium_write8(&priv->regs->otgirq, OTGIRQ_IDLEIRQ);

	if (otg_state != HOST_OTG_STATE_B_IDLE) {
		pr_info("IRQ OTG: B_IDLE Babble\n");
		return;
	}

	otgctrl = phytium_read8(&priv->regs->otgctrl);
	otgctrl &= ~OTGCTRL_ASETBHNPEN;
	phytium_write8(&priv->regs->otgctrl, otgctrl);

	host_set_vbus(priv, 0);

	priv->otg_state = HOST_OTG_STATE_B_IDLE;

	usbcs = phytium_read8(&priv->regs->usbcs);
	usbcs &= ~USBCS_DISCON;
	phytium_write8(&priv->regs->usbcs, usbcs);
}

static inline void disconnect_host_detect(struct host_ctrl *priv)
{
	uint8_t otgctrl, otg_state;
	uint32_t gen_cfg;

	if (!priv)
		return;

	otgctrl = phytium_read8(&priv->regs->otgctrl);
	if ((otgctrl & OTGCTRL_ASETBHNPEN) && priv->otg_state == HOST_OTG_STATE_A_SUSPEND)
		pr_info("Device no Response\n");

	phytium_write8(&priv->regs->otgirq, OTGIRQ_CONIRQ);
retry:
	otg_state = phytium_read8(&priv->regs->otgstate);
	if ((otg_state == HOST_OTG_STATE_A_HOST || otg_state == HOST_OTG_STATE_B_HOST)) {
		pr_info("IRQ OTG: DisconnIrq Babble\n");
		goto retry;
	}

	phytium_write8(&priv->regs->endprst, ENDPRST_IO_TX);
	phytium_write8(&priv->regs->endprst, ENDPRST_FIFORST | ENDPRST_TOGRST | ENDPRST_IO_TX);
	phytium_write8(&priv->regs->endprst, ENDPRST_FIFORST | ENDPRST_TOGRST);
	phytium_write8(&priv->regs->ep0fifoctrl, FIFOCTRL_FIFOAUTO | 0 | 0x04);
	phytium_write8(&priv->regs->ep0fifoctrl, FIFOCTRL_FIFOAUTO | FIFOCTRL_IO_TX | 0 | 0x04);

	priv->port_status = USB_PORT_STAT_POWER;
	priv->port_status |= USB_PORT_STAT_C_CONNECTION << 16;

	if (priv->otg_state == HOST_OTG_STATE_A_SUSPEND)
		host_set_vbus(priv, 1);

	priv->otg_state = HOST_OTG_STATE_A_IDLE;

	if (priv->custom_regs) {
		phytium_write32(&priv->custom_regs->wakeup, 1);
	} else {
		gen_cfg = phytium_read32(&priv->vhub_regs->gen_cfg);
		gen_cfg |= BIT(1);
		phytium_write32(&priv->vhub_regs->gen_cfg, gen_cfg);
	}
}

static void host_otg_irq(struct host_ctrl *priv)
{
	uint8_t otgirq, otgien;
	uint8_t otgstatus, otg_state;
	uint8_t otgctrl;

	if (!priv)
		return;

	otgirq = phytium_read8(&priv->regs->otgirq);
	otgien = phytium_read8(&priv->regs->otgien);
	otgstatus = phytium_read8(&priv->regs->otgstatus);
	otg_state = phytium_read8(&priv->regs->otgstate);
	debug("otgirq:0x%x otgien:0x%x\n", otgirq, otgien);
	otgirq &= otgien;

	if (!otgirq)
		return;

	if (otgirq & OTGIRQ_BSE0SRPIRQ) {
		otgirq &= ~OTGIRQ_BSE0SRPIRQ;
		phytium_write8(&priv->regs->otgirq, OTGIRQ_BSE0SRPIRQ);

		otgctrl = phytium_read8(&priv->regs->otgctrl);
		otgctrl &= ~OTGIRQ_BSE0SRPIRQ;
		phytium_write8(&priv->regs->otgctrl, otgctrl);
	}

	if (otgirq & OTGIRQ_SRPDETIRQ) {
		otgirq &= ~OTGIRQ_SRPDETIRQ;
		phytium_write8(&priv->regs->otgirq, OTGIRQ_SRPDETIRQ);

		otgctrl = phytium_read8(&priv->regs->otgctrl);
		otgctrl &= ~OTGIRQ_SRPDETIRQ;
		phytium_write8(&priv->regs->otgctrl, otgctrl);
	}

	if (otgirq & OTGIRQ_VBUSERRIRQ) {
		otgirq &= ~OTGIRQ_VBUSERRIRQ;
		phytium_write8(&priv->regs->otgirq, OTGIRQ_VBUSERRIRQ);

		if (otg_state != HOST_OTG_STATE_A_VBUS_ERR) {
			debug("IRQ OTG: VBUS ERROR Babble\n");
			return;
		}

		host_set_vbus(priv, 0);
		priv->otg_state = HOST_OTG_STATE_A_VBUS_ERR;
		if (priv->port_status & USB_PORT_STAT_CONNECTION) {
			priv->port_status = USB_PORT_STAT_POWER | (USB_PORT_STAT_C_CONNECTION << 16);
			return;
		}

		if (priv->vbus_err_cnt >= 3) {
			priv->vbus_err_cnt = 0;
			debug("%s %d VBUS OVER CURRENT\n", __func__, __LINE__);
			priv->port_status |= USB_PORT_STAT_OVERCURRENT |
				(USB_PORT_STAT_C_OVERCURRENT << 16);

			phytium_write8(&priv->regs->otgirq, OTGIRQ_IDLEIRQ);
		} else {
			priv->vbus_err_cnt++;
			host_set_vbus(priv, 1);
			phytium_write8(&priv->regs->otgirq, OTGIRQ_IDLEIRQ);
		}
	}

	if (otgirq & OTGIRQ_CONIRQ) {
		if (priv->otg_state == HOST_OTG_STATE_A_HOST ||
				priv->otg_state == HOST_OTG_STATE_B_HOST ||
				priv->otg_state == HOST_OTG_STATE_A_SUSPEND) {
			if (otg_state == HOST_OTG_STATE_A_WAIT_VFALL ||
					otg_state == HOST_OTG_STATE_A_WAIT_BCON ||
					otg_state == HOST_OTG_STATE_A_SUSPEND)
				disconnect_host_detect(priv);
		} else if (priv->otg_state != HOST_OTG_STATE_A_HOST &&
				priv->otg_state != HOST_OTG_STATE_B_HOST &&
				priv->otg_state != HOST_OTG_STATE_A_SUSPEND)
			connect_host_detect(priv, otg_state);

		phytium_write8(&priv->regs->otgirq, OTGIRQ_CONIRQ);
	}

	if (otgirq & OTGIRQ_IDLEIRQ) {
		if (!(otgstatus & OTGSTATUS_ID))
			a_Idle_detect(priv, otg_state);
		else
			b_Idle_detect(priv, otg_state);
	}

	phytium_write8(&priv->regs->otgirq, OTGIRQ_IDCHANGEIRQ | OTGIRQ_SRPDETIRQ);
}

static uint32_t decode_error_code(uint8_t code)
{
	uint32_t status = 0;

	switch (code) {
	case ERR_NONE:
		status = 0;
		break;
	case ERR_CRC:
		printf("CRC Error\n");
		status = HOST_ESHUTDOWN;
		break;
	case ERR_DATA_TOGGLE_MISMATCH:
		printf("Toggle MisMatch Error\n");
		status = HOST_ESHUTDOWN;
		break;
	case ERR_STALL:
		printf("Stall Error\n");
		status = HOST_ESTALL;
		break;
	case ERR_TIMEOUT:
		printf("Timeout Error\n");
		status = HOST_ESHUTDOWN;
		break;
	case ERR_PID:
		printf("PID Error\n");
		status = HOST_ESHUTDOWN;
		break;
	case ERR_TOO_LONG_PACKET:
		printf("TOO_LONG_PACKET Error\n");
		status = HOST_ESHUTDOWN;
		break;
	case ERR_DATA_UNDERRUN:
		printf("UNDERRUN Error\n");
		status = HOST_ESHUTDOWN;
		break;
	default:
		printf("UNKNOWN Error\n");
		break;
	}

	return status;
}

int host_request_program(struct host_ctrl *priv, struct host_req  *req)
{
	u8  reg_con;
	u8  ep_num;
	u8  device_address;
	u16 maximum_packet_length;

	if ((priv == NULL) || (req == NULL))
		return -EINVAL;

	mdelay(1);

	maximum_packet_length = (u16)req->maximum_packet_length;
	device_address = req->device_address;
	ep_num = req->ep_num;
	reg_con = 0;
	switch (req->type) {
	case usb_request_control:
		debug("%s, usb_request_control\n", __func__);
		reg_con |= 0x0;
		break;
	case usb_request_bulk:
		debug("%s, usb_request_bulk\n", __func__);
		reg_con |= 0x8;
		break;
	case usb_request_int:
		debug("%s, usb_request_int\n", __func__);
		reg_con |= 0xC;
		break;
	}

	if (req->is_in) {  /*In Direction*/
		if (req->ep_num) {  /*Ep 1-15*/
			//Enable Rx Interrupt
			phytium_write16(&priv->regs->rxien,
				phytium_read16(&priv->regs->rxien) | BIT(req->ep_num));
			phytium_write16(&priv->regs->rxerrien,
				phytium_read16(&priv->regs->rxerrien) | BIT(req->ep_num));
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcon, reg_con);
			//Function Address
			phytium_write8(&priv->regs->fnaddr, (u8) device_address);
			//set in
			phytium_write8(&priv->regs->endprst, ep_num);
			//reset toggle and fifo
			if (req->toggle)
				phytium_write8(&priv->regs->endprst, BIT(7) | BIT(6) | ep_num);
			else
				phytium_write8(&priv->regs->endprst, BIT(6) | BIT(5) | ep_num);

			phytium_write8(&priv->regs->fifoctrl, BIT(5) | ep_num);
			//coutn switch ep_num
			phytium_write8(&priv->regs->ep_ext[ep_num - 1].rxctrl, ep_num);
			//Max packet size
			phytium_write16(&priv->regs->rxmaxpack[ep_num - 1], maximum_packet_length);
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcs, 1);
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcs, 1);
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcs, 1);
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcs, 1);
			//Transfer mode. Enable Ep
			phytium_write8(&priv->regs->ep[ep_num - 1].rxcon, reg_con | BIT(7));
			debug("rx_con:0x%02x\n", phytium_read8(&priv->regs->ep[ep_num - 1].rxcon));
			if (req->type == usb_request_int)
				phytium_write8(&priv->regs->irqmode[ep_num - 1].inirqmode,
					phytium_read8(&priv->regs->irqmode[ep_num - 1].inirqmode) | BIT(7));

			//Int Transfer Interval
			if (req->frame_interval != 0) {
				debug("Interval : %d\n", req->frame_interval);
				phytium_write8(&priv->regs->rxsoftimer[ep_num].ctrl,
					phytium_read8(&priv->regs->rxsoftimer[ep_num].ctrl) | BIT(1));
				phytium_write16(&priv->regs->rxsoftimer[ep_num].timer, req->frame_interval);
				phytium_write8(&priv->regs->rxsoftimer[ep_num].ctrl, 0x83);
			}
		} else {  /*Ep 0*/
			//Enable Rx Interrupt
			phytium_write16(&priv->regs->rxien,
				phytium_read16(&priv->regs->rxien) | BIT(0));
			phytium_write16(&priv->regs->rxerrien,
				phytium_read16(&priv->regs->rxerrien) | BIT(0));
			//Set Ep0 Max Packet Number
			phytium_write8(&priv->regs->ep0maxpack, (u8)req->maximum_packet_length);
			//Set Ep0 Device Address
			phytium_write8(&priv->regs->fnaddr, (u8)req->device_address);
			//Set Ep0 EP Address
			phytium_write8(&priv->regs->ep0ctrl, 0);
			//Reset Fifo
			phytium_write8(&priv->regs->ep0fifoctrl, 0x0);
			phytium_write8(&priv->regs->ep0fifoctrl, BIT(5) | BIT(2));
			//CS Set
			if ((req->stage == HOST_EP0_STAGE_STATUSIN) ||
					(req->stage == HOST_EP0_STAGE_IN)) {
				//Set toggle
				phytium_write8(&priv->regs->ep0_cs, BIT(6));
			}
		}
	} else {
		if (req->ep_num) { /*Ep 1-15*/
			//Enable interrupt
			phytium_write16(&priv->regs->txien,
				phytium_read16(&priv->regs->txien) | BIT(req->ep_num));
			//Transfer mode. Ep num
			phytium_write8(&priv->regs->ep[ep_num - 1].txcon, reg_con);
			//Set Ep0 Device Address
			phytium_write8(&priv->regs->fnaddr, (u8) device_address);
			//set out
			phytium_write8(&priv->regs->endprst, BIT(4) | ep_num);
			//reset toggle and fifo
			if (req->toggle)
				phytium_write8(&priv->regs->endprst, BIT(6) | BIT(7) | BIT(4) | ep_num);
			else
				phytium_write8(&priv->regs->endprst, BIT(6) | BIT(5) | BIT(4) | ep_num);

			//fifo control out
			phytium_write8(&priv->regs->fifoctrl, BIT(5) | BIT(4) | ep_num);
			//coutn switch ep_num
			phytium_write8(&priv->regs->ep_ext[ep_num - 1].txctrl, ep_num);
			//Max packet size
			phytium_write16(&priv->regs->txmaxpack[ep_num - 1], maximum_packet_length);
			//bulk mode. Ep num
			phytium_write8(&priv->regs->ep[ep_num - 1].txcon, CON_TYPE_BULK | BIT(7));
			//Int Transfer Interval
			if (req->frame_interval != 0) {
				phytium_write8(&priv->regs->txsoftimer[ep_num].ctrl,
					phytium_read8(&priv->regs->txsoftimer[ep_num].ctrl) | BIT(1));
				phytium_write16(&priv->regs->txsoftimer[ep_num].timer, req->frame_interval);
				phytium_write8(&priv->regs->txsoftimer[ep_num].ctrl, 0x83);
			}
		} else {/*Ep 0*/
			//Enable Tx Interrupt
			phytium_write16(&priv->regs->txien,
				phytium_read16(&priv->regs->txien) | BIT(0));
			phytium_write16(&priv->regs->txerrien,
				phytium_read16(&priv->regs->txerrien) | BIT(0));
			//Set Ep0 Max Packet Number
			phytium_write8(&priv->regs->ep0maxpack, (u8)req->maximum_packet_length);
			//Set Ep0 Device Address
			phytium_write8(&priv->regs->fnaddr, (u8)req->device_address);
			//Set Ep0 EP Address
			phytium_write8(&priv->regs->ep0ctrl, 0);
			//Reset Fifo
			phytium_write8(&priv->regs->ep0fifoctrl, BIT(4));
			phytium_write8(&priv->regs->ep0fifoctrl, BIT(5) | BIT(2) | BIT(4));
			//HSet
			if (req->stage == HOST_EP0_STAGE_SETUP)
				phytium_write8(&priv->regs->ep0_cs,
					phytium_read8(&priv->regs->ep0_cs) | BIT(4));
		}
	}

	return 0;
}

bool host_check_transfer_complete(struct host_ctrl  *priv, u8 ep_num,
		u8 is_in, u32 *transfer_result)
{
	bool  state;
	u8    code;
	u8    error;

	state = false;
	*transfer_result = USB_NOERROR;
	struct dma_ctrl *priv_dma = (struct dma_ctrl *)(priv->dma_controller);

	if (is_in) {
		if ((phytium_read16(&priv->regs->rxirq) & BIT(ep_num))
				&& (priv->dma_drv->dma_check_interrupt(priv->dma_controller,
					ep_num, is_in) == true)) {
			//clear epn rx interrupt
			state = true;
			phytium_write32(&priv_dma->regs->ep_sts, BIT(2));
			phytium_write16(&priv->regs->rxirq, BIT(ep_num));
			code = (phytium_read8(&priv->regs->ep_ext[ep_num - 1].rxerr) >> 2) & 0x7;
			//Check ep0 tx error irq
			if (phytium_read16(&priv->regs->rxerrirq) & BIT(ep_num)) {
				error = decode_error_code(code);
				//clear ep0 tx error irq
				phytium_write16(&priv->regs->rxerrirq, BIT(ep_num));
				if (error != USB_NOERROR)
					*transfer_result = error;
			}
		}
	} else {
		if ((phytium_read16(&priv->regs->txirq) & BIT(ep_num))
				&& (priv->dma_drv->dma_check_interrupt(priv->dma_controller,
					ep_num, is_in) == true)) {
			//clear ep0 tx interrupt
			state = true;
			phytium_write32(&priv_dma->regs->ep_sts, BIT(2));
			phytium_write16(&priv->regs->txirq, BIT(ep_num));
			code = (phytium_read8(&priv->regs->ep_ext[ep_num - 1].txerr) >> 2) & 0x7;
			//Check ep0 tx error irq
			if (phytium_read16(&priv->regs->txerrirq) & BIT(ep_num)) {
				error = decode_error_code(code);
				//clear ep0 tx error irq
				phytium_write16(&priv->regs->txerrirq, BIT(ep_num));
				if (error != USB_NOERROR)
					*transfer_result = error;
			}
		}
	}

	return state;
}

int32_t _host_init(struct host_ctrl *priv, struct host_cfg *config,
		struct dma_ctrl *dma_controller, bool is_vhub_host)
{
	int index;

	if (!config || !priv)
		return 0;

	priv->host_cfg = *config;
	priv->regs = (struct hw_regs *)config->reg_base;
	priv->host_drv = host_get_instance();

	priv->dma_drv = dma_get_instance();
	priv->dma_cfg.dma_mode_rx = 0xFFFF;
	priv->dma_cfg.dma_mode_tx = 0xFFFF;
	priv->dma_cfg.reg_base = config->reg_base + 0x400;
	priv->dma_cfg.trb_addr = config->trb_addr;
	priv->dma_cfg.trb_dma_addr = config->trb_dma_addr;

	dma_controller->regs = (struct dma_regs *)(priv->dma_cfg.reg_base);
	priv->dma_controller = (void *)dma_controller;

	INIT_LIST_HEAD(&priv->ctrl_h_ep_queue);

	for (index = 0; index < MAX_INSTANCE_EP_NUM; index++) {
		INIT_LIST_HEAD(&priv->iso_in_h_ep_queue[index]);
		INIT_LIST_HEAD(&priv->iso_out_h_ep_queue[index]);
		INIT_LIST_HEAD(&priv->int_in_h_ep_queue[index]);
		INIT_LIST_HEAD(&priv->int_out_h_ep_queue[index]);
		INIT_LIST_HEAD(&priv->bulk_in_h_ep_queue[index]);
		INIT_LIST_HEAD(&priv->bulk_out_h_ep_queue[index]);
	}

	phytium_write8(&priv->regs->cpuctrl, BIT(1));
	phytium_write8(&priv->regs->otgctrl, OTGCTRL_ABUSDROP);
	phytium_write8(&priv->regs->ep0maxpack, 0x40);

	//disable interrupts
	phytium_write8(&priv->regs->otgien, 0x0);
	phytium_write8(&priv->regs->usbien, 0x0);
	phytium_write16(&priv->regs->txerrien, 0x0);
	phytium_write16(&priv->regs->rxerrien, 0x0);

	//clear all interrupt except otg idle irq
	phytium_write8(&priv->regs->otgirq, 0xFE);
	phytium_write8(&priv->regs->usbirq, 0xFF);
	phytium_write16(&priv->regs->txerrirq, 0xFF);
	phytium_write16(&priv->regs->rxerrirq, 0xFF);

	phytium_write8(&priv->regs->tawaitbcon, 0x80);

	priv->otg_state = HOST_OTG_STATE_B_IDLE;

	if (is_vhub_host)
		priv->vhub_regs = (struct vhub_regs *)(config->phy_reg_base);
	else
		priv->custom_regs = (struct custom_regs *)(config->phy_reg_base);

	return 0;
}

void _host_destroy(struct host_ctrl *priv)
{
}

void _host_start(struct host_ctrl *priv)
{
	uint8_t otg_state, usbien;

	if (!priv)
		return;

	usbien = phytium_read8(&priv->regs->usbien);
	usbien = usbien | USBIR_URES | USBIR_SUSP;
	phytium_write8(&priv->regs->usbien, usbien);
retry:
	otg_state = phytium_read8(&priv->regs->otgstate);
	debug("otg_state = %d\n", otg_state);
	switch (otg_state) {
	case HOST_OTG_STATE_A_IDLE:
		priv->ep0_state = HOST_EP0_STAGE_IDLE;
		priv->otg_state = HOST_OTG_STATE_A_IDLE;
		phytium_write8(&priv->regs->otgirq, OTGIRQ_IDLEIRQ);
		host_set_vbus(priv, 1);
		break;
	case HOST_OTG_STATE_B_IDLE:
		host_set_vbus(priv, 1);
		break;
	case HOST_OTG_STATE_A_WAIT_VFALL:
		goto retry;
	}

	phytium_write8(&priv->regs->otgien, OTGIRQ_CONIRQ | OTGIRQ_VBUSERRIRQ
		| OTGIRQ_SRPDETIRQ);
}

void _host_stop(struct host_ctrl *priv)
{
	if (!priv)
		return;

	phytium_write8(&priv->regs->otgien, 0x0);
	phytium_write8(&priv->regs->usbien, 0x0);
	phytium_write16(&priv->regs->txerrien, 0x0);
	phytium_write16(&priv->regs->rxerrien, 0x0);

	phytium_write8(&priv->regs->otgirq, 0xFE);
	phytium_write8(&priv->regs->usbirq, 0xFF);
	phytium_write16(&priv->regs->txerrirq, 0xFF);
	phytium_write16(&priv->regs->rxerrirq, 0xFF);

	host_set_vbus(priv, 0);
}

void _host_isr(struct host_ctrl *priv)
{
	host_otg_irq(priv);
}

int32_t host_get_private_data_size(struct host_ctrl *priv)
{
	if (!priv)
		return 0;

	return sizeof(struct host_ep_priv);
}

int hub_descriptor(struct usb_hub_descriptor *buffer)

{
	buffer->bLength = 9;
	buffer->bDescriptorType = 0x29;
	buffer->bNbrPorts = 1;
	buffer->wHubCharacteristics = 0x11;
	buffer->bPwrOn2PwrGood = 5;
	buffer->bHubContrCurrent = 0;
	buffer->u.hs.DeviceRemovable[0] = 0x2;

	return 0;
}

int dev_descriptor(void *buffer, struct usb_device *dev,
		struct devrequest *cmd, int txlen)
{
	int len = 0;
	int stat = 0;
	uint16_t wValue = cpu_to_le16(cmd->value);
	uint16_t wLength = cpu_to_le16(cmd->length);
	struct usb_hub_descriptor *hub_buf = (struct usb_hub_descriptor *)(buffer);

	switch (cmd->requesttype & ~USB_DIR_IN) {
	case 0:
		switch (wValue & 0xff00) {
		case 0x0100:	/* device descriptor */
			len = min3(txlen, (int)sizeof(root_hub_dev_des), (int)wLength);
			memcpy(buffer, root_hub_dev_des, len);

			struct usb_device_descriptor *dev_buf =
				(struct usb_device_descriptor *)buffer;
			dev_buf->bcdUSB = 0x200;
			dev_buf->bDeviceProtocol = 0x1;
			dev_buf->bMaxPacketSize0 = 0x40;
			dev_buf->idVendor = 0x1d6b;
			dev_buf->idProduct = 0x0002;
			break;
		case 0x0200:	/* configuration descriptor */
			len = min3(txlen, (int)sizeof(root_hub_config_des), (int)wLength);
			memcpy(buffer, root_hub_config_des, len);
			struct usb_config_descriptor *conf_buf =
				(struct usb_config_descriptor *)buffer;
			conf_buf->bmAttributes = 0xe0;
			struct usb_endpoint_descriptor *ep_buf =
				(struct usb_endpoint_descriptor *)(buffer +
					(sizeof(struct usb_config_descriptor)) +
						(sizeof(struct usb_interface_descriptor)));
			ep_buf->wMaxPacketSize = 0x4;
			ep_buf->bInterval = 0xc;
			break;
		case 0x0300:	/* string descriptors */
			switch (wValue & 0xff) {
			case 0x00:
				len = min3(txlen, (int)sizeof(root_hub_str_index0), (int)wLength);
				memcpy(buffer, root_hub_str_index0, len);
				break;
			case 0x01:
				len = min3(txlen, (int)sizeof(root_hub_str_index1), (int)wLength);
				memcpy(buffer, root_hub_str_index1, len);
				break;
			}
			break;
		default:
			stat = USB_ST_STALLED;
		}
		break;

	case USB_TYPE_CLASS:
		//hun desc
		hub_buf->bLength = 9;
		hub_buf->bDescriptorType = 0x29;
		hub_buf->bNbrPorts = 1;
		hub_buf->wHubCharacteristics = 0x11;
		hub_buf->bPwrOn2PwrGood = 5;
		hub_buf->bHubContrCurrent = 0;
		hub_buf->u.hs.DeviceRemovable[0] = 0x2;
		len = min3(txlen, (int)hub_buf->bLength, (int)wLength);
		break;
	default:
		puts("unsupported root hub command\n");
		stat = USB_ST_STALLED;
	}

	dev->act_len = min(len, txlen);
	dev->status = stat;

	return stat;
}

int hub_port_suspend(struct host_ctrl *priv, u16 on)
{
	uint8_t otgctrl;

	if (!priv)
		return -1;

	otgctrl = phytium_read8(&priv->regs->otgctrl);

	if (on) {
		otgctrl &= ~OTGCTRL_BUSREQ;
		otgctrl &= ~OTGCTRL_BHNPEN;

		priv->port_status |= USB_PORT_STAT_SUSPEND;

		switch (phytium_read8(&priv->regs->otgstate)) {
		case HOST_OTG_STATE_A_HOST:
			priv->otg_state = HOST_OTG_STATE_A_SUSPEND;
			otgctrl |= OTGCTRL_ASETBHNPEN;
			break;
		case HOST_OTG_STATE_B_HOST:
			priv->otg_state = HOST_OTG_STATE_B_HOST_2;
			break;
		default:
			break;
		}
		phytium_write8(&priv->regs->otgctrl, otgctrl);
	} else {
		otgctrl |= OTGCTRL_BUSREQ;
		otgctrl &= ~OTGCTRL_ASETBHNPEN;
		phytium_write8(&priv->regs->otgctrl, otgctrl);
		priv->port_status |= USB_PORT_STAT_RESUME;
	}

	return 0;
}

void hub_port_reset(struct host_ctrl *priv, uint8_t on)
{
	uint8_t speed;

	if (!priv)
		return;

	if (on) {
		phytium_write16(&priv->regs->txerrirq, 0xFFFF);
		phytium_write16(&priv->regs->txirq, 0xFFFF);
		phytium_write16(&priv->regs->rxerrirq, 0xFFFF);
		phytium_write16(&priv->regs->rxirq, 0xFFFF);

		phytium_write8(&priv->regs->ep0fifoctrl, FIFOCTRL_FIFOAUTO | 0 | 0x04);
		phytium_write8(&priv->regs->ep0fifoctrl, FIFOCTRL_FIFOAUTO |
				FIFOCTRL_IO_TX | 0 | 0x04);

		priv->port_status |= USB_PORT_STAT_RESET;
		priv->port_status &= ~USB_PORT_STAT_ENABLE;
		priv->port_resetting = 0;
	} else {
		speed = phytium_read8(&priv->regs->speedctrl);
		if (speed == SPEEDCTRL_HS)
			priv->port_status |= USB_PORT_STAT_HIGH_SPEED;
		else if (speed == SPEEDCTRL_FS)
			priv->port_status &= ~(USB_PORT_STAT_HIGH_SPEED | USB_PORT_STAT_LOW_SPEED);
		else
			priv->port_status |= USB_PORT_STAT_LOW_SPEED;

		priv->port_status &= ~USB_PORT_STAT_RESET;
		priv->port_status |= USB_PORT_STAT_ENABLE | (USB_PORT_STAT_C_RESET << 16)
			| (USB_PORT_STAT_C_ENABLE << 16);
	}
}

int get_port_status(struct host_ctrl *priv, u16 wIndex, uint8_t *buff)
{
	uint32_t temp = 0;

	if (!priv || !buff)
		return 0;

	if ((wIndex & 0xff) != 1)
		return 1;

	if (priv->port_status & USB_PORT_STAT_RESET) {
		if (!priv->port_resetting)
			hub_port_reset(priv, 0);
	}

	if (priv->port_status & USB_PORT_STAT_RESUME) {
		priv->port_status &= ~(USB_PORT_STAT_SUSPEND | USB_PORT_STAT_RESUME);
		priv->port_status |= USB_PORT_STAT_C_SUSPEND << 16;
	}

	temp = priv->port_status & (~USB_PORT_STAT_RESUME);
	buff[0] = temp;
	buff[1] = temp >> 8;
	buff[2] = temp >> 16;
	buff[3] = temp >> 24;

	return 0;
}

int set_port_feature(struct host_ctrl *priv, u16 wValue, u16 wIndex)
{
	if ((wIndex & 0xff) != 1)
		return 1;

	switch (wValue) {
	case USB_PORT_FEAT_CONNECTION:
		break;
	case USB_PORT_FEAT_ENABLE:
		break;
	case USB_PORT_FEAT_SUSPEND:
		hub_port_suspend(priv, 1);
		break;
	case USB_PORT_FEAT_OVER_CURRENT:
		break;
	case USB_PORT_FEAT_RESET:
		hub_port_reset(priv, 1);
		break;
	case USB_PORT_FEAT_L1:
		break;
	case USB_PORT_FEAT_POWER:
		_host_start(priv);
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
	priv->port_status |= 1 << wValue;

	return 0;
}

int clear_port_feature(struct host_ctrl *priv, u16 wValue, u16 wIndex)
{
	if ((wIndex & 0xff) != 1)
		return 1;

	switch (wValue) {
	case USB_PORT_FEAT_CONNECTION:
		break;
	case USB_PORT_FEAT_ENABLE:
		break;
	case USB_PORT_FEAT_SUSPEND:
		hub_port_suspend(priv, 0);
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
	priv->port_status &= ~(1 << wValue);

	return 0;
}

static int phytium_host_set_default_cfg(struct phytium_cusb *config)
{
	int index;

	config->host_cfg.reg_base = (uintptr_t)config->regs;
	config->host_cfg.phy_reg_base = (uintptr_t)config->phy_regs;
	config->host_cfg.dmult_enabled = 1;
	config->host_cfg.memory_alignment = 0;
	config->host_cfg.dma_support = 1;
	config->host_cfg.is_embedded_host = 1;

	for (index = 0; index < HOST_EP_NUM; index++) {
		if (index == 0) {
			config->host_cfg.ep_in[index].buffering_value = 1;
			config->host_cfg.ep_in[index].max_packet_size = 64;
			config->host_cfg.ep_in[index].start_buf = 0;

			config->host_cfg.ep_out[index].buffering_value = 1;
			config->host_cfg.ep_out[index].max_packet_size = 64;
			config->host_cfg.ep_out[index].start_buf = 0;
		} else {
			config->host_cfg.ep_in[index].buffering_value = 2;
			config->host_cfg.ep_in[index].max_packet_size = 1024;
			config->host_cfg.ep_in[index].start_buf = 64 + 2 * 1024 * (index - 1);

			config->host_cfg.ep_out[index].buffering_value = 2;
			config->host_cfg.ep_out[index].max_packet_size = 1024;
			config->host_cfg.ep_out[index].start_buf = 64 + 2 * 1024 * (index - 1);
		}
	}

	return 0;
}

int phytium_host_init(struct phytium_cusb *config)
{
	int ret;

	if (!config)
		return 0;

	phytium_host_set_default_cfg(config);
	config->host_obj = host_get_instance();
	config->dma_cfg.reg_base = config->host_cfg.reg_base + 0x400;

	config->dma_obj = dma_get_instance();

	config->host_sysreq.priv_data_size = sizeof(struct host_ctrl);
	config->host_sysreq.trb_mem_size = config->dma_sysreq.trb_mem_size;
	config->host_sysreq.priv_data_size += config->dma_sysreq.priv_data_size;

	config->host_priv = malloc(config->host_sysreq.priv_data_size);
	if (!config->host_priv)
		ret = -ENOMEM;

	ret = config->host_obj->host_init(config->host_priv, &config->host_cfg,
			&config->dma_controller, config->is_vhub_host);
	if (ret) {
		printf("host init failed! ret = %d\n", ret);
		return ret;
	}

	config->host_obj->host_start(config->host_priv);
	dev_set_drvdata(config->dev, config);

	return 0;
}

int32_t host_vhub_control(struct host_ctrl *priv,
		struct usb_ctrlrequest *setup, uint8_t *buff)
{
	uint16_t request;
	uint32_t retval = 0;

	if (!priv || !setup || !buff)
		return -EINVAL;

	request = setup->bRequestType << 0x08 | setup->bRequest;
	switch (request) {
	case CLEAR_HUB_FEATURE:
		break;
	case SET_HUB_FEATURE:
		break;
	case CLEAR_PORT_FEATURE:
		retval = clear_port_feature(priv, setup->wValue, setup->wIndex);
		break;
	case SET_PORT_FEATURE:
		retval = set_port_feature(priv, setup->wValue, setup->wIndex);
		break;
	case GET_HUB_DESCRIPTOR:
		retval = hub_descriptor((struct usb_hub_descriptor *)buff);
		break;
	case GET_HUB_STATUS:
		break;
	case GET_PORT_STATUS:
		retval = get_port_status(priv, setup->wIndex, (uint8_t *)buff);
		break;
	default:
		retval = EOPNOTSUPP;
		break;
	}

	return retval;
}

struct host_obj host_drv = {
	.host_init		=	_host_init,
	.host_destroy	=	_host_destroy,
	.host_start		=	_host_start,
	.host_stop		=	_host_stop,
	.host_isr		=	_host_isr,

	.host_vhub_control =	host_vhub_control,
	.host_ep_get_private_data_size = host_get_private_data_size,
};

struct host_obj *host_get_instance(void)
{
	return &host_drv;
}
