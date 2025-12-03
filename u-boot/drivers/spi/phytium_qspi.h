/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#ifndef __PHYTIUM_QSPI_H__
#define __PHYTIUM_QSPI_H_

int qspi_erase_block(uint64_t block_address);
void qspinor_erase(uint64_t addr, uint32_t len);
int qspi_write_word(uint64_t address, uint32_t value);
int qspi_page_program(uint64_t src_addr, uint64_t page_addr, uint32_t len);

#endif /*__PHYTIUM_QSPI_H_*/

