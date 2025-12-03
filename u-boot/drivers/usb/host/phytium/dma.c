// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#include "core.h"
#include "dma.h"
#include "hw-regs.h"

int phytium_dma_channel_program(struct dma_ctrl *priv, u8 ep_num,
		u8 is_in, u32 trb_addr)
{
	u32  temp;

	//single Mode
	phytium_write32(&priv->regs->conf, BIT(8));
	//Enable TRB Error Interrupt
	phytium_write32(&priv->regs->ep_sts_en, BIT(7));
	if (is_in) {
		//ep_num and In
		phytium_write32(&priv->regs->ep_sel, ep_num);
		//Enable Interrupt
		temp = phytium_read32(&priv->regs->ep_ien);
		temp |= BIT((ep_num + 16));
		phytium_write32(&priv->regs->ep_ien, temp);
	} else {
		//ep_num and Out
		phytium_write32(&priv->regs->ep_sel, BIT(7) | ep_num);
		//Enable Interrupt
		temp = phytium_read32(&priv->regs->ep_ien);
		temp |= BIT(ep_num);
		phytium_write32(&priv->regs->ep_ien, temp);
	}
	//Trb Address
	phytium_write32(&priv->regs->traddr, trb_addr);
	phytium_write32(&priv->regs->ep_cmd, BIT(6));
	phytium_write32(&priv->regs->ep_cfg, BIT(0));

	return 0;
}

int phytium_dma_channel_release(struct dma_ctrl *priv, u8 ep_num, u8 is_in)
{
	if (is_in)
		phytium_write32(&priv->regs->ep_sel, ep_num);
	else
		phytium_write32(&priv->regs->ep_sel, BIT(7) | ep_num);

	phytium_write32(&priv->regs->ep_cmd, BIT(0));

	return 0;
}

bool phytium_dma_check_interrupt(struct dma_ctrl *priv, u8 ep_num, u8 is_in)
{
	if (is_in)
		phytium_write32(&priv->regs->ep_sel, ep_num);
	else
		phytium_write32(&priv->regs->ep_sel, BIT(7) | ep_num);

	if ((phytium_read32(&priv->regs->ep_sts) &  BIT(2)) || (phytium_read32(&priv->regs->ep_sts) &  BIT(3)))
		return true;
	else
		return false;
}

void phytium_dma_controller_reset(struct dma_ctrl *priv)
{
	u32  conf;

	if (!priv)
		return;

	conf = phytium_read32(&priv->regs->conf);
	conf |= BIT(0);
	phytium_write32(&priv->regs->conf, conf);
}

void phytium_dma_set_host_mode(struct dma_ctrl *priv)
{
	if (!priv)
		return;

	priv->is_host_ctrl_mode = 1;
}

struct dma_obj phytium_dma_drv = {
	.dma_channel_program = phytium_dma_channel_program,
	.dma_channel_release = phytium_dma_channel_release,
	.dma_check_interrupt = phytium_dma_check_interrupt,
	.dma_controller_reset = phytium_dma_controller_reset,
	.dma_set_host_mode = phytium_dma_set_host_mode,
};

struct dma_obj *dma_get_instance(void)
{
	return &phytium_dma_drv;
}
