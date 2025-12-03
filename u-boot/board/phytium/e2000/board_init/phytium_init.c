// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/gic.h>
#include "cpu.h"
#include <asm/armv8/mmu.h>
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include <e_uart.h>
#include "board.h"
#include "../cpu.h"
#include "../parameter/parameter.h"
#include <rand.h>

DECLARE_GLOBAL_DATA_PTR;

void inline set_freq_div(uint8_t rank)
{
	uint32_t temp;

	if(rank > 7)
		rank = 7;

	temp = readl(QSPI_ADDR_READ_CFG_REG);
	temp = (temp & (~0x7)) | rank;

	writel(temp, QSPI_ADDR_READ_CFG_REG);
}

void set_flash_speed(void)
{
	uint32_t speed_rank;

	speed_rank = pm_get_qspi_freq_rank();
	p_printf("get flash speed = %d\n", speed_rank);

	speed_rank += 3;
	set_freq_div(speed_rank);
	p_printf("flash set done\n");
}

typedef struct qspi_ops_s{
	uint32_t id;
	void (*fun)(void);
	uint8_t str[16];
}qspi_ops_t;

//QUAD set
void w25q128fw_config(void)
{
	p_printf("qspi flash quad set!\n");
	writel(0x06000000, QSPI_CMD_PORT_REG);
	writel(0x1, QSPI_LD_PORT_REG);

	writel(0x31402000, QSPI_CMD_PORT_REG);
	writel(0x2, QSPI_LD_PORT_REG);

	writel((0x6B<<24) | (0x24 << 16) |(0x7 << 4) | (1 << 3), QSPI_ADDR_READ_CFG_REG);

	writel(0x04000000, QSPI_CMD_PORT_REG);
	writel(0x1, QSPI_LD_PORT_REG);
}

const qspi_ops_t qspi_ops[] = {
	{0x01182001, NULL, "flash debug"},
	{0x001860ef, w25q128fw_config, "w25q128fw"},
	{0x001960ef, w25q128fw_config, "w25q128fw"},
	{0x001840ef, w25q128fw_config, "w25q128"},
	{0x0, NULL, "default flash"}
};

uint32_t get_qspi_ops(void)
{
	uint32_t jedec_id, i;

	writel(0x9F002040, QSPI_CMD_PORT_REG);
	dsb();
	isb();
	jedec_id = readl(QSPI_LD_PORT_REG);
	for(i = 0; 0 != qspi_ops[i].id; i++){
		if(qspi_ops[i].id == jedec_id)
			break;
	}
	p_printf("flash tpye = %s\n", qspi_ops[i].str);
	return i;
}

void qspi_flash_setup(void)
{
	void (*ops_fun)(void);
	uint32_t ops_id;

	ops_id = get_qspi_ops();

	if(0 != ops_id){
		ops_fun = qspi_ops[ops_id].fun;

		if(NULL != ops_fun)
			ops_fun();

		set_flash_speed();
	}
}

/*core 安全等级配置*/
void core_security_level_cfg(void)
{
	struct arm_smccc_res res;
	//level: 0, 1, 2
	uint8_t level_cfg = 0x0;

	printf("security level cfg...\n");
	arm_smccc_smc(CPU_SECURITY_CFG, 0, level_cfg, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

common_config_t const common_base_info = {
	.magic = PARAMETER_COMMON_MAGIC,
	.version = 0x1,
	.core_bit_map = 0x3333,
};

/*get reset source*/
uint32_t get_reset_source(void)
{
    struct arm_smccc_res res;

    debug("get reset source\n");
    arm_smccc_smc(CPU_GET_RST_SOURCE, 0, 0, 0, 0, 0, 0, 0, &res);
    p_printf("reset source = %lx \n", res.a0);

    return res.a0;
}

void check_reset(void)
{
	uint32_t rst;

	qspi_flash_setup();
	rst = get_reset_source();
	if(0x01 == rst){
        p_printf("power on reset done!\n");
		pll_init();
	}else if(0x80 == rst){
		p_printf("wdt reset done!\n");
	}else if(0x03 == rst){
		p_printf("core reset done!\n");
	}else{
		p_printf("other reset source!\n");
		while(1);
	}
}

/*sec init*/
void sec_init(uint8_t s3_flag)
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;
	uint32_t s3_fun = 0;

	/*从参数表中获取参数*/
	if((get_parameter_info(PM_COMMON, buff, sizeof(buff)))){
		printf("use default parameter\n");
		memcpy(buff, &common_base_info, sizeof(common_base_info));
	}
	if(s3_flag)
		s3_fun = 1;
	arm_smccc_smc(CPU_INIT_SEC_SVC, s3_fun, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

int e2000_phy_init(void)
{
	/*调试将全部phy domain选上*/
	uint32_t phy_domain = pm_get_phy_sel_all()>>24;
	uint32_t phy_sel = pm_get_phy_sel_all()&0xffffff;
	uint32_t mode = pm_get_mac_mode_all()<<8;
	uint32_t speed = pm_get_speed_mode_all()<<8;
	struct arm_smccc_res res;

 //   if (gd->m2_flag){
 //       phy_sel |= 0x8;
 //   }
	debug("phy_sel = 0x%x\n", phy_sel);

    arm_smccc_smc(SET_PHY_MODE, phy_domain, phy_sel, mode, speed, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("set phy error ret  %lx\n", res.a0);
		while(1);
	}

	return 0;
}

#ifdef PHYTIUM_PINCTRL_TOOLS
int e2000_pin_ctrl(void)
{
	int groups, size;
	int pad_base;
	uint32_t buffer[384] = {0};//max 1.5k

	pad_base = PLAT_PAD_BASE;
	p_printf("parameter pad_base = 0x%x\n", pad_base);
	if (readl((size_t)(pad_base)) != PARAMETER_PAD_MAGIC)
	{
		p_printf("ERROR:board pad info no found!!!\n");
		return -1;
	}

    groups = readl((size_t)(pad_base + 0x4));
	size = groups * 2 * (sizeof(int));
	memcpy((void *)buffer, (void *)((size_t)(pad_base + 0x8)), size);
	for(int i=0; i<groups; i++){
		p_printf("value = 0x%x, addr = 0x%x\n", buffer[i*2+1], buffer[i*2]);
		writel(buffer[i*2+1], (u64)buffer[i*2]);
	}

    return 0;
}
#endif

void gsd_sata_setup(void)
{
	writel(0x08000000, (size_t)(GSD_SATA_BASE + SATA_CAP));
    writel(0x00000001, (size_t)(GSD_SATA_BASE + SATA_PI));
    writel(0x04000000, (size_t)(GSD_SATA_BASE + SATA_P_CMD));
}

void psu_sata_setup(void)
{
	writel(0x08000000, (size_t)(PSU_SATA_BASE + SATA_CAP));
    writel(0x00000001, (size_t)(PSU_SATA_BASE + SATA_PI));
	writel(0x04000000, (size_t)(PSU_SATA_BASE + SATA_P_CMD));
}

void usb_init_setup(void)
{
	writel(0x20031e00, 0x31a00000 + 0xa040);
	writel(0x20031e00, 0x31a20000 + 0xa040);

    //过流bypass
	/*usb 3.0*/
    writeb(0x3, 0x31a10038);
    writeb(0x3, 0x31a30038);
	/*usb 2.0*/
	writel(0x3, 0x32880038);
	mdelay(1);
    writeb(0x3, 0x3280018d);
    mdelay(1);
    writel(0x3, 0x328c0038);
    mdelay(1);
    writeb(0x3, 0x3284018d);
    mdelay(1);

    //兼容鼠标键盘
	/*3.0*/
	writel(readl(0x31a18004)|0x80000, 0x31a18004);
	writel(readl(0x31a38004)|0x80000, 0x31a38004);
	/*2.0*/
	writel(readl(0x32881004)|0x80000, 0x32881004);
	writel(readl(0x328c1004)|0x80000, 0x328c1004);

    /*usb25M 源时钟选择,对应v1.04-1的SE*/
	writel(0x0,0x3288002c);
	writel(0x0,0x328c002c);

    /*usb25M 源时钟选择,对应v1.04-1的SE*/
	writel(0x0,0x31a10020);
	writel(0x0,0x31a30020);
}

void vhub_init_setup(void)
{
	if(!(pm_get_param_mio_sel()&0x10000)){
		p_printf("vhub device set!\n");
		//隔离+device复位
		writel(0xE8, 0x31990000);
		writel(0xE8, 0x319a0000);
		writel(0xE8, 0x319b0000);
		//置device0 为 host模式
		writel(0xa, 0x31990020);
		//释放PHY复位并切换到host模式
		writel(0xd9d9, 0x319c0000);
		//释放device0 复位，产生DP、DM变化
		writel(0xe9, 0x31990000);
		//再次复位HUB和PHY到确定状态
		writel(0x41d9, 0x319c0000);
		//置device为device模式
		writel(0x26, 0x31990020);
		writel(0x26, 0x319a0020);
		writel(0x26, 0x319b0020);
		//复位device0到确定状态
		writel(0xe8, 0x31990000);
		mdelay(1);
		//切换为HUB模式，并释放HUB和PHY复位，产生DP、DM变化，使得HUB被枚举
		writel(0x59d8, 0x319c0000);
		//释放device复位
		writel(0xe9, 0x31990000);
		writel(0xe9, 0x319a0000);
		writel(0xe9, 0x319b0000);
	}else{
		p_printf("vhub host set!\n");
		writel(0xd9d9, 0x319c0000);
		writel(0x69, 0x31990000);
		writel(0x69, 0x319a0000);
		writel(0x69, 0x319b0000);

		writel(0xa, 0x31990020);
		writel(0xa, 0x319a0020);
		writel(0xa, 0x319b0020);
	}
    /*KEEP ALIVE 兼容鼠标键盘*/
	writel(readl(0x31981004)|0x80000, 0x31981004);
    /*usb25M 源时钟选择,对应v1.04-1的SE*/
    writel(0xd909, 0x319c0000);
}

void onewire_init_setup(void)
{
	/*配置onewire模式*/
	writel(0x10, 0x2803f000);
}

//识别M.2设备
void m2_check(void)
{
	int ret, cfg;
	int port = 0;

	//gpio0_14
	//读取之前配置方向寄存器
	cfg = readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));
	port = 14;
	cfg &= ~(1<<port);
	writel(cfg, (size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));

	//读取当前控io寄存器的值
	ret = readl((size_t)(GPIO0_BASE + GPIO_EXT_PORTA));
	debug("m.2/sata ret = 0x%x\n", ret);
	//判断对应io引脚是否为高电平
	if(ret & 0x4000) {
		p_printf("m.2 device\n");
		gd->m2_flag = 1;
	} else {
		p_printf("sata device\n");
		gd->m2_flag = 0;
	}
}

void gpio_reset_device(int dev_id, int level)
{
    switch (dev_id){
    case 0x0://pcie and m.2
        if (level){
            p_printf("pcie and m.2 reset high\n");
            writel((0x2001 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));	//配gpio方向控制寄存器为输出
            writel((0x2001 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DR));	//输出高电平
            mdelay(50);
        }else{
            p_printf("pcie and m.2 reset low\n");
            writel((0x2001 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));	//配gpio方向控制寄存器为输出
            writel((0xdffe & readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DR));	//输出低电平
            //mdelay(100);
        }
        break;
    case 0x1://gmac phy
        if(level){
            p_printf("gmac phy reset high\n");
            writel((0x1000 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));	//配gpio方向控制寄存器为输出
            writel((0x1000 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DR));   //输出高电平
            //mdelay(100);
            mdelay(50);
        }else{
            p_printf("gmac phy reset low\n");
            writel((0x1000 | readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DDR));	//配gpio方向控制寄存器为输出
            writel((0xefff & readl((size_t)(GPIO0_BASE + GPIO_SWPORTA_DR))), (size_t)(GPIO0_BASE + GPIO_SWPORTA_DR));	//输出低电平
            //mdelay(20);
        }
        break;
    default:
        p_printf("unknown device!!!plase check input argv!\n");
    }
}

void mio_func_sel(void)
{
	uint32_t mio_sel;

	mio_sel = pm_get_param_mio_sel();
	debug("mio_sel = 0x%x\n", mio_sel);

	for (int i=0; i<16; i++) {
		if ((mio_sel >> i) & 0x1)//uart
			writel(MIO_UART, (size_t)((MIO00 + i*0x2000) + MIO_CTRL_OFFSET + MIO_FUNC_SEL));
		else//i2c
			writel(MIO_I2C, (size_t)((MIO00 + i*0x2000) + MIO_CTRL_OFFSET + MIO_FUNC_SEL));
	}
}

void sd_mmc_hw_cfg(void)
{
	uint32_t sd_hw_cfg;

	sd_hw_cfg = pm_get_sd0_hw_cfg();
	if(sd_hw_cfg)
		writel(sd_hw_cfg, (MMCSD0_BASE + MCI_HWCONF));

	sd_hw_cfg = pm_get_sd1_hw_cfg();
	if(sd_hw_cfg)
		writel(sd_hw_cfg, (MMCSD1_BASE + MCI_HWCONF));
}
