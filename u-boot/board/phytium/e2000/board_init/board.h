// SPDX-License-Identifier: GPL-2.0+
/*
 * Board init header file for Phytium E2000 SoC
 *
 * Copyright (C) 2022, Phytium Technology Co., Ltd.
 */

#ifndef __BOARD_H_
#define __BOARD_H_
#include <linux/delay.h>

/**********************early setup smc id*************************************/
#define CPU_SVC_VERSION			0xC2000F00
#define CPU_GET_RST_SOURCE		0xC2000F01
#define CPU_INIT_PLL			0xC2000F02
#define CPU_INIT_PCIE			0xC2000F03
#define CPU_INIT_MEM			0xC2000F04
#define CPU_INIT_SEC_SVC		0xC2000F05
#define CPU_SECURITY_CFG		0xC2000F07
#define CPU_GET_MEM_INFO		0xC2000F14	

#define SET_PHY_MODE			0xC2000015

/********************************phy******************************************/
#define RMII_WD_0			0x1
#define RGMII_WD_0			0x2
#define SGMII_WD_0			0x3
#define USXGMII_WD_0		0x4
#define SGMII_WD_1			0x5
#define USXGMII_WD_1		0x6
#define GMAC_PAD_RMII		0
#define GMAC_PAD_RGMII		1

/***********************parameter check****************************************/
#define PARAMETER_CPU_MAGIC		0x54460020	//parameter cpu
#define PARAMETER_PLL_MAGIC		0x54460020	//parameter pll
#define PARAMETER_COMMON_MAGIC	0x54460013	//parameter fw comon
#define PARAMETER_PCIE_MAGIC	0x54460021	//parameter pcie

/***********************defult PEU set*****************************************/
#define CONFIG_PCI_PEU0 		0x1
#define CONFIG_PCI_PEU1         0x1  /*0x0 : disable peu1, 0x01 : enable peu1 configuartions */
/*0:单根    1:多根*/
#define CONFIG_INDEPENDENT_TREE	0x0
/*peu*/
#define X4						0x0
#define X2X1X1					0x1
#define X1X1X1X1				0x2
/*peu_psu*/
#define X1X1					0x0
#define X0X1					0x1
#define X1X0					0x2
#define X0X0					0x3

#define CONFIG_PEU0_SPLIT_MODE  X4
#define CONFIG_PEU1_SPLIT_MODE  X1X1

/* peu device mode: 0 ep, 1 rc default */
#define EP_MODE                 0x0
#define RC_MODE                 0x1
#define PEU1_OFFSET				16
#define PEU_C_OFFSET_MODE		16
#define PEU_C_OFFSET_SPEED		0
#define PEU_C_OFFSET_PORT_DISABLE   9

#define GEN3					3
#define GEN2					2
#define GEN1					1

#define GPIO_RESET_PCIE         0
#define GPIO_RESET_PHY          1

/********************SCMI*****************************************************/
#define SCMI_CH_STATUS_RES0_MASK    0xFFFFFFFE
#define SCMI_CH_STATUS_FREE_SHIFT   0
#define SCMI_CH_STATUS_FREE_WIDTH   1
#define SCMI_CH_STATUS_FREE_MASK    ((1 << SCMI_CH_STATUS_FREE_WIDTH) - 1)

#define SCMI_IS_CHANNEL_FREE(status)                    \
    (!!(((status) >> SCMI_CH_STATUS_FREE_SHIFT) & SCMI_CH_STATUS_FREE_MASK))

#define SCMI_MARK_CHANNEL_BUSY(status)  do {                \
        (status) &= ~(SCMI_CH_STATUS_FREE_MASK <<       \
            SCMI_CH_STATUS_FREE_SHIFT);     \
    } while (0)

#define SCMI_MSG_CREATE_E(_protocol, _msg_id, _token)				\
	 ((((_protocol) & 0xff) << 10) | \
	 (((_msg_id) & 0xff) << 0) |		 \
	 (((_token) & 0x3ff) << 18))

typedef struct mailbox_mem_e {
	unsigned int res_a; /* Reserved */
	volatile unsigned int status;
	unsigned long res_b; /* Reserved */
	unsigned int flags;
	volatile unsigned int len;
	unsigned int msg_header;
	unsigned int payload[];
} mailbox_mem_e_t;

#ifdef PHYTIUM_SAVE_TRAIN_DATA
void save_train_data(void);
#endif

void check_reset(void);
void pll_init(void);
void e2000_ddr_init(uint32_t s3_flag);
uint16_t i2c_read_spd_lmu_freq(uint8_t ch_enable);
void pcie_init(void);
void sec_init(uint8_t s3_flag);
void register_pfdi(void);
void core_security_level_cfg(void);
int e2000_phy_init(void);
#ifdef PHYTIUM_PINCTRL_TOOLS
int e2000_pin_ctrl(void);
#endif

void gsd_sata_setup(void);
void psu_sata_setup(void);
void usb_init_setup(void);
void vhub_init_setup(void);
void onewire_init_setup(void);
void m2_check(void);
void gpio_reset_device(int dev_id, int level);
void mio_func_sel(void);
void sd_mmc_hw_cfg(void);
#ifdef PHYTIUM_ETH_TRAIN
void phytium_eth_training(void);
#endif

#endif	/*__BOARD_H_*/
