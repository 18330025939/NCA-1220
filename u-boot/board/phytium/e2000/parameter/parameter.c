// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * lixinde <lixinde@phytium.com.cn>
 */

#include <common.h>
#include "parameter.h"
#include <e_uart.h>

void *get_parameter_base(uint32_t id)
{
	head_config_t *head;
	uint64_t offset, magic;

	switch (id) {
	case PM_PLL:
		offset = PM_PLL_OFFSET;
		magic = PARAMETER_PLL_MAGIC;
		break;
	case PM_PCIE:
		offset = PM_PCIE_OFFSET;
		magic = PARAMETER_PCIE_MAGIC;
		break;
	case PM_COMMON:
		offset = PM_COMMON_OFFSET;
		magic = PARAMETER_COMMON_MAGIC;
		break;
	case PM_MCU:
		offset = PM_MCU_OFFSET;
		magic = PARAMETER_MCU_MAGIC;
		break;
	case PM_BOARD:
		offset = PM_BOARD_OFFSET;
		magic = PARAMETER_BOARD_MAGIC;
		break;
	default:
		printf("bed input\n");
		while(1);
	}

	head = (head_config_t *)(O_PARAMETER_BASE + offset);

	if(magic != head->magic)
		return NULL;
	else
		return (void *)(head);
}

int get_parameter_info(uint32_t id, uint8_t *pm_data, const uint32_t data_size)
{
	head_config_t *head;
	head = (head_config_t *)get_parameter_base(id);

	if(!head)
		return -1;

	if((data_size < head->size) || (16 > head->size)){
		p_printf("error data size %d - %d\n", data_size, head->size);
		while(1);
	}

	memcpy(pm_data, head, head->size);
	return 0;
}
