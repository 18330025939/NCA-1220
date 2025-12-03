// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/io.h>
#include <e_uart.h>
#include "power_manage.h"
#include "../cpu.h"

void send_cpld_ctr(uint32_t cmd)
{
	p_printf("u-boot : send cmd to cpld : %d \n", cmd);
	int cfg;
	u32 port_level = 0;
	int port = 0;
	
    //send to cpld,gpio1_0,gpio1_1
	//读取之前配置方向寄存器
	cfg = readl((u64)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	cfg |= (1<<port);
	port = 1;
	cfg |= (1<<port);
	//配置为输出模式, 0:输入，1:输出
	writel(cfg, (u64)(GPIO1_BASE + GPIO_SWPORTA_DDR));

	//发送电平值
	port_level = readl((u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
	port = 0;
	port_level |= (1<<port);
	writel(port_level, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));//start
	mdelay(2);

	port = 1;
	port_level |= (1<<port);
	int high = port_level;
	port_level &= ~(1<<port);
	int low  = port_level;

	for(int i = 0; i < cmd; i++){
		writel(high, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
		mdelay(1);
		writel(low, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));
		mdelay(1);
	}
	mdelay(2);
	port = 0;
	port_level &= ~(1<<port);
	writel(port_level, (u64)(GPIO1_BASE + GPIO_SWPORTA_DR));//end
}

int gpio_get_s3_flag(void)
{
	int ret, cfg;
	int port = 0;
    
    //get s3 flag from cpld,gpio1_2
	//读取之前配置方向寄存器
	cfg = readl((size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));
	//配置为输出模式, 0:输入，1:输出
	port = 2;
	cfg &= ~(1<<port);
	writel(cfg, (size_t)(GPIO1_BASE + GPIO_SWPORTA_DDR));

	//读取当前控io寄存器的值
	ret = readl((size_t)(GPIO1_BASE + GPIO_EXT_PORTA));
	debug("s3 flag ret = 0x%x\n", ret);

	//判断对应io引脚是否为高电平
	if(ret & 0x4)
		return  1;
	else
		return  0;
}
