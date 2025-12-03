// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium LBC nor flash driver
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <e_uart.h>

#ifndef PHYTIUM_LBC
#  error  must have PHYTIUM_LBC
#endif

#define E2000_LBC_BASE				0x28007000
#define LBC_CSAR0					0x018
#define LBC_CSASR0					0x01c
#define LBC_CFG_SEQ_ADDR			0x130
#define LBC_CFG_SEQ_DATA			0x134
#define LBC_CFG_SEQ_STAT			0x138
#define LBC_FTIM0_CS0_NOR			0x13c
#define LBC_FTIM1_CS0_NOR			0x140
#define LBC_FTIM2_CS0_NOR			0x144
#define LBC_RB_STAT					0x0d8
#define LBC_CSPR0					0x058

#define E2000_LBC_NOR_FLASH_BASE	0x10000000

void e2000_lbc_clock_setup(void)
{
	/*cs0写时序配置*/
	writel(0x3fc40f, (size_t)(E2000_LBC_BASE + LBC_FTIM2_CS0_NOR));//0x144
	writel(0x3f1f01, (size_t)(E2000_LBC_BASE + LBC_FTIM1_CS0_NOR));//0x140
	writel(0x007f03, (size_t)(E2000_LBC_BASE + LBC_CSPR0));//0x058

#ifdef PHYTIUM_LBC_DIRECT
	//片选0
	writel(0x010000, (size_t)(E2000_LBC_BASE + LBC_CSAR0));//0x018
	writel(0x00000e, (size_t)(E2000_LBC_BASE + LBC_CSASR0));//0x01c
#endif
	p_printf("e2000 lbc time seq setup ok!\n");
}

static void e2000_lbc_wait_ready(void)
{
	int ret;

	while(1) {
		ret = readl((size_t)(E2000_LBC_BASE + LBC_RB_STAT));
		debug("lbc rb status = 0x%x\n", ret);
		if(((ret >> 0) & 0x1) == 0x0) {
			while(1) {
				ret = readl((size_t)(E2000_LBC_BASE + LBC_RB_STAT));
				if((ret >> 0) & 0x1)
					break;
			}
			break;
		}
	}
}

#ifndef PHYTIUM_LBC_DIRECT /*通过寄存器间接访问*/
static void e2000_lbc_nor_flash_write(u32 offset, u32 data)
{
	writel(data, (size_t)(E2000_LBC_BASE + offset));
}

static void e2000_lbc_erase_chip(void)
{
	printf("nor flash erase chip start ...\n");
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00aa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0055);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0080);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00aa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0055);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0010);
	e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

	e2000_lbc_wait_ready();
	//mdelay(1000 * 150);//128M:30~150s
	printf("nor flash erase chip end!\n");
}

void e2000_lbc_erase_sector(u32 addr, u32 space)
{
	printf("nor flash erase sector start ...\n");
	u32 buff_erase, addr_temp;
	u32 i;

	if (space <= 0x20000) {//128k
		buff_erase = 1;
	} else {
		if ((space%0x20000)>0) {
			buff_erase = space/0x20000+1;
		} else {
			buff_erase = space/0x20000;
		}
	}

	for (i=0; i<buff_erase; i++) {
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00aa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0055);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0080);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00aa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0055);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		addr_temp = addr + (i * 0x20000);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, addr_temp);//sector addr
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0030);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);
	}

	e2000_lbc_wait_ready();
	//mdelay(2000);//128M:0.2~2s
	printf("nor flash erase sector end!\n");
}

void e2000_lbc_write_word(u32 src_addr, u32 page_addr, u32 pp_num)
{
	printf("nor flash write ...\n");
	u32 src_addr_temp, page_addr_temp, num;
	u32 value = 0;

	src_addr_temp = src_addr;
	page_addr_temp = page_addr;
	num = pp_num/2;

	while (num-- > 0) {
		value = readw((size_t)src_addr_temp);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00aa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x0055);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x00a0);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, page_addr_temp);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, value);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_wait_ready();
		//udelay(200);
		src_addr_temp += 2;
		page_addr_temp += 2;
	}
	printf("nor flash write word ok!\n");
}

void e2000_lbc_write_buffer(unsigned long src_addr, unsigned long page_addr, unsigned int pp_num)
{
	printf("nor flash write ...\n");
	uint32_t i, cnt, bp_num, write_buff_count, value;
	static unsigned long src_addr_temp, page_addr_temp;

	bp_num = pp_num;

	if ((page_addr_temp%64) > 0) {
		printf("page_addr error!!!\n");
	}

	if (bp_num <= 64) {
		write_buff_count = 1;
	} else {
		if ((bp_num%64) > 0) {
			write_buff_count = (bp_num/64) + 1;
		} else {
			write_buff_count = bp_num/64;
		}
	}

	for (cnt=0; cnt<write_buff_count; cnt++) {
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0xaaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0xaa);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x1);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, 0x554);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x55);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x01);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, (page_addr + (64*cnt)));
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x25);
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x01);

		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, (page_addr + (64*cnt)));
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x1f);//write word count(minus 1)
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x01);

		page_addr_temp = page_addr + (64*cnt);
		src_addr_temp  = src_addr + (64*cnt);

		for (i=0; i<=31; i++) {
			value = readw((size_t)src_addr_temp);
			e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, page_addr_temp);
			e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, value);
			e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x01);
			page_addr_temp = page_addr_temp + 2;
			src_addr_temp = src_addr_temp + 2;
		}
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_ADDR, (page_addr + (64*cnt)));
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_DATA, 0x29);//write confirm
		e2000_lbc_nor_flash_write(LBC_CFG_SEQ_STAT, 0x01);
		e2000_lbc_wait_ready();
		//udelay(250);
	}
	printf("nor flash write buffer ok!\n");
}
#else/*直接地址访问*/
static void e2000_lbc_read_nor_flash_id(void)
{
	writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
	writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
	writew(0x0090, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));

	u16 manufacturer_id = readw((size_t)(E2000_LBC_NOR_FLASH_BASE + 0x00));
	u16 device_id = readw((size_t)(E2000_LBC_NOR_FLASH_BASE + 0x02));
	printf("manufacturer_id  = 0x%x, device_id = 0x%x\n", manufacturer_id, device_id);

	writew(0x00F0, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x000));//exit
}

static void e2000_lbc_erase_chip(void)
{
	printf("nor flash erase chip start ...\n");
	writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
	writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
	writew(0x0080, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
	writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
	writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
	writew(0x0010, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));

	e2000_lbc_wait_ready();
	//mdelay(1000 * 150);//128M:30~150s
	printf("nor flash erase chip end!\n");
}

void e2000_lbc_erase_sector(u32 addr, u32 space)
{
	printf("nor flash erase sector start ...\n");
	u32 buff_erase, addr_temp;
	u32 i;

	if (space <= 0x20000) {//128k
		buff_erase = 1;
	} else {
		if ((space%0x20000)>0) {
			buff_erase = space/0x20000+1;
		} else {
			buff_erase = space/0x20000;
		}
	}

	for (i=0; i<buff_erase; i++) {
		writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
		writew(0x0080, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
		addr_temp = addr + (i * 0x20000);
		writew(0x0030, (size_t)(E2000_LBC_NOR_FLASH_BASE + addr_temp));//sector addr
	    e2000_lbc_wait_ready();
	}

	//mdelay(2000);//128M:0.2~2s
	printf("nor flash erase sector end!\n");
}

void e2000_lbc_write_word(u32 src_addr, u32 page_addr, u32 pp_num)
{
	printf("nor flash write ...\n");
	u32 src_addr_temp, page_addr_temp, num;
	u16 value = 0;

	src_addr_temp = src_addr;
	page_addr_temp = page_addr;
	num = pp_num/2;

	while (num-- > 0) {
		value = readw((size_t)src_addr_temp);
		writew(0x00aa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(0x0055, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
		writew(0x00a0, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(value, (size_t)(E2000_LBC_NOR_FLASH_BASE + page_addr_temp));

		e2000_lbc_wait_ready();
		//udelay(200);
		src_addr_temp += 2;
		page_addr_temp += 2;
	}
	printf("nor flash write word ok!\n");
}

void e2000_lbc_write_buffer(unsigned long src_addr, unsigned long page_addr, unsigned int pp_num)
{
	printf("nor flash write ...\n");
	uint32_t i, cnt, bp_num, write_buff_count, value;
	static unsigned long src_addr_temp, page_addr_temp;

	bp_num = pp_num;

	if ((page_addr_temp%64) > 0) {
		printf("page_addr error!!!\n");
	}

	if (bp_num <= 64) {
		write_buff_count = 1;
	} else {
		if ((bp_num%64) > 0) {
			write_buff_count = (bp_num/64) + 1;
		} else {
			write_buff_count = bp_num/64;
		}
	}

	for (cnt=0; cnt<write_buff_count; cnt++) {
		writew(0xaa, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0xaaa));
		writew(0x55, (size_t)(E2000_LBC_NOR_FLASH_BASE + 0x554));
		writew(0x25, (size_t)(E2000_LBC_NOR_FLASH_BASE + (page_addr + (64*cnt))));
		writew(0x1f, (size_t)(E2000_LBC_NOR_FLASH_BASE + (page_addr + (64*cnt))));//write word count(minus 1)

		page_addr_temp = page_addr + (64*cnt);
		src_addr_temp  = src_addr + (64*cnt);

		for (i=0; i<=31; i++) {
			value = readw((size_t)src_addr_temp);
			writew(value, (size_t)(E2000_LBC_NOR_FLASH_BASE + page_addr_temp));
			page_addr_temp = page_addr_temp + 2;
			src_addr_temp = src_addr_temp + 2;
		}
		writew(0x29, (size_t)(E2000_LBC_NOR_FLASH_BASE + (page_addr + (64*cnt))));
		e2000_lbc_wait_ready();
	}

	printf("nor flash write buffer ok!\n");
}
#endif

static int do_lbc(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	u32 src_addr = 0;
	u32 page_addr = 0;
	u32 space = 0;
	const char *cmd1, *cmd2;

	if (argc < 2 || argc > 5)
		return -1;

	cmd1 = argv[1];
	cmd2 = argv[2];

	if (strcmp(cmd1, "start") == 0) {
		e2000_lbc_clock_setup();
#ifdef PHYTIUM_LBC_DIRECT
		e2000_lbc_read_nor_flash_id();
#endif
	} else if (strcmp(cmd1, "erase") == 0) {
		if (strcmp(cmd2, "chip") == 0)
			e2000_lbc_erase_chip();
		if (strcmp(cmd2, "sector") == 0) {
			src_addr = simple_strtoul(argv[3], NULL, 16);
			space = simple_strtoul(argv[4], NULL, 16);
			e2000_lbc_erase_sector(src_addr , space);
		}
	} else if (strcmp(cmd1, "writeb") == 0) {
		src_addr = simple_strtoul(argv[2], NULL, 16);
		page_addr = simple_strtoul(argv[3], NULL, 16);
		space = simple_strtoul(argv[4], NULL, 16);
		e2000_lbc_write_buffer(src_addr, page_addr, space);
	} else if (strcmp(cmd1, "writew") == 0) {
		src_addr = simple_strtoul(argv[2], NULL, 16);
		page_addr = simple_strtoul(argv[3], NULL, 16);
		space = simple_strtoul(argv[4], NULL, 16);
		e2000_lbc_write_word(src_addr, page_addr, space);
	} else {
		printf("Unknow sub-command.\n");
		return -1;
	}

	return 0;
}

U_BOOT_CMD(
	lbc, 5, 0, do_lbc,
	"lbc erase/write nor flash",
	"start                                  - lbc init\n"
	"lbc erase  chip                            - lbc erase chip\n"
	"lbc erase  sector     [addr]      [space]  - lbc erase one sector 128kBytes\n"
	"lbc writeb [src_addr] [page_addr] [space]  - write buffer 64Bytes to norflash\n"
	"lbc writew [src_addr] [page_addr] [space]  - lbc write word to norflash\n"
);
