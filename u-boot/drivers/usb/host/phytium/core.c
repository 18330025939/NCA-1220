// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2023 Phytium Corporation.
 */

#include <common.h>
#include <asm/io.h>
#include "core.h"

int phytium_core_reset(struct phytium_cusb *config, bool skip_wait)
{
	if (!config)
		return 0;

	spin_lock_init(&config->lock);

	return 0;
}

uint32_t phytium_read32(uint32_t *address)
{
#ifdef USB2_DEBUG
	uint32_t value;

	value = readl(address);
	printf("read32 addr = 0x%p, value = 0x%x\n", address, value);
	return value;
#else
	return readl(address);
#endif
}

void phytium_write32(uint32_t *address, uint32_t value)
{
#ifdef USB2_DEBUG
	printf("write32 addr = 0x%p, value = 0x%x\n", address, value);
	writel(value, address);
#else
	writel(value, address);
#endif
}

uint16_t phytium_read16(uint16_t *address)
{
#ifdef USB2_DEBUG
	uint16_t value;

	value = readw(address);
	printf("read16 addr = 0x%p, value = 0x%x\n", address, value);
	return value;
#else
	return readw(address);
#endif
}

void phytium_write16(uint16_t *address, uint16_t value)
{
#ifdef USB2_DEBUG
	printf("write16 addr = 0x%p, value = 0x%x\n", address, value);
	writew(value, address);
#else
	writew(value, address);
#endif
}

uint8_t phytium_read8(uint8_t *address)
{
#ifdef USB2_DEBUG
	uint8_t value;

	value = readb(address);
	printf("read8 addr = 0x%p, value = 0x%x\n", address, value);
	return value;
#else
	return readb(address);
#endif
}

void phytium_write8(uint8_t *address, uint8_t value)
{
#ifdef USB2_DEBUG
	printf("write8 addr = 0x%p, value = 0x%x\n", address, value);
	writeb(value, address);
#else
	writeb(value, address);
#endif
}
