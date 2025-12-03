// SPDX-License-Identifier: GPL-2.0+
/*
 * header file  for Phytium E2000 SoC
 *
 * Copyright (C) 2022, Phytium Technology Co., Ltd.
 */
#ifndef _FT_E2000_H
#define _FT_E2000_H

/*QSPI*/
#define QSPI_BASE					0x28008000
#define QSPI_FLASH_CAPACITY_REG     (QSPI_BASE + 0x0)
#define QSPI_ADDR_READ_CFG_REG      (QSPI_BASE + 0x4)
#define QSPI_CMD_PORT_REG           (QSPI_BASE + 0x10)
#define QSPI_LD_PORT_REG            (QSPI_BASE + 0x1C)

/*QSPI FLASH*/
#define FLASH_BASE					0x38040000

/*UART*/
#define UART0_BASE					0x2800c000
#define UART1_BASE					0x2800d000
#define UART2_BASE					0x2800e000
#define UART3_BASE					0x2800f000

/*I2C/PMBUS/SMBUS*/
#define PMBUS0_BASE					0x28011000
#define PMBUS1_BASE					0x28012000
#define SMBUS_BASE					0x28013000

/*MIO*/
#define MIO00						0x28014000
#define MIO01						0x28016000
#define MIO02						0x28018000
#define MIO03						0x2801a000
#define MIO04						0x2801c000
#define MIO05						0x2801e000
#define MIO06						0x28020000
#define MIO07						0x28022000
#define MIO08						0x28024000
#define MIO09						0x28026000
#define MIO10						0x28028000
#define MIO11						0x2802a000
#define MIO12						0x2802c000
#define MIO13						0x2802e000
#define MIO14						0x28030000
#define MIO15						0x28032000

#define MIO_CTRL_OFFSET				0x1000
#define MIO_FUNC_SEL				0x00
#define MIO_FUNC_SEL_STATE			0x04
#define MIO_VERSION					0x100

#define MIO_I2C						0x00
#define MIO_UART					0x01

/*GPIO*/
#define GPIO0_BASE					0x28034000
#define GPIO1_BASE					0x28035000
#define GPIO2_BASE					0x28036000
#define GPIO3_BASE					0x28037000
#define GPIO4_BASE					0x28038000
#define GPIO5_BASE					0x28039000
#define GPIO_SWPORTA_DR				0x00
#define GPIO_SWPORTA_DDR			0x04
#define GPIO_EXT_PORTA				0x08

/*Platform SATA*/
#define PSU_SATA_BASE				0x31a40000
#define GSD_SATA_BASE				0x32014000
#define SATA_CAP					0x0
#define SATA_PI						0xC
#define SATA_P_CMD					0x118

/*MMCSD*/
#define MMCSD0_BASE					0x28000000
#define MMCSD1_BASE					0x28001000
#define MCI_HWCONF					0x70 /* the hardware configuration reg */

/*ETH*/
#define GMAC0_BASE_ADDR			0x3200c000
#define GMAC1_BASE_ADDR			0x3200e000
#define GMAC2_BASE_ADDR			0x32010000
#define GMAC3_BASE_ADDR			0x32012000
//phy*
#define GSD_PHY0_BASE			0x32100000
#define GSD_PHY1_BASE			0x32200000
#define GSD_PHY2_BASE			0x32300000
#define GSD_PHY3_BASE			0x32400000

/*SCMI*/
//MHU address space
#define MHU_BASE					(0x32a00000)
#define MHU_SCP_UBOOT_BASE			(MHU_BASE + 0x20)
#define MHU_UBOOT_SCP_BASE			(MHU_BASE + 0x120)
#define MHU_CONFIG_BASE				(MHU_BASE + 0x500)

//MHU os chanle address
#define MHU_SCP_OS_BASE				(MHU_BASE + 0x0)
#define MHU_OS_SCP_BASE				(MHU_BASE + 0x100)
#define SHARE_MEM_BASE				(0x32a10000)
#define SCP_TO_AP_OS_MEM_BASE		(0x32a10000 + 0x1000)
#define AP_TO_SCP_OS_MEM_BASE		(0x32a10000 + 0x1400)

//shared memroy base
#define SCP_TO_AP_SHARED_MEM_BASE  (0x32a10000 + 0x800)
#define AP_TO_SCP_SHARED_MEM_BASE  (0x32a10000 + 0xC00)

#endif/*_FT_E2000_H*/
