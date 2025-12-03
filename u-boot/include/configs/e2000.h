//SPDX-License-Identifier: GPL-2.0+ */
/*
* header file for Phytium E2000 SoC
*
* Copyright (C) 2022, Phytium Technology Co., Ltd.
*/

#ifndef __E2000_H
#define __E2000_H

#define PHYS_SDRAM_1				0x80000000  /* SDRAM Bank #1 start address */
#define PHYS_SDRAM_1_SIZE			0x7b000000  
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1
#define CONFIG_SYS_INIT_SP_ADDR     (0x30c00000 + 0x1a000)

/*PCI*/
#define CONFIG_SYS_PCI_64BIT        1
#define CONFIG_PCI_SCAN_SHOW

/*LBC*/
#define PHYTIUM_LBC
#define PHYTIUM_LBC_DIRECT
#ifdef PHYTIUM_LBC
#define CONFIG_SYS_MAX_FLASH_BANKS	1	/* max number of memory banks		*/
#endif

/*NAND*/
#define PHYTIUM_NAND

/*PLAT AHCI*/
#ifdef CONFIG_PHYTIUM_AHCI
#define CONFIG_SCSI_AHCI_PLAT
#endif

/*DDR TRAIN*/
/*#define PHYTIUM_SAVE_TRAIN_DATA*/
#ifdef PHYTIUM_SAVE_TRAIN_DATA
#define TRAIN_FLAG_ADDR					(0x30c80000 - 0x4)
#define QSPI_TRAIN_DATA_REAL_ADDR		0x280000
#define LBC_TRAIN_DATA_REAL_ADDR		0x280000
#define LBC_TRAIN_DATA_MAP_ADDR			(0x10000000 + LBC_TRAIN_DATA_REAL_ADDR)
#define SD_TRAIN_DATA_REAL_ADDR			0x280000
#define SD_TRAIN_DATA_IACC_ADDR         (0x38000000  + SD_TRAIN_DATA_REAL_ADDR)
#define EMMC_TRAIN_DATA_REAL_ADDR       0x280000
#define EMMC_TRAIN_DATA_IACC_ADDR       (0x38000000 + EMMC_TRAIN_DATA_REAL_ADDR)
#define NAND_TRAIN_DATA_REAL_ADDR       0x280000
#define NAND_TRAIN_DATA_IACC_ADDR       (0x38000000 + NAND_TRAIN_DATA_REAL_ADDR)
#endif

/*use pinctrl tools*/
#define PHYTIUM_PINCTRL_TOOLS

/* BOOT */
#define CONFIG_SYS_BOOTM_LEN		(60 * 1024 * 1024)
//#define FDT_FIXUP_ETH_MAC_ADDR
#define CONFIG_EXTRA_ENV_SETTINGS	\
 					"kernel_addr=0x90100000\0"	   \
 					"initrd_addr=0x95000000\0"	   \
					"ft_fdt_addr=0x90000000\0"		\
					"ft_fdt_name=boot/dtb/e2000.dtb\0"		\
 					"load_kernel=ext4load scsi 0:2 $kernel_addr boot/uImage-2004\0"	\
 					"load_initrd=ext4load scsi 0:2 $initrd_addr initrd.img-4.19.0.e2000\0"	\
 					"load_fdt=ext4load scsi 0:2 $ft_fdt_addr $ft_fdt_name\0" \
 					"boot_os=bootm $kernel_addr -:- $ft_fdt_addr\0" \
 					"ipaddr=202.197.67.2\0"       \
 					"gatewayip=202.197.67.1\0"      \
 					"netdev=eth0\0"                 \
 					"netmask=255.255.255.0\0"       \
 					"serverip=202.197.67.3\0" 	\
                    "distro_bootcmd=run load_kernel; run load_initrd; run load_fdt; run boot_os\0"

#endif	/* __E2000_H */

