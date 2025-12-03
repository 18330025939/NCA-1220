// SPDX-License-Identifier: GPL-2.0+
/*
 * DDR init for Phytium E2000 SoC
 *
 * Copyright (C) 2022, Phytium Technology Co., Ltd.
 */
#include <common.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/gic.h>
#include <asm/armv8/mmu.h>
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include <malloc.h>
#include <e_uart.h>
#include "mcu_info.h"
#include "board.h"
#include "../cpu.h"
#include "../parameter/parameter.h"
#include "../power_manage/power_manage.h"

#ifdef PHYTIUM_SAVE_TRAIN_DATA
#include "../../../drivers/spi/phytium_qspi.h"
#include <flash.h>
#include <mmc.h>
#include <nand.h>
#endif

static void get_mcu_up_info_default(mcu_config_t *pm)
{
	pm->magic = PARAMETER_MCU_MAGIC;
	pm->version = PARAM_MCU_VERSION;
	pm->size = PARAM_MCU_SIZE;
	pm->ch_enable = PARAM_CH_ENABLE;
//	pm->ecc_enable = PARAM_ECC_ENABLE;
//	pm->force_spd_enable = PARAM_FORCE_SPD_DISABLE;
//	pm->misc_enable = PARAM_MCU_MISC_ENABLE;
	pm->train_recover = 0;
}

static void print_ddr_info(mcu_config_t *pm)
{
	uint8_t temp;
	uint16_t sdram_density;		//Mb
	uint64_t dimm_capacity;		//G

	if(pm->ddr_spd_info.dram_type == DDR4_TYPE){
		printf("  tAAmim    = %dps\n",	pm->ddr_spd_info.tAAmin);
		printf("  tRCDmin   = %dps\n",	pm->ddr_spd_info.tRCDmin);
    	printf("  tRPmin    = %dps\n",	pm->ddr_spd_info.tRPmin);
    	printf("  tRASmin   = %dps\n",	pm->ddr_spd_info.tRASmin);
    	printf("  tRCmin    = %dps\n",	pm->ddr_spd_info.tRCmin);
    	printf("  tFAWmin   = %dps\n",	pm->ddr_spd_info.tFAWmin);
        printf("  tRRD_Smin = %dps\n",	pm->ddr_spd_info.tRRD_Smin);
        printf("  tRRD_Lmin = %dps\n",	pm->ddr_spd_info.tRRD_Lmin);
        printf("  tCCD_Lmin = %dps\n",	pm->ddr_spd_info.tCCD_Lmin);
        printf("  tWRmin    = %dps\n",	pm->ddr_spd_info.tWRmin);
        printf("  tWTR_Smin = %dps\n",	pm->ddr_spd_info.tWTR_Smin);
        printf("  tWTR_Lmin = %dps\n",	pm->ddr_spd_info.tWTR_Lmin);
		printf("  tRFC1min  = %dps\n",	pm->ddr_spd_info.tRFC1min);
		printf("  tRFC2min  = %dps\n",	pm->ddr_spd_info.tRFC2min);
		printf("  tRFC4min  = %dps\n",	pm->ddr_spd_info.tRFC4_RFCsbmin);
	}else if(pm->ddr_spd_info.dram_type == DDR5_TYPE){
		printf("  tAAmim    = %dps\n",	pm->ddr_spd_info.tAAmin);
		printf("  tRCDmin   = %dps\n",	pm->ddr_spd_info.tRCDmin);
    	printf("  tRPmin    = %dps\n",	pm->ddr_spd_info.tRPmin);
    	printf("  tRASmin   = %dps\n",	pm->ddr_spd_info.tRASmin);
    	printf("  tRCmin    = %dps\n",	pm->ddr_spd_info.tRCmin);
//    	printf("  tFAWmin   = %dps\n",	pm->ddr_spd_info.tFAWmin);
//		printf("  tRRD_Smin = %dps\n",	pm->ddr_spd_info.tRRD_Smin);
//		printf("  tRRD_Lmin = %dps\n",	pm->ddr_spd_info.tRRD_Lmin);
//		printf("  tCCD_Lmin = %dps\n",	pm->ddr_spd_info.tCCD_Lmin);
        printf("  tWRmin    = %dps\n",	pm->ddr_spd_info.tWRmin);
//		printf("  tWTR_Smin = %dps\n",	pm->ddr_spd_info.tWTR_Smin);
//		printf("  tWTR_Lmin = %dps\n",	pm->ddr_spd_info.tWTR_Lmin);
		printf("  tRFC1min  = %dps\n",	pm->ddr_spd_info.tRFC1min);
		printf("  tRFC2min  = %dps\n",	pm->ddr_spd_info.tRFC2min);
		printf("  tRFCsbmin = %dps\n",	pm->ddr_spd_info.tRFC4_RFCsbmin);
	}else if(pm->ddr_spd_info.dram_type == LPDDR4_TYPE){
	}

	temp = 4<<pm->ddr_spd_info.data_width;
	if(pm->ddr_spd_info.dram_type == DDR3_TYPE){
		sdram_density =
            (1UL<<(pm->ddr_spd_info.row_num + pm->ddr_spd_info.col_num)) * temp
            * pm->ddr_spd_info.bank_num >> 20; //units: Mb
	}else{
		if(pm->ddr_spd_info.dram_type == LPDDR4_TYPE){
			sdram_density =
				(1UL<<(pm->ddr_spd_info.row_num + pm->ddr_spd_info.col_num)) * temp
				* pm->ddr_spd_info.bank_num >> 20; //units: Mb
		}else{
			sdram_density =
				(1UL<<(pm->ddr_spd_info.row_num + pm->ddr_spd_info.col_num)) * temp
				* pm->ddr_spd_info.bg_num * pm->ddr_spd_info.bank_num >> 20; //units: Mb
		}
	}

	if ((pm->misc_enable >> 13) & 0x1)
		dimm_capacity = sdram_density * pm->ddr_spd_info.rank_num * 32 / temp >> 13;   //units: GB
 	else
		dimm_capacity = sdram_density * pm->ddr_spd_info.rank_num * 64 / temp >> 13;   //units: GB

	printf("\tDimm_Capacity = %lldGB\n", dimm_capacity );
	switch(pm->ddr_spd_info.dram_type){
		case DDR3_TYPE:
			printf("\tDDR3"); break;
		case DDR4_TYPE:
			printf("\tDDR4"); break;
		case LPDDR4_TYPE:
			printf("\tLPDDR4"); break;
		case DDR5_TYPE:
			printf("\tDDR5"); break;
		default:
			printf("\tdram_type=0x%x", pm->ddr_spd_info.dram_type);
	}
	switch(pm->ddr_spd_info.dimm_type){
		case 1:
			printf("\tRDIMM"); break;
		case 2:
			printf("\tUDIMM"); break;
		case 3:
		case 9:
			printf("\tSODIMM"); break;
		case 4:
			printf("\tLRDIMM"); break;
		default:
			printf("\tdimm_type=0x%x", pm->ddr_spd_info.dimm_type);
	}
	if(pm->ddr_spd_info.dram_type == DDR4_TYPE){
		printf("/%d Bank Groups", pm->ddr_spd_info.bg_num);
		printf("/%d Banks", pm->ddr_spd_info.bank_num);
	}else{
		printf("/%d Banks", pm->ddr_spd_info.bank_num);
		printf("/sdram_density=%dGb", sdram_density >> 10);
	}
	printf("/Column %d", pm->ddr_spd_info.col_num);
	printf("/Row %d", pm->ddr_spd_info.row_num);
	switch(pm->ddr_spd_info.data_width){
		case DIMM_x4:
			printf("/X4"); break;
		case DIMM_x8:
			printf("/X8"); break;
		case DIMM_x16:
			printf("/X16"); break;
		case DIMM_x32:
			printf("/X32"); break;
		default:
			printf("/data_width=0x%x", pm->ddr_spd_info.data_width);
	}
	printf("/%d Rank", pm->ddr_spd_info.rank_num);
	switch(pm->ddr_spd_info.ecc_type){
		case 0:
			printf("/NO ECC"); break;
		case 1:
			printf("/ECC"); break;
		default:
			printf("/ecc_type=0x%x", pm->ddr_spd_info.ecc_type);
	}
	if(pm->ddr_spd_info.mirror_type == 0)
		printf("/Standard\n");
	else
		printf("/Mirror\n");
	printf("\tModual:");
	switch(pm->ddr_spd_info.module_manufacturer_id){
		case SAMSUNG_VENDOR:
			printf("Samsung"); break;
		case MICRON_VENDOR:
			printf("Micron"); break;
		case HYNIX_VENDOR:
			printf("Hynix"); break;
		case KINGSTON_VENDOR:
			printf("KingSton"); break;
		case RAMAXEL_VENDOR:
			printf("Ramaxel"); break;
		case LANQI_VENDOR:
			printf("Lanqi"); break;
		default:
			printf("Unknown=0x%x", pm->ddr_spd_info.module_manufacturer_id);
	}
}

static void set_speed(int i2c_spd, int i2c_base)
{
	unsigned int cntl;
	unsigned int hcnt, lcnt;
	struct i2c_regs *i2c_regs_p;

	i2c_regs_p = (struct i2c_regs *)((size_t)i2c_base);

	cntl = (mmio_read_32((uintptr_t)&i2c_regs_p->ic_con) & (~IC_CON_SPD_MSK));

	switch (i2c_spd) {
	case IC_SPEED_MODE_MAX:
		cntl |= IC_CON_SPD_HS;
		hcnt = (IC_CLK * MIN_HS_SCL_HIGHTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_hs_scl_hcnt , hcnt);
		lcnt = (IC_CLK * MIN_HS_SCL_LOWTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_hs_scl_lcnt, lcnt);
		break;

	case IC_SPEED_MODE_STANDARD:
		cntl |= IC_CON_SPD_SS;
		hcnt = (IC_CLK * MIN_SS_SCL_HIGHTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_ss_scl_hcnt , hcnt);
		lcnt = (IC_CLK * MIN_SS_SCL_LOWTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_ss_scl_lcnt , lcnt);
		break;

	case IC_SPEED_MODE_FAST:
	default:
		cntl |= IC_CON_SPD_FS;
		hcnt = (IC_CLK * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_fs_scl_hcnt , hcnt);
		lcnt = (IC_CLK * MIN_FS_SCL_LOWTIME) / NANO_TO_MICRO;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_fs_scl_lcnt , lcnt);
		break;
	}

	mmio_write_32((uintptr_t)&i2c_regs_p->ic_con , cntl);
}

static int i2c_set_bus_speed(int speed, int i2c_base)
{
	if (speed >= I2C_MAX_SPEED)
		set_speed(IC_SPEED_MODE_MAX, i2c_base);
	else if (speed >= I2C_FAST_SPEED)
		set_speed(IC_SPEED_MODE_FAST, i2c_base);
	else
		set_speed(IC_SPEED_MODE_STANDARD, i2c_base);

	return 0;
}

static void init_spd_i2c(void)
{
	struct i2c_regs *i2c_regs_p;
	unsigned int enbl;

	i2c_regs_p = (struct i2c_regs *)(MIO06);

	/* Disable i2c */
	enbl = mmio_read_32((uintptr_t)&i2c_regs_p->ic_enable);
	enbl &= ~IC_ENABLE_0B;
	mmio_write_32((uintptr_t)&i2c_regs_p->ic_enable , enbl);

	mmio_write_32((uintptr_t)&i2c_regs_p->ic_con ,(IC_CON_SD | IC_CON_RE | IC_CON_SPD_FS | IC_CON_MM));

	i2c_set_bus_speed(I2C_FAST_SPEED, MIO06);

	mmio_write_32((uintptr_t)&i2c_regs_p->ic_sar , 0);

	/* Enable i2c */
	enbl = mmio_read_32((uintptr_t)&i2c_regs_p->ic_enable);
	enbl |= IC_ENABLE_0B;
	mmio_write_32((uintptr_t)&i2c_regs_p->ic_enable , enbl);
}

static void spd_i2c_init(void)
{
	init_spd_i2c();
}

static int i2c_wait_for_bb(int i2c_base)
{
	struct i2c_regs *i2c_regs_p;

	i2c_regs_p = (struct i2c_regs *)((size_t)i2c_base);
	while ((mmio_read_32((uintptr_t)&i2c_regs_p->ic_status) & IC_STATUS_MA) ||
	       !(mmio_read_32((uintptr_t)&i2c_regs_p->ic_status) & IC_STATUS_TFE)) {
	}

	return 0;
}

static int i2c_xfer_init(unsigned char chip, int addr, int i2c_base)
{
	struct i2c_regs *i2c_regs_p;

	if (i2c_wait_for_bb(i2c_base))
		return 1;

	i2c_regs_p = (struct i2c_regs *)(MIO06);

	/* set chip address */
	mmio_write_32((uintptr_t)&i2c_regs_p->ic_tar, chip);

	/* set register address */
	mmio_write_32((uintptr_t)&i2c_regs_p->ic_cmd_data , addr);

	return 0;
}

int i2c_xfer_finish(int i2c_base)
{
	struct i2c_regs *i2c_regs_p;

	i2c_regs_p = (struct i2c_regs *)(MIO06);

	while (1) {
		if ((mmio_read_32((uintptr_t)&i2c_regs_p->ic_raw_intr_stat) & IC_STOP_DET)) {
			mmio_read_32((uintptr_t)&i2c_regs_p->ic_clr_stop_det);
			break;
		}
	}

	if (i2c_wait_for_bb(i2c_base)) {
		printf("Timed out waiting for bus\n");
		return 1;
	}

	/* flush_rxfifo */
	while (mmio_read_32((uintptr_t)&i2c_regs_p->ic_status) & IC_STATUS_RFNE)
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_cmd_data);

	return 0;
}

static void check_dram_type(uint8_t ch, uint8_t *buffer, mcu_config_t *mcu_ptr)
{
	uint8_t temp_val;

	printf("Check Dram Type ...\n");

	//Key Byte /DRAM Device Type
	temp_val = buffer[2] & 0xf;
	mcu_ptr->ddr_spd_info.dram_type = temp_val;
}

static void parse_spd_byte_ddr3(uint8_t *buffer, mcu_config_t *mcu_ptr)
{
	uint16_t temp_val;

	printf("Parse SPD Data ...\n");

	//Key Byte / Module Type
	temp_val = buffer[3] & 0xf;
	mcu_ptr->ddr_spd_info.dimm_type = temp_val;

//	temp_val = buffer[4] & 0xf;
//	mcu_ptr->ddr_spd_info.sdram_density = 256 * (1<<temp_val); //units: Mb

	mcu_ptr->ddr_spd_info.bank_num = 8;

	temp_val = buffer[5] & 0x7;
	mcu_ptr->ddr_spd_info.col_num = temp_val + 9;

	temp_val = buffer[5]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.row_num = temp_val + 12;

	//Module Organization
	temp_val = buffer[7] & 0x7;
	mcu_ptr->ddr_spd_info.data_width = temp_val;

	temp_val = buffer[7]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.rank_num= temp_val + 1;

	//Module Organization
	temp_val = buffer[8]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.ecc_type = temp_val;

	//(RDIMM/LRDIMM): Address Mapping from Register to DRAM
	if(mcu_ptr->ddr_spd_info.dimm_type == UDIMM_TYPE){
		temp_val = buffer[63] & 0x1;
		mcu_ptr->ddr_spd_info.mirror_type = temp_val;
	}

//	mcu_ptr->ddr_spd_info.F0RC03 = (buffer[70]>>4) & MASK_4BIT;
//	mcu_ptr->ddr_spd_info.F0RC04 = buffer[71] & MASK_4BIT;
//	mcu_ptr->ddr_spd_info.F0RC05 = (buffer[71]>>4) & MASK_4BIT;
	mcu_ptr->ddr_spd_info.module_manufacturer_id = ((uint16_t)buffer[117] << 8) + buffer[118];

/*	mcu_ptr->ddr_spd_info.dram_manufacturer_id = ((uint16_t)buffer[147] << 8) + buffer[149];

	mcu_ptr->ddr_spd_info.module_serial_number = ((uint32_t)buffer[122]<<24) + ((uint32_t)buffer[123]<<16)
												+ ((uint32_t)buffer[124]<<8) + buffer[125];
*/

	//tRCDmin
	mcu_ptr->ddr_spd_info.tRCDmin = buffer[18] * SPD_MTB;
	//tRPmin
	mcu_ptr->ddr_spd_info.tRPmin = buffer[20] * SPD_MTB;
	//tRASmin
	mcu_ptr->ddr_spd_info.tRASmin = (((((uint16_t)buffer[21]) & 0xf)<<8) + buffer[23]) * SPD_MTB;
	//tRCmin
	mcu_ptr->ddr_spd_info.tRCmin = (((((uint16_t)buffer[21]>>4) & 0xf)<<8) + buffer[23]) * SPD_MTB;
	//tFAWmin
	mcu_ptr->ddr_spd_info.tFAWmin = ((((uint16_t)buffer[28] & 0xf) <<8) + buffer[29]) * SPD_MTB;
	//tRRDmin
//	mcu_ptr->ddr_spd_info.tRRDmin = buffer[19] * SPD_MTB;
	//tWRmin
	mcu_ptr->ddr_spd_info.tWRmin = 15000;
	//tWTRmin, 7.5ns
//	mcu_ptr->ddr_spd_info.tWTRmin = 7500;

	print_ddr_info(mcu_ptr);
	temp_val = ((uint16_t)buffer[147] << 8) + buffer[149];		//dram_manufacturer_id
	printf("/Dram:");
	switch(temp_val){
		case SAMSUNG_VENDOR:
			printf("Samsung"); break;
		case MICRON_VENDOR:
			printf("Micron"); break;
		case HYNIX_VENDOR:
			printf("Hynix"); break;
		case KINGSTON_VENDOR:
			printf("KingSton"); break;
		case RAMAXEL_VENDOR:
			printf("Ramaxel"); break;
		case LANQI_VENDOR:
			printf("Lanqi"); break;
		case CXMT_VENDOR:
			printf("CXMT"); break;
		case UNILC_VENDOR:
			printf("Unilc"); break;
		default:
			printf("Unknown=0x%x", temp_val);
	}

	temp_val = ((uint32_t)buffer[122]<<24) + ((uint32_t)buffer[123]<<16)
				+ ((uint32_t)buffer[124]<<8) + buffer[125];			//module_serial_number
	printf("/Serial:0x%x\n", temp_val);
}

static void parse_spd_byte(uint8_t *buffer, mcu_config_t *mcu_ptr)
{
	uint16_t temp_val;

	printf("Parse SPD Data ...\n");

	//Key Byte / Module Type
	temp_val = buffer[3] & 0xf;
	mcu_ptr->ddr_spd_info.dimm_type = temp_val;

	//SDRAM Density and Banks
	temp_val = buffer[4]>>6 & 0x3;
	if(!temp_val)
		mcu_ptr->ddr_spd_info.bg_num = 0x0;
	else
		mcu_ptr->ddr_spd_info.bg_num = 0x1<<temp_val;


	temp_val = buffer[4]>>4 & 0x3;
	if(temp_val == 1)
		mcu_ptr->ddr_spd_info.bank_num = 8;
	else
		mcu_ptr->ddr_spd_info.bank_num = 4;

//	temp_val = buffer[4] & 0xf;
//	mcu_ptr->ddr_spd_info.sdram_density = 256 * (1<<temp_val); //units: Mb	//可以算出来,不用读了

	//SDRAM Addressing
	temp_val = buffer[5] & 0x7;
	mcu_ptr->ddr_spd_info.col_num = temp_val + 9;

	temp_val = buffer[5]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.row_num = temp_val + 12;

	//Module Organization
	temp_val = buffer[12] & 0x7;
	mcu_ptr->ddr_spd_info.data_width = temp_val;

	temp_val = buffer[12]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.rank_num= temp_val + 1;

	//Module Organization
	temp_val = buffer[13]>>3 & 0x7;
	mcu_ptr->ddr_spd_info.ecc_type = temp_val;

	//(Registered): Address Mapping from Register to DRAM
	if((mcu_ptr->ddr_spd_info.dimm_type == RDIMM_TYPE)
		|| (mcu_ptr->ddr_spd_info.dimm_type == LRDIMM_TYPE) )
		temp_val = buffer[136] & 0x1;
	else//(Unbuffered): Address Mapping from Edge Connector to DRAM
		temp_val = buffer[131] & 0x1;
	mcu_ptr->ddr_spd_info.mirror_type = temp_val;

	mcu_ptr->ddr_spd_info.F0RC03 = (buffer[137]>>4) & MASK_4BIT;
	mcu_ptr->ddr_spd_info.F0RC04 = buffer[137] & MASK_4BIT;
	mcu_ptr->ddr_spd_info.F0RC05 = buffer[138] & MASK_4BIT;

/*for LRDIMM*/
//	mcu_ptr->ddr_spd_info.vrefDQ_PR0 = buffer[140];
//	mcu_ptr->ddr_spd_info.vrefDQ_MDRAM = buffer[144];
//	mcu_ptr->ddr_spd_info.RTT_MDRAM_1866 = buffer[145];
//	mcu_ptr->ddr_spd_info.RTT_MDRAM_2400 = buffer[146];
//	mcu_ptr->ddr_spd_info.RTT_MDRAM_3200 = buffer[147];
//	mcu_ptr->ddr_spd_info.Drive_DRAM = buffer[148];
//	mcu_ptr->ddr_spd_info.ODT_DRAM_1866 = buffer[149];
//	mcu_ptr->ddr_spd_info.ODT_DRAM_2400 = buffer[150];
//	mcu_ptr->ddr_spd_info.ODT_DRAM_3200 = buffer[151];
//	mcu_ptr->ddr_spd_info.PARK_DRAM_1866 = buffer[152];
//	mcu_ptr->ddr_spd_info.PARK_DRAM_2400 = buffer[153];
//	mcu_ptr->ddr_spd_info.PARK_DRAM_3200 = buffer[154];

	mcu_ptr->ddr_spd_info.rcd_num = (uint16_t)buffer[256];

	mcu_ptr->ddr_spd_info.module_manufacturer_id = ((uint16_t)buffer[320] << 8) + buffer[321];

/*
	mcu_ptr->ddr_spd_info.dram_manufacturer_id = ((uint16_t)buffer[350] << 8) + buffer[351];

	mcu_ptr->ddr_spd_info.module_serial_number = ((uint32_t)buffer[325]<<24) + ((uint32_t)buffer[326]<<16)
												+ ((uint32_t)buffer[327]<<8) + buffer[328];
*/
	//tAAmin , buffer[123] may be positive/negative
	mcu_ptr->ddr_spd_info.tAAmin = buffer[24] * SPD_MTB + (int8_t)buffer[123] * SPD_FTB;
	//tRCDmin , buffer[122] may be positive/negative
	mcu_ptr->ddr_spd_info.tRCDmin = buffer[25] * SPD_MTB + (int8_t)buffer[122] * SPD_FTB;
	//tRPmin
	mcu_ptr->ddr_spd_info.tRPmin = buffer[26] * SPD_MTB + (int8_t)buffer[121] * SPD_FTB;
	//tRASmin
	mcu_ptr->ddr_spd_info.tRASmin = (((((uint16_t)buffer[27]) & 0xf)<<8) + buffer[28]) * SPD_MTB;
	//tRCmin
	mcu_ptr->ddr_spd_info.tRCmin = (((((uint16_t)buffer[27]>>4) & 0xf)<<8) + buffer[29]) * SPD_MTB + (int8_t)buffer[120] * SPD_FTB;
	//tFAWmin
	mcu_ptr->ddr_spd_info.tFAWmin = ((((uint16_t)buffer[36] & 0xf) <<8) + buffer[37]) * SPD_MTB;
	//tRDD_Smin
	mcu_ptr->ddr_spd_info.tRRD_Smin = buffer[38] * SPD_MTB + (int8_t)buffer[119] * SPD_FTB;
	//tRDD_Lmin
	mcu_ptr->ddr_spd_info.tRRD_Lmin = buffer[39] * SPD_MTB + (int8_t)buffer[118] * SPD_FTB;
	//tCCD_Lmin
	mcu_ptr->ddr_spd_info.tCCD_Lmin = buffer[40] * SPD_MTB + (int8_t)buffer[117] * SPD_FTB;
	//tWRmin
	if((buffer[42] == 0x0) && (buffer[41] == 0x0) ){
		printf("Error! spd byte42 = 0\n");
		mcu_ptr->ddr_spd_info.tWRmin = 15000;
	}else
		mcu_ptr->ddr_spd_info.tWRmin = buffer[41] * SPD_MTB * 256 + buffer[42] * SPD_MTB;
	//tWTR_Smin, 2.5ns
	if( (buffer[43] == 0) && (buffer[44] == 0) ){
		printf("Error! spd byte43 = 0\n");
		mcu_ptr->ddr_spd_info.tWTR_Smin = 2500;
	}else
		mcu_ptr->ddr_spd_info.tWTR_Smin = ((((uint16_t)buffer[43] & 0xf) << 8) + buffer[44]) * SPD_MTB;
	//tWTR_Lmin, 7.5ns
	if( (buffer[43] == 0) && (buffer[45] == 0) )
		mcu_ptr->ddr_spd_info.tWTR_Lmin = 7500;
	else
		mcu_ptr->ddr_spd_info.tWTR_Lmin = ((((uint16_t)buffer[43] >> 4) & 0xf) + buffer[45]) * SPD_MTB;

	//tRFC1min
		mcu_ptr->ddr_spd_info.tRFC1min = (((uint16_t)buffer[31]<<8) + buffer[30]) * SPD_MTB;
	//tRFC2min
		mcu_ptr->ddr_spd_info.tRFC2min = (((uint16_t)buffer[33]<<8) + buffer[32]) * SPD_MTB;
	//tRFC4min
		mcu_ptr->ddr_spd_info.tRFC4_RFCsbmin = (((uint16_t)buffer[35]<<8) + buffer[34]) * SPD_MTB;

	print_ddr_info(mcu_ptr);
	temp_val = ((uint16_t)buffer[350] << 8) + buffer[351];	//dram_manufacturer_id
	printf("/Dram:");
	switch(temp_val){
		case SAMSUNG_VENDOR:
			printf("Samsung"); break;
		case MICRON_VENDOR:
			printf("Micron"); break;
		case HYNIX_VENDOR:
			printf("Hynix"); break;
		case KINGSTON_VENDOR:
			printf("KingSton"); break;
		case RAMAXEL_VENDOR:
			printf("Ramaxel"); break;
		case LANQI_VENDOR:
			printf("Lanqi"); break;
		case CXMT_VENDOR:
			printf("CXMT"); break;
		case UNILC_VENDOR:
			printf("Unilc"); break;
		default:
			printf("Unknown=0x%x", temp_val);
	}
	temp_val = ((uint32_t)buffer[325]<<24) + ((uint32_t)buffer[326]<<16)
				+ ((uint32_t)buffer[327]<<8) + buffer[328];				//module_serial_number
	printf("/Serial:0x%x\n", temp_val);
}

static int spd_i2c_read(uint8_t chip, int addr, uint8_t *val, int page_num)
{
	struct i2c_regs *i2c_regs_p;
	unsigned char *buffer = (unsigned char *)val;
	uint32_t i;
	uint32_t cnt = 0;

	if(page_num == 1)
		buffer = val + 256;

	if (i2c_xfer_init(chip, addr, MIO06))
		return 1;

	i2c_regs_p = (struct i2c_regs *)(MIO06);

	for(i = 0; i < SPD_NUM; i++ )
	{
		cnt = 0;
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_cmd_data , IC_CMD | IC_STOP);
		while (!(mmio_read_32((uintptr_t)&i2c_regs_p->ic_status) & IC_STATUS_RFNE)) {
			cnt++;
			if(cnt >= 5000)
				break;
		}
		if(cnt >= 5000){
			if(mmio_read_32((uintptr_t)&i2c_regs_p->ic_raw_intr_stat) & IC_TX_ABRT)
				mmio_read_32((uintptr_t)&i2c_regs_p->ic_clr_tx_abrt);
			return 1;
		}

		*(buffer + i) = (unsigned char)mmio_read_32((uintptr_t)&i2c_regs_p->ic_cmd_data);
		if(i == 0){
			if(((*val & 0x70) == 0x10) || ((*val & 0x70) == 0x20)){
			}else{
				printf("spd BYTE0 = 0x%x\n", *buffer);
				return 1;
			}
		}
	}

	return i2c_xfer_finish(MIO06);
}

static void spd_setpage(int i2c_base, int page_num)
{
	struct i2c_regs *i2c_regs_p;

	i2c_regs_p = (struct i2c_regs *)(MIO06);

	if(page_num == 0x1)
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_tar , 0x37);
	else
		mmio_write_32((uintptr_t)&i2c_regs_p->ic_tar , 0x36);

	mmio_write_32((uintptr_t)&i2c_regs_p->ic_cmd_data , 0x0);
	mmio_write_32((uintptr_t)&i2c_regs_p->ic_cmd_data , 0x0|IC_STOP);

	/*******************For NoAck Response************************/

	while(mmio_read_32((uintptr_t)&i2c_regs_p->ic_status) != 0x6);

	if(mmio_read_32((uintptr_t)&i2c_regs_p->ic_intr_stat) & 0x40){
		volatile int i = 0;
		for(i = 0; i < 100000; i++);
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_intr_stat);
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_status);
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_clr_tx_abrt);
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_intr_stat);
		mmio_read_32((uintptr_t)&i2c_regs_p->ic_status);
	}
	/**************************************************************/
}

#if 0
int dimm_probe_two_slot(uint8_t ch, uint8_t *temp,mcu_config_t *p)
{
    uint8_t slave_addr[2] = {LMU0_I2C_SLAVE_ADDR_1, LMU1_I2C_SLAVE_ADDR_1};

    //init MIO06_I2C
	spd_i2c_init();

	printf("Read Spd Begin...\n");
	/* read page 0 data*/
	if(spd_i2c_read(slave_addr[ch], 0, temp, 0))
			return 1;

	check_dram_type(ch, temp, p);

	if(p->ddr_spd_info.dram_type == DDR3_TYPE){
	}else{
			/*
			 * according to <JEDEC Standard No.21-C>, divied to 2 page
			 */
			spd_setpage(MIO06, 1); /* set to page 1 */

			/* read page 1 data*/
			spd_i2c_read(slave_addr[ch], 0, temp, 1);

			//parse_spd_byte(temp, mcu_ptr);

			/* must rechanged to page 0 */
			spd_setpage(MIO06, 0); /* set to page 0 */
	}

        return 0;
}

int two_slot_en(uint8_t ch,mcu_config_t *mcu_ptr,uint8_t *temp_1)
{
	uint16_t temp_val;
	uint8_t buff[9]={0};
	int flag = 0;

	temp_val = temp_1[3] & 0xf;
	if(temp_val == mcu_ptr->ddr_spd_info.dimm_type)
		buff[0] = 1;

	temp_val = temp_1[4]>>6 & 0x3;
	if(!temp_val)
		temp_val = 0x0;
	else
		temp_val = 0x1<<temp_val;
	if(temp_val == mcu_ptr->ddr_spd_info.bg_num)
		buff[1] = 1;

	temp_val = temp_1[4]>>4 & 0x3;
	if(temp_val == 1)
		temp_val = 8;
	else
		temp_val = 4;
	if(temp_val == mcu_ptr->ddr_spd_info.bank_num)
		buff[2] = 1;

	temp_val = temp_1[5] & 0x7;
	temp_val = temp_val + 9;
	if(temp_val == mcu_ptr->ddr_spd_info.col_num)
		buff[3] = 1;

	temp_val = temp_1[5]>>3 & 0x7;
	temp_val = temp_val + 12;
	if(temp_val == mcu_ptr->ddr_spd_info.row_num)
		buff[4] = 1;

	temp_val = temp_1[12] & 0x7;
	if(temp_val == mcu_ptr->ddr_spd_info.data_width)
		buff[5] = 1;

	temp_val = temp_1[12]>>3 & 0x7;
	temp_val = temp_val + 1;
	if(temp_val == mcu_ptr->ddr_spd_info.rank_num)
		buff[6] = 1;

	temp_val = temp_1[13]>>3 & 0x7;
	if(temp_val == mcu_ptr->ddr_spd_info.ecc_type)
		buff[7] = 1;

	if((mcu_ptr->ddr_spd_info.dimm_type == RDIMM_TYPE)
		|| (mcu_ptr->ddr_spd_info.dimm_type == LRDIMM_TYPE) )
		temp_val = temp_1[136] & 0x1;
	else//(Unbuffered): Address Mapping from Edge Connector to DRAM
		temp_val = temp_1[131] & 0x1;
	if(temp_val == mcu_ptr->ddr_spd_info.mirror_type)
		buff[8] =1;

	for(int i =0;i<sizeof(buff)/sizeof(uint8_t);i++)
	{
		if(buff[i] == 0){
			printf("The memory on CH :%d are different!\n",ch);
			flag = 1;
			break;
		}

	}
	if(flag != 1)
	{
		printf("one channel driven two slot enable!\n");
		mcu_ptr->misc3_enable = mcu_ptr->misc3_enable | 0x10;
	}

	return 0;
}
#endif

int dimm_probe(uint8_t ch, mcu_config_t *mcu_ptr)
{
	uint8_t temp[SPD_NUM*2] = {0};
	mcu_config_t *p = NULL;
	uint8_t slave_addr = 0x50;

	memcpy(p,mcu_ptr,sizeof(mcu_config_t));
	//init MIO06_I2C
	spd_i2c_init();

	printf("Read Spd Begin...\n");
	/* read page 0 data*/
	if(spd_i2c_read(slave_addr, 0, temp, 0))
		return 1;

	check_dram_type(ch, temp, mcu_ptr);

	if(mcu_ptr->ddr_spd_info.dram_type == DDR3_TYPE){
		parse_spd_byte_ddr3(temp, mcu_ptr);
	}else{
		/*
		 * according to <JEDEC Standard No.21-C>, divied to 2 page
		 */
		spd_setpage(MIO06, 1); /* set to page 1 */

		/* read page 1 data*/
		spd_i2c_read(slave_addr, 0, temp, 1);

		parse_spd_byte(temp, mcu_ptr);

		/* must rechanged to page 0 */
		spd_setpage(MIO06, 0); /* set to page 0 */
	}

    //dimm_probe_two_slot(ch,temp_1,p);
	//two_slot_en(ch,mcu_ptr,temp_1);

	return 0;
}

uint16_t parse_spd_freq(uint8_t *buffer)
{
	//spdÊÖ²á page25 byte[17]
	uint16_t tCKAVG_min, freq_val;
	tCKAVG_min = buffer[18] * SPD_MTB + (int8_t)buffer[125] * SPD_FTB;
//	printf("tCKAVG_min: %d (ps)\n", tCKAVG_min);
	freq_val = 1000000 / tCKAVG_min;

	return freq_val;
}

uint16_t pll_to_lmu_freq(uint32_t freq)
{
	uint32_t val_3, val_2, val_1, val;

	val_3 = (freq * 3 -1) / 200 ;
	val_2 = ((freq * 3 -1) % 200)?1:0;
	val_1 = val_3 + val_2;
	val = val_1 * 200 / 3 + (val_1 * 200 % 3)/2;

	return val;
}

uint16_t i2c_read_spd_lmu_freq(uint8_t ch_enable)
{
	uint16_t freq0=0, freq1=0, ret=0;
	uint8_t temp[SPD_NUM*2] = {0};
	uint8_t slave_addr = 0x50;

	//init MIO06_I2C0
	spd_i2c_init();

	if(ch_enable&0x1){
		printf("\tread dimm0 spd freq\n");

		/* read page 0 data*/
		ret = spd_i2c_read(slave_addr, 0, temp, 0);
		if(ret){
			printf("\tdimm 0 don't probed\n");
		}else{
			freq0 = pll_to_lmu_freq(parse_spd_freq(temp)/2);
			printf("\tfreq0: %d\n", freq0);
		}
	}
	if((!freq0) && (!freq1))
		ret = 0;
	else if(freq0 && freq1)
		ret = freq0 > freq1 ? freq1 : freq0;
	else if(freq0)
		ret = freq0;
	else if(freq1)
		ret = freq1;

	printf("\tspd freq: %d\n", ret);

	return ret;
}

static uint8_t init_dimm_param(uint8_t ch, mcu_config_t *pm)
{
#ifdef MCU_USE_MANUAL
	/*Fist: Manually config parameters*/
	printf("manual config dimm info...\n");
	pm->ddr_spd_info.dimm_type = UDIMM_TYPE;
	pm->ddr_spd_info.data_width = DIMM_x8;
	pm->ddr_spd_info.mirror_type = NO_MIRROR;
	pm->ddr_spd_info.ecc_type = NO_ECC_TYPE;
	pm->ddr_spd_info.dram_type = DDR4_TYPE;
	pm->ddr_spd_info.rank_num = 1;
	pm->ddr_spd_info.row_num = 16;
	pm->ddr_spd_info.col_num = 10;
	pm->ddr_spd_info.bg_num = 4;
	pm->ddr_spd_info.bank_num = 4;
	pm->ddr_spd_info.tAAmin = 13125;
	pm->ddr_spd_info.tRCDmin = 13125;
	pm->ddr_spd_info.tRPmin = 13125;
	pm->ddr_spd_info.tRASmin = 24125;
	pm->ddr_spd_info.tRCmin = 45750;
	pm->ddr_spd_info.tFAWmin = 21000;
	pm->ddr_spd_info.tRRD_Smin = 4375;
	pm->ddr_spd_info.tRRD_Lmin = 5625;
	pm->ddr_spd_info.tCCD_Lmin = 5000;
	pm->ddr_spd_info.tWRmin = 15000;
	pm->ddr_spd_info.tWTR_Smin = 2500;
	pm->ddr_spd_info.tWTR_Lmin = 7500;
	print_ddr_info(ch, pm);
	printf("\n");
#else
	/*Second:Read parameter table*/
	if((((pm->misc_enable)>>0)&1) == 1){	/*If choice force spd,then in the previous step directly from the parameter table read*/
		printf("force spd, dimm info is from parm\n");
		print_ddr_info(pm);
		printf("\n");
	}else{						/*read spd*/
		printf("dimm info from spd, read spd...\n");
		if(dimm_probe(ch, pm))
			return 1;
	}
#endif

	return 0;
}

void get_default_mcu_info(uint8_t *data)
{
	get_mcu_up_info_default((mcu_config_t *)data);
}

void fix_mcu_info(uint8_t *data, uint8_t s3_flag)
{
	uint8_t ch;
	mcu_config_t *mcu_info = (mcu_config_t *)data;

	for (ch=0; ch<2; ch++) {
		if(!((mcu_info->ch_enable>>ch)&0x1))
			continue;

		printf("  CH %d : \n", ch);
		if (init_dimm_param(ch, mcu_info)) {
			printf("DIMM %d Don't Probed!\n", ch);
				mcu_info->ch_enable &= ~(0x1<<ch);
		} else {
			if(!mcu_info->misc_enable>>1)
				printf("set ecc disable!\n");
		}
	}

	if(s3_flag)
		mcu_info->train_recover = 0x1;        //由参数表中配置和s3_flag决定
}

static secure_config_t const secure_base_info = {
	.misc_enable	= 0x0,
};

void tzc_aes_init(void)
{
	//tzc aes init
	uint8_t buff[0x100];
	struct arm_smccc_res res;

	//从打包工具里获取配置的tzc aes信息
	uint64_t secure_addr = O_PARAMETER_BASE + PM_SECURE_OFFSET;
	memcpy(buff, (void *)secure_addr, sizeof(secure_base_info));

	arm_smccc_smc(CPU_INIT_MEM, 6, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

#ifdef PHYTIUM_SAVE_TRAIN_DATA
int ddr_init(uint8_t s3_flag, u64 flash_addr)
#else
void ddr_init(uint8_t s3_flag)
#endif
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;

	if((get_parameter_info(PM_MCU, buff, sizeof(buff)))){ //从参数表中获取参数
		p_printf("use default parameter\n");
		get_default_mcu_info(buff);
	}

	fix_mcu_info(buff, s3_flag);//修正参数值

#ifdef PHYTIUM_SAVE_TRAIN_DATA
	mcu_config_t *mcu_info = (mcu_config_t *)buff;//debug
	if (s3_flag == 0) {
		if(flash_addr != 0) {
			arm_smccc_smc(CPU_INIT_MEM, 0xa, (u64)buff, flash_addr, 0, 0, 0, 0, &res);
		} else {
			arm_smccc_smc(CPU_INIT_MEM, 0, (u64)buff, flash_addr, 0, 0, 0, 0, &res);
		}
	} else {
		mcu_info->train_recover = s3_flag;
		arm_smccc_smc(CPU_INIT_MEM, 0, (u64)buff, flash_addr, 0, 0, 0, 0, &res);
	}
#else
	arm_smccc_smc(CPU_INIT_MEM, 0, (u64)buff, 0, 0, 0, 0, 0, &res);
#endif
	if(0 != res.a0){
		printf("error x0: 0x%lx, x1: 0x%lx\n", res.a0, res.a1);
		uint8_t i;
		for(i=0; i<8; i++){
			if(((res.a1>>24)&0xff) & (1<<i)){
				printf("ch %d ", i);
				break;
			}
		}

		switch(res.a1&0xffffff) {
			case 0x1:
				printf("training fail!\n");
				break;
			default:
				printf("error code: 0x%lx\n", res.a1&0xffffff);
				break;
		}
		while(1);
	}

	if(s3_flag){
		//vtt enable
		mdelay(10);
		//pwr_vtt_enable();
		mdelay(40);
		arm_smccc_smc(CPU_INIT_MEM, 3, 0, 0, 0, 0, 0, 0, &res);
	}

#ifdef PHYTIUM_SAVE_TRAIN_DATA
	if((s3_flag == 0) && (flash_addr != 0)){
		arm_smccc_smc(CPU_INIT_MEM, 0xb, 0, 0, 0, 0, 0, 0, &res);//channel 0
		if(0 != res.a1){
			printf("error:ddr bist failed! x0:%ld, x1:0x%lx, reinit!\n",res.a0, res.a1);
			return -1;
		}
	}

	return 0;
#endif
}

#ifdef PHYTIUM_SAVE_TRAIN_DATA
/*save train data to temp memory*/
static void get_train_data(uint32_t head)
{
	unsigned long train_data_addr;
	unsigned long train_data_addr_t = 0x90000000;
	struct arm_smccc_res res;

	arm_smccc_smc(CPU_GET_MEM_INFO, 0, 0, 0, 0, 0, 0, 0, &res);//返回train data所在的地址
	if(0 != res.a0){
		train_data_addr = res.a0;
		u64 train_data_size = readl((u64)train_data_addr);
		writel(head, train_data_addr_t);
		memcpy((void *)(train_data_addr_t + sizeof(head)), (void *)train_data_addr, train_data_size + sizeof(train_data_size));
		writel(head, TRAIN_FLAG_ADDR);
	}else{
		p_printf("error:get train data addr from pbf failed! x0: %d\n", res.a0);
	}
}

int save_train_data_nand(uint64_t src_addr, uint64_t des_addr, int data_size)
{
	int ret;
	u32 length;
	u32 blk_count;
	u32 blk_size = 0x20000;

	if (data_size <= 0x20000) {//128k
		blk_count = 1;
	} else {
		if ((data_size%0x20000)>0) {
			blk_count = data_size/0x20000+1;
		} else {
			blk_count = data_size/0x20000;
		}
	}
	length = blk_size * blk_count;

	ret = ftnand_erase_skip_block(des_addr, length, 0);
    if(ret != 0)
    {
        printf("ftnand_erase_skip_block is error\n");
        return -1;
    }

    ret = ftnand_skip_write(des_addr, length, src_addr, 0);
    if(ret != 0)
    {
        printf("ftnand_skip_write is error\n");
        return -1;
    }

	return 0;
}

void save_train_data_mmc(int mmc_num, uint64_t src_addr, uint64_t des_addr, int data_size)
{
	struct mmc *mmc;
	u32 blk, cnt, n;
	void *addr;
	int blk_size = 512;//bytes
	int curr_device = mmc_num;

	addr = (void *)src_addr;
	blk = des_addr/blk_size;//6M
	if((data_size % blk_size)>0)
		cnt = data_size/blk_size + 1;
	else
		cnt = data_size/blk_size;

	mmc = init_mmc_device(curr_device, false);
	printf("curr_device = %d\n", curr_device);
	if (!mmc)
		printf("Save ddr trainning data to mmc failed! No mmc device!!!\n");

	printf("\nMMC write: dev # %d, block # %d, count %d ... ", curr_device, blk, cnt);

	if (mmc_getwp(mmc) == 1)
		printf("Error: card is write protected!\n");

	n = blk_dwrite(mmc_get_blk_desc(mmc), blk, cnt, addr);
	printf("%d blocks written: %s\n", n, (n == cnt) ? "OK" : "ERROR");
}

void save_train_data(void)
{
	unsigned long train_data_addr_t = 0x90000000;
	uint32_t head = 0x524444;//DDR
	int ret, ret_t, boot_media_type;
	int train_data_size, save_data_size;

	if(readl(TRAIN_FLAG_ADDR) != head){
		p_printf("It`s not the first time to power on!\n");
	}else{
		boot_media_type = readl((size_t)(SHARE_MEM_BASE + 0x180c));
		train_data_size = readl(train_data_addr_t + sizeof(head));
		switch (boot_media_type & 0x7) {
		case 0x1://qspi
			p_printf("boot media is qspi!\n");
			ret = readl(QSPI_TRAIN_DATA_REAL_ADDR);
			ret_t = memcmp((void *)(train_data_addr_t + sizeof(head)), (void *)(QSPI_TRAIN_DATA_REAL_ADDR + sizeof(head)), train_data_size + sizeof(train_data_size));
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			if((ret != head) || (ret_t != 0)){
				qspi_erase_block(QSPI_TRAIN_DATA_REAL_ADDR);
				qspi_page_program(train_data_addr_t, QSPI_TRAIN_DATA_REAL_ADDR, save_data_size);
			}
			break;
		case 0x2://lbc
			p_printf("boot media is lbc!\n");
			ret = readl(LBC_TRAIN_DATA_MAP_ADDR);
			ret_t = memcmp((void *)(train_data_addr_t + sizeof(head)), (void *)(LBC_TRAIN_DATA_MAP_ADDR + sizeof(head)), train_data_size + sizeof(train_data_size));
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			if((ret != head) || (ret_t != 0)){
				e2000_lbc_clock_setup();
				e2000_lbc_erase_sector(LBC_TRAIN_DATA_REAL_ADDR, 0x20000);
				e2000_lbc_write_word(train_data_addr_t, LBC_TRAIN_DATA_REAL_ADDR, save_data_size);
			}
			break;
		case 0x3://nand
			p_printf("boot media is nand!\n");
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			ftnand_test_init_entry();
			save_train_data_nand(train_data_addr_t, NAND_TRAIN_DATA_REAL_ADDR, save_data_size);
			break;
		case 0x4://sd0
			p_printf("boot media is sd0!\n");
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			save_train_data_mmc(0, train_data_addr_t, SD_TRAIN_DATA_REAL_ADDR, save_data_size);
			break;
		case 0x5://sd1
			p_printf("boot media is sd1!\n");
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			save_train_data_mmc(1, train_data_addr_t, SD_TRAIN_DATA_REAL_ADDR, save_data_size);
			break;
		case 0x6://mmc0
			p_printf("boot media is mmc0!\n");
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			save_train_data_mmc(0, train_data_addr_t, EMMC_TRAIN_DATA_REAL_ADDR, save_data_size);
			break;
		case 0x7://mmc1
			p_printf("boot media is mmc1!\n");
			save_data_size = train_data_size + sizeof(head) + sizeof(train_data_size);
			save_train_data_mmc(1, train_data_addr_t, EMMC_TRAIN_DATA_REAL_ADDR, save_data_size);
			break;
		default:
			p_printf("Save ddr trainning data failed!unknown boot media type!!!plase check dial switch!!!\n");
			break;
		}
	}
}
#endif

void e2000_ddr_init(uint32_t s3_flag)
{
#ifdef PHYTIUM_SAVE_TRAIN_DATA
	uint32_t head = 0x524444;//DDR
	int ret, boot_media_type;
	u64 train_data_addr = 0;

	boot_media_type = readl((size_t)(SHARE_MEM_BASE + 0x180c));
	switch (boot_media_type & 0x7) {
	case 0x1://qspi
		train_data_addr = QSPI_TRAIN_DATA_REAL_ADDR;
		break;
	case 0x2://lbc
		train_data_addr = LBC_TRAIN_DATA_MAP_ADDR;
		break;
	case 0x3://nand
		train_data_addr = NAND_TRAIN_DATA_IACC_ADDR;
		break;
	case 0x4://sd0
		train_data_addr = SD_TRAIN_DATA_IACC_ADDR;
		break;
	case 0x5://sd1
		train_data_addr = SD_TRAIN_DATA_IACC_ADDR;
		break;
	case 0x6://mmc0
		train_data_addr = EMMC_TRAIN_DATA_IACC_ADDR;
		break;
	case 0x7://mmc1
		train_data_addr = EMMC_TRAIN_DATA_IACC_ADDR;
		break;
	default:
		p_printf("unknown boot media type!!!plase check dial switch!!!\n");
		break;
	}

	if(readl(train_data_addr) != head){
		ddr_init(s3_flag, 0);
		get_train_data(head);
	}else if(s3_flag){
		ddr_init(s3_flag, train_data_addr + sizeof(head));
	}else{
		ret = ddr_init(s3_flag, train_data_addr + sizeof(head));
		if(0 != ret){
			ddr_init(s3_flag, 0);
			get_train_data(head);
		}
	}
#else
	ddr_init(s3_flag);
#endif
#if 0
    tzc_aes_init();
#endif
}
