// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium Nand flash driver
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#include <stdio.h>
#include <log.h>
#include <string.h>
#include <linux/delay.h>
#include <command.h>
#include <asm/io.h>
#include <rand.h>
#include <exports.h>
#include "e2000_nand.h"

#ifndef PHYTIUM_NAND
#  error  must have PHYTIUM_NAND
#endif

const u16 ftnand_timing_asy_mode0[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x28, 0x28, 0x03, 0x03, 0x06, 0x06, 0x28, 0x70, 0x30, 0x50};
const u16 ftnand_timing_asy_mode1[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x14, 0x14, 0x03, 0x03, 0x06, 0x06, 0x14, 0x70, 0x20, 0x28};
const u16 ftnand_timing_asy_mode2[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x0D, 0x0D, 0x03, 0x03, 0x06, 0x06, 0x0D, 0x70, 0x20, 0x1A};
const u16 ftnand_timing_asy_mode3[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x0A, 0x0A, 0x03, 0x03, 0x06, 0x06, 0x0A, 0x70, 0x20, 0x14};
const u16 ftnand_timing_asy_mode4[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x08, 0x08, 0x03, 0x03, 0x06, 0x06, 0x08, 0x70, 0x15, 0x10};
const u16 ftnand_timing_asy_mode5[FNAND_TIMING_ASY_NUM] = {0x03, 0x03, 0x07, 0x07, 0x03, 0x03, 0x06, 0x06, 0x07, 0x20, 0x15, 0x0E};

const u16 ftnand_timing_syn_mode0[FNAND_TIMING_SYN_NUM] = {0x20, 0x41, 0x05, 0x20, 0x10, 0x19, 0x62, 0x40, 0x38, 0x20, 0x00, 0x09, 0x50, 0x20};
const u16 ftnand_timing_syn_mode1[FNAND_TIMING_SYN_NUM] = {0x18, 0x32, 0x06, 0x18, 0x0C, 0x10, 0x76, 0x40, 0x2A, 0x18, 0x00, 0x12, 0x24, 0x18};
const u16 ftnand_timing_syn_mode2[FNAND_TIMING_SYN_NUM] = {0x10, 0x0A, 0x04, 0x10, 0x08, 0x0A, 0x6E, 0x50, 0x1D, 0x10, 0x00, 0x0C, 0x18, 0x10};
const u16 ftnand_timing_syn_mode3[FNAND_TIMING_SYN_NUM] = {0x0C, 0x1A, 0x02, 0x0C, 0x06, 0x08, 0x78, 0x7C, 0x15, 0x0C, 0x00, 0x08, 0x12, 0x0C};
const u16 ftnand_timing_syn_mode4[FNAND_TIMING_SYN_NUM] = {0x08, 0x17, 0x05, 0x08, 0x04, 0x01, 0x73, 0x40, 0x0C, 0x08, 0x00, 0x06, 0x0C, 0x10};

const u16 ftnand_timing_tog_ddr_mode0[FNAND_TIMING_TOG_NUM] = {0x14, 0x0a, 0x08, 0x08, 0xc8, 0xc8, 0x08, 0x08, 0x14, 0x0a, 0x14, 0x08}; /* 600M clk */

const struct ftnand_manufacturer_ops toshiba_ops;
const struct ftnand_manufacturer_ops micron_ops;
const struct ftnand_manufacturer_ops skhynix_ops;


static const ftnand_manufacturer ftnand_manufacturers[] = {
    {NAND_MFR_TOSHIBA, "Toshiba", &toshiba_ops},
    {NAND_MFR_MICRON, "micron", &micron_ops},
    {NAND_MFR_SKHYNIX,"skhynix",&skhynix_ops},
};

static ftnand_cmd_format cmd_format[CMD_INDEX_LENGTH_NEW] =
{
    {NAND_CMD_READ1, NAND_CMD_READ2, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_TYPE_READ, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_CHANGE_READ_COLUMN1, NAND_CMD_CHANGE_READ_COLUMN2, FNAND_ADDR_CYCLE_NUM2, FNAND_CMDCTRL_TYPE_READ_COL, FNAND_CTRL_ECC_EN, FNAND_CTRL_AUTO_AUTO_RS_DIS},
    {NAND_CMD_PAGE_PROG1, NAND_CMD_PAGE_PROG2, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_TYPE_PAGE_PRO, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_PAGE_PROG1, NAND_END_CMD_NONE, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_CH_ROW_ADDR, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_DIS},
    {NAND_CMD_CHANGE_WRITE_COLUMN, NAND_CMD_PAGE_PROG2, FNAND_ADDR_CYCLE_NUM2, FNAND_CMDCTRL_TYPE_PAGE_PRO, FNAND_CTRL_ECC_EN, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_BLOCK_ERASE1, NAND_CMD_BLOCK_ERASE2, FNAND_ADDR_CYCLE_NUM3, FNAND_CMDCTRL_TYPE_ERASE, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_RESET, NAND_END_CMD_NONE, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_TYPE_RESET, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_READ_ID, NAND_END_CMD_NONE, FNAND_ADDR_CYCLE_NUM1, FNAND_CMDCTRL_TYPE_READ_ID, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_DIS},
    {NAND_CMD_READ_PARAM_PAGE, NAND_END_CMD_NONE, FNAND_ADDR_CYCLE_NUM1, FNAND_CMDCTRL_READ_PARAM, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_READ1, NAND_CMD_READ2, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_TYPE_READ_ID, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_EN},
    {NAND_CMD_READ_STATUS, NAND_END_CMD_NONE, FNAND_ADDR_CYCLE_NUM5, FNAND_CMDCTRL_TYPE_READ_COL, FNAND_CTRL_ECC_DIS, FNAND_CTRL_AUTO_AUTO_RS_DIS},
};

ftnand_config ftnand_configtable[FNAND_NUM] = {
    {
         .instance_id = FNAND_INSTANCE0 , /* Id of device*/
         .irq_num = FNAND_IRQ_NUM,     /* Irq number */
         .base_address = FNAND_BASEADDRESS,
         .ecc_strength = 8, /* 每次ecc 步骤纠正的位数 */
         .ecc_step_size = 512 /* 进行读写操作时，单次ecc 的步骤的跨度 */
    },
};

ftnand  ftnand_instance;
static u8 read_buffer[NAND_TEST_LENGTH];
static u8 write_buffer[NAND_TEST_LENGTH];
static u8 read_image_buffer[NAND_IMAGE_LENGTH];


/**
 * @name: ft_nand_get_ecc_total_length
 * @msg:  根据page size 与 ecc_strength（纠错个数）确定硬件ecc 产生
 * @param {u32} bytes_per_page
 * @param {u32} ecc_strength
 * @return {*}
 * @note:
 */
u32 ftnand_get_ecc_total_length(u32 bytes_per_page,u32 ecc_strength)
{
    int ecc_total = 0;
	switch (bytes_per_page)
    {
        case 0x200:
            if (ecc_strength == 8)
                ecc_total = 0x0D;
            else if (ecc_strength == 4)
                ecc_total = 7;
            else if (ecc_strength == 2)
                ecc_total = 4;
            else
                ecc_total = 0;
            break;
        case 0x800:
            if (ecc_strength == 8)
                ecc_total = 0x34;
            else if (ecc_strength == 4)
                ecc_total = 0x1a;
            else if (ecc_strength == 2)
                ecc_total = 0xd;
            else
                ecc_total = 0;
            break;
        case 0x1000:
            if (ecc_strength == 8)
                ecc_total = 0x68;
            else if (ecc_strength == 4)
                ecc_total = 0x34;
            else if (ecc_strength == 2)
                ecc_total = 0x1a;
            else
                ecc_total = 0;
            break;
        case 0x2000:
            if (ecc_strength == 8)
                ecc_total = 0xD0;
            else if (ecc_strength == 4)
                ecc_total = 0x68;
            else if (ecc_strength == 2)
                ecc_total = 0x34;
            else
                ecc_total = 0;
            break;
        case 0x4000:
            if (ecc_strength == 8)
                ecc_total = 0x1A0;
            if (ecc_strength == 4)
                ecc_total = 0xD0;
            else if (ecc_strength == 2)
                ecc_total = 0x68;
            else
                ecc_total = 0;
            break;
        default:
            ecc_total = 0;
            break;
	}

	FNAND_ECC_DEBUG_I("[%s %d]writesize: 0x%x, ecc strength: %d, ecc_total: 0x%x\n",__func__, __LINE__, bytes_per_page, ecc_strength, ecc_total);
	return ecc_total;
}

// 校验offset 0xb8  + i * 0x10
// 校验强度为 2 j = 1
// 校验强度为 4 j = 2
// 校验强度为 8 j = 4
int ftnand_correct_ecc(uintptr_t base_address,u32 ecc_step_size,u32 hw_ecc_steps,u8* buf ,u32 length)
{
    u32 i, j;
	u32 value, tmp;
	int stat = 0;
    if(!buf)
    {
        FNAND_ECC_DEBUG_E("page buffer is null");
        return -1;
    }

    /* i  */
	for (i = 0; i < hw_ecc_steps; i++)
    {
		for (j = 0; j < 4; j++) {
			// value = ftnand_readreg(base_address, 0xB8 + 4 * (2 * i + j));
            value = ftnand_readreg(base_address, 0xB8 + 0x10 * i + 4*j);
            // FNAND_ECC_DEBUG_W("index:%x i is %d ,j is %d ",
			// 		 0xB8 + 0x10 * i + 4*j,i,j);
            if (value)
            {
				// FNAND_ECC_DEBUG_W("offset:%x value:0x%08x\n",
				// 	 0xB8 + 0x10 * i + 4*j, value);
				//phytium_nfc_data_dump2(nfc, nfc->dma_buf + (ecc_step_size * i + tmp/8)/512, 512);
			}

			tmp = value & 0xFFFF;
			if (tmp && (tmp <= 4096))
            {
				tmp -= 1;
				FNAND_ECC_DEBUG_W( "ECC_CORRECT %x %02x\n",
					 ecc_step_size * i + tmp / 8,
					 buf[ecc_step_size * i + tmp / 8]);

				buf[ecc_step_size*i + tmp/8] ^= (0x01 << tmp%8);
				stat++;

				FNAND_ECC_DEBUG_W( "ECC_CORRECT xor %x %02x\n",
					 0x01 << tmp % 8,
					 buf[ecc_step_size * i + tmp / 8]);
			}


            tmp = (value >> 16) & 0xFFFF;
			if (tmp && (tmp <= 4096) )
            {
				tmp -= 1;
				FNAND_ECC_DEBUG_W( "ECC_CORRECT %x %02x\n",
					 ecc_step_size * i + tmp / 8,
					 buf[ecc_step_size * i + tmp / 8]);

				buf[ecc_step_size*i + tmp/8] ^= (0x01 << tmp%8);
				stat++;

				FNAND_ECC_DEBUG_W( "ECC_CORRECT xor %x %02x\n",
					 ecc_step_size * i + tmp / 8,
					 buf[ecc_step_size * i + tmp / 8]);
			}

		}
	}

	return stat;

}

u32 ftnand_set_option(ftnand *instance_p,u32 options,u32 value)
{

    ftnand_config *config_p;
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    config_p = &instance_p->config;

    switch(options)
    {
        case FNAND_OPS_INTER_MODE_SELECT:
            ASSERT(FNAND_TOG_ASYN_DDR >= value) ;
            ftnand_clearbit(config_p->base_address,FNAND_CTRL0_OFFSET,FNAND_CTRL0_INTER_MODE(3UL)) ;
            ftnand_setbit(config_p->base_address,FNAND_CTRL0_OFFSET,FNAND_CTRL0_INTER_MODE((unsigned long)value)) ;
        break;
        default:
            return FNAND_ERR_INVAILD_PARAMETER;
    }

    return FT_SUCCESS;
}

static u32 ftnand_memcpy_toreg16(ftnand *instance_p,u32 reg,u32 reg_step,const u16 *buf,u32 len)
{
    u32 i;
    u32 value = 0;

    if(!instance_p || !buf )
    {
        FNAND_TIMING_DEBUG_E("instance_p is %p ,buf is %p",instance_p,buf);
        return FNAND_ERR_INVAILD_PARAMETER;
    }

    for (i = 0; i < len; i++)
    {
        value = (value << 16) + buf[i];
        if (i % 2)
        {
            ftnand_writereg(instance_p->config.base_address, reg, value);
            value = 0;
            reg += reg_step;
        }
    }

    return FT_SUCCESS;
}

/**
 * @name:
 * @msg: 根据inter_mode 与 timing_mode
 * @note:
 * @return {*}
 * @param {ftnand} *instance_p
 */
u32 ftnand_timing_interface_update(ftnand *instance_p,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM);
    const u16 *target_timming_data = NULL;
    ftnand_config * config_p = &instance_p->config ;
    u32 value = 0 ;
    u32 ret;
    FNAND_TIMING_DEBUG_I("The entry of %s at %d.\n", __func__, __LINE__);

    ftnand_set_option(instance_p, FNAND_OPS_INTER_MODE_SELECT, instance_p->inter_mode[chip_addr]);

    ftnand_clearbit(config_p->base_address,FNAND_CTRL1_OFFSET,FNAND_CTRL1_SAMPL_PHASE_MAKE(0xffffUL));  /* clear sampl_phase */
    switch(instance_p->inter_mode[chip_addr])
    {
        case FNAND_ASYN_SDR:
            if(FNAND_TIMING_MODE4 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                target_timming_data = ftnand_timing_asy_mode4;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(4UL) ;
            }
            else if(FNAND_TIMING_MODE3 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                target_timming_data = ftnand_timing_asy_mode3;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(5UL) ;
            }
            else if(FNAND_TIMING_MODE2 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                target_timming_data = ftnand_timing_asy_mode2;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(6UL) ;
            }
            else if(FNAND_TIMING_MODE1 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                target_timming_data = ftnand_timing_asy_mode1;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(5UL) ;
            }
            else
            {
                target_timming_data = ftnand_timing_asy_mode0;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(1UL) ;
            }
            ret = ftnand_memcpy_toreg16(instance_p,FNAND_ASY_TIMING0_OFFSET,4,target_timming_data,FNAND_TIMING_ASY_NUM);
            if(ret != FT_SUCCESS)
            {
                return  ret;
            }
            ftnand_setbit(config_p->base_address, FNAND_CTRL1_OFFSET, value);
            ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,1);
        break;
        case FNAND_ONFI_DDR:
            if(FNAND_TIMING_MODE4 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,0x30);
                target_timming_data = ftnand_timing_syn_mode4;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(0xdUL) ;
            }
            else if(FNAND_TIMING_MODE3 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,0x18);
                target_timming_data = ftnand_timing_syn_mode3;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(5UL) ;
            }
            else if(FNAND_TIMING_MODE2 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,0x20);
                target_timming_data = ftnand_timing_syn_mode2;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(0x8UL) ;
            }
            else if(FNAND_TIMING_MODE1 == (instance_p->timing_mode[chip_addr] &0xf))
            {
                ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,0x40);
                target_timming_data = ftnand_timing_syn_mode1;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(0x12UL) ;
            }
            else
            {
                ftnand_writereg(config_p->base_address,FNAND_INTERVAL_OFFSET,0x40);
                target_timming_data = ftnand_timing_syn_mode0;
                value = FNAND_CTRL1_SAMPL_PHASE_MAKE(0x12UL) ;
            }
            ret =  ftnand_memcpy_toreg16(instance_p,FNAND_SYN_TIMING6_OFFSET,4,target_timming_data,FNAND_TIMING_SYN_NUM);
            if(ret != FT_SUCCESS)
            {
                return  ret;
            }
            ftnand_setbit(config_p->base_address, FNAND_CTRL1_OFFSET, value);
            break;
        case FNAND_TOG_ASYN_DDR:
            value = FNAND_CTRL1_SAMPL_PHASE_MAKE(8UL);
            target_timming_data = ftnand_timing_tog_ddr_mode0;
            ret = ftnand_memcpy_toreg16(instance_p, FNAND_TOG_TIMING13_OFFSET, 4, target_timming_data, FNAND_TIMING_SYN_NUM);
            if(ret != FT_SUCCESS)
            {
                return  ret;
            }
            ftnand_writereg(config_p->base_address, FNAND_INTERVAL_OFFSET, 0xc8);
            ftnand_setbit(config_p->base_address, FNAND_CTRL1_OFFSET, value);
            break;
        default:
            FNAND_TIMING_DEBUG_E("Error inter_mode is %x",instance_p->inter_mode[chip_addr]);
            return  FNAND_ERR_INVAILD_PARAMETER;
        }

        return FT_SUCCESS;
}


const struct ftnand_sdr_timings* ftnand_async_timing_mode_to_sdr_timings(ftnand_async_tim_int mode)
{
    if(mode >= FNAND_ASYNC_TIM_INT_MODE4)
    {
        FNAND_TIMING_DEBUG_E("ftnand_async_timing_mode_to_sdr_timings set is over mode range");
        return NULL;
    }

    return &onfi_sdr_timings[mode];
}

/**
 * @name:
 * @msg:
 * @return {*}
 * @param {ftnand} *instance_p
 * @param {ftnand_async_tim_int} mode
 * @Note 当前只支持onfi 模式
 */
u32 ftnand_fill_timing_mode_timing(ftnand *instance_p,ftnand_async_tim_int mode)
{
    struct ftnand_sdr_timings *sdr_timing_p = NULL;
    const struct ftnand_sdr_timings *source_timing_p = NULL;
    ASSERT(instance_p != NULL);
    sdr_timing_p = &instance_p->sdr_timing;
    source_timing_p = ftnand_async_timing_mode_to_sdr_timings(mode);
    ASSERT(source_timing_p != NULL);
    *sdr_timing_p = *source_timing_p;


    return FT_SUCCESS;
}


static inline void ftnand_enable_hw_ecc(uintptr_t base_address)
{
    ftnand_setbit(base_address,FNAND_CTRL0_OFFSET,FNAND_CTRL0_ECC_EN_MASK);

}


static inline void ftnand_disable_hw_ecc(uintptr_t base_address)
{
    ftnand_clearbit(base_address,FNAND_CTRL0_OFFSET,FNAND_CTRL0_ECC_EN_MASK);
}

static u16 ftnand_toggle_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}



/* Sanitize ONFI strings so we can safely print them */
static void ftnand_sanitize_string(u8 *s, size_t len)
{
	size_t i;

	/* Null terminate */
	s[len - 1] = 0;

	/* Remove non printable chars */
	for (i = 0; i < len - 1; i++) {
		if (s[i] < ' ' || s[i] > 127)
			s[i] = '?';
	}

}


void ftnand_dma_dump(struct ftnand_dma_descriptor *descriptor_p)
{
    FNAND_DMA_DEBUG_I("Phytium NFC cmd dump:\n");
    FNAND_DMA_DEBUG_I("cmd0:%x, cmd1:%x, ctrl:%x, page_cnt:%d\n",
                      descriptor_p->cmd0, descriptor_p->cmd1, descriptor_p->cmd_ctrl.ctrl, descriptor_p->page_cnt);
    FNAND_DMA_DEBUG_I("mem_addr_first:%02x %02x %02x %02x %02x\n",
                      descriptor_p->mem_addr_first[0], descriptor_p->mem_addr_first[1], descriptor_p->mem_addr_first[2], descriptor_p->mem_addr_first[3], descriptor_p->mem_addr_first[4]);
    FNAND_DMA_DEBUG_I("addr:%02x %02x %02x %02x %02x\n",
                      descriptor_p->addr[0], descriptor_p->addr[1], descriptor_p->addr[2], descriptor_p->addr[3], descriptor_p->addr[4]);

    FNAND_DMA_DEBUG_I(" csel : 0x%x  ", descriptor_p->cmd_ctrl.nfc_ctrl.csel);
    FNAND_DMA_DEBUG_I(" dbc : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.dbc);
    FNAND_DMA_DEBUG_I(" addr_cyc : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.addr_cyc);
    FNAND_DMA_DEBUG_I(" nc : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.nc);
    FNAND_DMA_DEBUG_I(" cmd_type : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.cmd_type);
    FNAND_DMA_DEBUG_I(" dc : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.dc);
    FNAND_DMA_DEBUG_I(" auto_rs : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.auto_rs);
    FNAND_DMA_DEBUG_I(" ecc_en : %d  ", descriptor_p->cmd_ctrl.nfc_ctrl.ecc_en);

}

static void ftnand_addr_correct(struct ftnand_dma_descriptor *descriptor_p,
                             u8 *addr_p,
                             u32 addr_length)
{
    int i, j;
    if (addr_length == 0 || addr_p == NULL)
    {
        FNAND_DMA_DEBUG_I("addr_p is null ,Calibration is not required ");
        return;
    }

    if (addr_length >= FNAND_NFC_ADDR_MAX_LEN)
    {
        memcpy(descriptor_p->addr, addr_p, FNAND_NFC_ADDR_MAX_LEN);
        descriptor_p->cmd_ctrl.nfc_ctrl.addr_cyc = FNAND_NFC_ADDR_MAX_LEN;
        return;
    }

    descriptor_p->cmd_ctrl.nfc_ctrl.addr_cyc = 0;
    for (i = addr_length - 1, j = FNAND_NFC_ADDR_MAX_LEN - 1; i >= 0; i--, j--)
    {
        descriptor_p->addr[j] = addr_p[i]; /* data shift to high array */
        descriptor_p->addr[i] = 0;
        descriptor_p->cmd_ctrl.nfc_ctrl.addr_cyc++;
    }
}

u32 ftnand_dma_pack(ftnand_cmd_format *cmd_format,
                    struct ftnand_dma_descriptor *descriptor_p,
                    ftnand_dma_pack_data *pack_data_p
                    )
{
    u32 i;
    ASSERT(cmd_format != NULL);
    ASSERT(descriptor_p != NULL);
    // printf(" descriptor_p is %p \r\n",descriptor_p);
    descriptor_p->cmd_ctrl.ctrl = 0;

    /* cmd */
    if (cmd_format->end_cmd == TOGGLE_END_CMD_NONE) /* Only one cmd ,need to correct */
    {
        descriptor_p->cmd1 = cmd_format->start_cmd;
        descriptor_p->cmd0 = 0;
    }
    else
    {
        descriptor_p->cmd0 = cmd_format->start_cmd;
        descriptor_p->cmd1 = cmd_format->end_cmd;
        descriptor_p->cmd_ctrl.nfc_ctrl.dbc = 1;
    }

    /* addr */
    ftnand_addr_correct(descriptor_p, pack_data_p->addr_p, pack_data_p->addr_length);
    descriptor_p->cmd_ctrl.nfc_ctrl.cmd_type = cmd_format->cmd_type; /* cmd type */
    FNAND_DMA_DEBUG_I("cmd_type is %x \r\n",descriptor_p->cmd_ctrl.nfc_ctrl.cmd_type);
    if (pack_data_p->contiune_dma)
    {
        descriptor_p->cmd_ctrl.nfc_ctrl.nc = 1;
    }

    descriptor_p->cmd_ctrl.nfc_ctrl.csel = (0xf ^ (1 << pack_data_p->chip_addr));

    if (pack_data_p->phy_address && (pack_data_p->phy_bytes_length > 0))
    {
        descriptor_p->cmd_ctrl.nfc_ctrl.dc = 1;
        for (i = 0; i < FNAND_NFC_ADDR_MAX_LEN; i++)
        {
            descriptor_p->mem_addr_first[i] = pack_data_p->phy_address >> (8 * i) & 0xff;
        }
        descriptor_p->page_cnt = pack_data_p->phy_bytes_length;
    }

    if (cmd_format->auto_rs)
        descriptor_p->cmd_ctrl.nfc_ctrl.auto_rs = 1;

    if (cmd_format->ecc_en)
        descriptor_p->cmd_ctrl.nfc_ctrl.ecc_en = 1;

    ftnand_dma_dump(descriptor_p);

    return FT_SUCCESS;
}

static u32 ftnand_toggle_read_param_page(ftnand *instance_p,  u8 *id_buffer, u32 buffer_length, u32 chip_addr)
{
    u32 ret;
    u8 address = 0x40;
    u32 memcpy_length;
    ftnand_dma_pack_data pack_data =
        {
            .addr_p = &address,
            .addr_length = 1,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = (3 * sizeof(struct toggle_nand_geometry) > FNAND_DMA_MAX_LENGTH) ? FNAND_DMA_MAX_LENGTH : (3 * sizeof(struct toggle_nand_geometry)),
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    ftnand_dma_pack(&cmd_format[CMD_READ_DEVICE_TABLE], &instance_p->descriptor[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, &instance_p->descriptor[0],FNAND_READ_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    if(buffer_length && id_buffer)
    {
        memcpy_length = (buffer_length > pack_data.phy_bytes_length)?pack_data.phy_bytes_length:buffer_length;
        memcpy(id_buffer,instance_p->dma_data_buffer.data_buffer,memcpy_length);
    }

    return FT_SUCCESS;
}

static u32 ftnand_toggle_detect_jedec(ftnand *instance_p,struct toggle_nand_geometry * toggle_geometry_p,ftnand_nand_geometry *geometry_p)
{
    /* 检查crc */
    if(ftnand_toggle_crc16(FNAND_TOGGLE_CRC_BASE, (u8 *)toggle_geometry_p,510) != toggle_geometry_p->crc)
    {
        FNAND_TOGGLE_DEBUG_E("Toggle error mode");
    }

    FNAND_TOGGLE_DEBUG_I("revision is %x",toggle_geometry_p->revision);

    ftnand_sanitize_string(toggle_geometry_p->manufacturer,sizeof(toggle_geometry_p->manufacturer));
    ftnand_sanitize_string(toggle_geometry_p->model,sizeof(toggle_geometry_p->model));
    FNAND_TOGGLE_DEBUG_I("manufacturer %s",toggle_geometry_p->manufacturer);
    FNAND_TOGGLE_DEBUG_I("model %s",toggle_geometry_p->model);

    geometry_p->bytes_per_page = toggle_geometry_p->byte_per_page;
    geometry_p->spare_bytes_per_page =  toggle_geometry_p->spare_bytes_per_page;
    geometry_p->pages_per_block = toggle_geometry_p->pages_per_block;
    geometry_p->blocks_per_lun = toggle_geometry_p->blocks_per_lun ;
    geometry_p->num_lun = toggle_geometry_p->lun_count;
    geometry_p->num_pages = (geometry_p->num_lun *
					  geometry_p->blocks_per_lun *
					 geometry_p->pages_per_block);
    geometry_p->num_blocks = (geometry_p->num_lun * geometry_p->blocks_per_lun);
    geometry_p->block_size = (geometry_p->pages_per_block * geometry_p->bytes_per_page);
    geometry_p->device_size = (geometry_p->num_blocks * geometry_p->block_size * geometry_p->bytes_per_page);
    geometry_p->rowaddr_cycles = toggle_geometry_p->addr_cycles & 0xf;
    geometry_p->coladdr_cycles = (toggle_geometry_p->addr_cycles >> 4) & 0xf ;
    geometry_p->hw_ecc_length =  ftnand_get_ecc_total_length(geometry_p->bytes_per_page,instance_p->config.ecc_strength);
    geometry_p->ecc_offset = geometry_p->spare_bytes_per_page - geometry_p->hw_ecc_length;
    geometry_p->hw_ecc_steps = geometry_p->bytes_per_page  / instance_p->config.ecc_step_size ;
    geometry_p->ecc_step_size = instance_p->config.ecc_step_size;
    FNAND_TOGGLE_DEBUG_D("bytes_per_page %d ", geometry_p->bytes_per_page);               /* Bytes per page */
    FNAND_TOGGLE_DEBUG_D("spare_bytes_per_page %d " ,geometry_p->spare_bytes_per_page) ;  /* Size of spare area in bytes */
    FNAND_TOGGLE_DEBUG_D("pages_per_block %d " , geometry_p->pages_per_block) ;       /* Pages per block */
    FNAND_TOGGLE_DEBUG_D("blocks_per_lun %d " ,geometry_p->blocks_per_lun ) ;        /* Bocks per LUN */
    FNAND_TOGGLE_DEBUG_D("num_lun %d " ,geometry_p->num_lun) ;                /* Total number of LUN */
    FNAND_TOGGLE_DEBUG_D("num_pages %lld " , geometry_p->num_pages) ;             /* Total number of pages in device */
    FNAND_TOGGLE_DEBUG_D("num_blocks %lld " , geometry_p->num_blocks) ;            /* Total number of blocks in device */
    FNAND_TOGGLE_DEBUG_D("block_size %lld " , geometry_p->block_size) ;            /* Size of a block in bytes */
    FNAND_TOGGLE_DEBUG_D("device_size %lld " , geometry_p->device_size) ;           /* Total device size in bytes */
    FNAND_TOGGLE_DEBUG_D("rowaddr_cycles %d " , geometry_p->rowaddr_cycles) ;         /* Row address cycles */
    FNAND_TOGGLE_DEBUG_D("coladdr_cycles %d " , geometry_p->coladdr_cycles) ;         /* Column address cycles */
    FNAND_TOGGLE_DEBUG_D("hw_ecc_lengthd %d " , geometry_p->hw_ecc_length) ;         /* 产生硬件ecc校验参数的个数 */
    FNAND_TOGGLE_DEBUG_D("ecc_offset %d " , geometry_p->ecc_offset) ;         /* obb存放硬件ecc校验参数页位置的偏移 */
    FNAND_TOGGLE_DEBUG_D("hw_ecc_steps %d " , geometry_p->hw_ecc_steps) ;         /* number of ECC steps per page */
    FNAND_TOGGLE_DEBUG_D("ecc_step_size %d " , geometry_p->ecc_step_size) ;      /* 进行读写操作时，单次ecc 的步骤的跨度 */

    return FT_SUCCESS;
}


/**
 * @name: ftnand_toggle_init
 * @msg:  Toggle mode interface initialization
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} chip_addr is chip address
 * @return {u32} FT_SUCCESS 初始化成功 ，FNAND_NOT_FET_TOGGLE_MODE 初始化toggle 模式错误。
 */
u32 ftnand_toggle_init(ftnand *instance_p,u32 chip_addr)
{
    u32 ret;
    char id[6];
    ASSERT(instance_p != NULL);
    struct toggle_nand_geometry *toggle_geometry_p;
    /* step 1 .reset nand chip */
    ret = ftnand_flash_reset(instance_p,chip_addr) ;
    if(ret != FT_SUCCESS)
    {
        FNAND_TOGGLE_DEBUG_E("ftnand_flash_reset is error");
        return ret;
    }

    /* step 2. readid operation 40h */
    ret = ftnand_flash_read_id(instance_p,0x40,id,sizeof(id),chip_addr);
    if(ret != FT_SUCCESS || strncmp(id,"JEDEC",sizeof(id) - 1))
    {
        FNAND_TOGGLE_DEBUG_E("40H read id is %s ",id);
        return FNAND_NOT_FET_TOGGLE_MODE;
    }

    if(id[5] == 1)
    {
        instance_p->inter_mode[chip_addr] = FNAND_ASYN_SDR;
    }
    else if(id[5] == 2)
    {
        instance_p->inter_mode[chip_addr] = FNAND_TOG_ASYN_DDR;
    }
    else if(id[5] == 4)
    {
        instance_p->inter_mode[chip_addr] = FNAND_ASYN_SDR;
    }

    ftnand_timing_interface_update(instance_p,chip_addr);

    /* step 3. read device id table */

    ret = ftnand_toggle_read_param_page(instance_p, NULL, 0, chip_addr);
    if (ret != FT_SUCCESS)
    {
        FNAND_TOGGLE_DEBUG_E("read device id table is error");
        return FNAND_NOT_FET_TOGGLE_MODE;
    }

    /* step 4. device id table parse */
    toggle_geometry_p = (struct toggle_nand_geometry *)instance_p->dma_data_buffer.data_buffer;

    return ftnand_toggle_detect_jedec(instance_p,toggle_geometry_p,&instance_p->nand_geometry[chip_addr]);
}


static u16 ftnand_onfi_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}



static u32 ftnand_onfi_detect_jedec(ftnand *instance_p,struct onfi_nand_geometry * onfi_geometry_p,ftnand_nand_geometry *geometry_p)
{
    /* 多参数页冗余检查 */
    if(ftnand_onfi_crc16(FNAND_ONFI_CRC_BASE, (u8 *)onfi_geometry_p,256) != onfi_geometry_p->crc)
    {
        FNAND_ONFI_DEBUG_E("Onfi error mode");
    }

    FNAND_ONFI_DEBUG_I("revision is %x",onfi_geometry_p->revision);

    ftnand_sanitize_string(onfi_geometry_p->manufacturer,sizeof(onfi_geometry_p->manufacturer));
    ftnand_sanitize_string(onfi_geometry_p->model,sizeof(onfi_geometry_p->model));
    FNAND_ONFI_DEBUG_I("manufacturer %s",onfi_geometry_p->manufacturer);
    FNAND_ONFI_DEBUG_I("model %s",onfi_geometry_p->model);

    geometry_p->bytes_per_page = onfi_geometry_p->byte_per_page;
    geometry_p->spare_bytes_per_page =  onfi_geometry_p->spare_bytes_per_page;
    geometry_p->pages_per_block = onfi_geometry_p->pages_per_block;
    geometry_p->blocks_per_lun = onfi_geometry_p->blocks_per_lun ;
    geometry_p->num_lun = onfi_geometry_p->lun_count;
    geometry_p->num_pages = (geometry_p->num_lun *
					  geometry_p->blocks_per_lun *
					 geometry_p->pages_per_block);
    geometry_p->num_blocks = (geometry_p->num_lun * geometry_p->blocks_per_lun);
    geometry_p->block_size = (geometry_p->pages_per_block * geometry_p->bytes_per_page);
    geometry_p->device_size = (geometry_p->num_blocks * geometry_p->block_size);
    geometry_p->rowaddr_cycles = onfi_geometry_p->addr_cycles & 0xf;
    geometry_p->coladdr_cycles = (onfi_geometry_p->addr_cycles >> 4) & 0xf ;
    geometry_p->hw_ecc_length =  ftnand_get_ecc_total_length(geometry_p->bytes_per_page,instance_p->config.ecc_strength); /* 需要增加检查oob 长度 */
    geometry_p->ecc_offset = geometry_p->spare_bytes_per_page - geometry_p->hw_ecc_length;
    geometry_p->hw_ecc_steps = geometry_p->bytes_per_page  / instance_p->config.ecc_step_size ;
    geometry_p->ecc_step_size = instance_p->config.ecc_step_size;
    FNAND_ONFI_DEBUG_D("bytes_per_page %d ", geometry_p->bytes_per_page);               /* Bytes per page */
    FNAND_ONFI_DEBUG_D("spare_bytes_per_page %d " ,geometry_p->spare_bytes_per_page) ;  /* Size of spare area in bytes */
    FNAND_ONFI_DEBUG_D("pages_per_block %d " , geometry_p->pages_per_block) ;       /* Pages per block */
    FNAND_ONFI_DEBUG_D("blocks_per_lun %d " ,geometry_p->blocks_per_lun ) ;        /* Bocks per LUN */
    FNAND_ONFI_DEBUG_D("num_lun %d " ,geometry_p->num_lun) ;                /* Total number of LUN */
    FNAND_ONFI_DEBUG_D("num_pages %lld " , geometry_p->num_pages) ;             /* Total number of pages in device */
    FNAND_ONFI_DEBUG_D("num_blocks %lld " , geometry_p->num_blocks) ;            /* Total number of blocks in device */
    FNAND_ONFI_DEBUG_D("block_size %lld " , geometry_p->block_size) ;            /* Size of a block in bytes */
    FNAND_ONFI_DEBUG_D("device_size %lld " , geometry_p->device_size) ;           /* Total device size in bytes */
    FNAND_ONFI_DEBUG_D("rowaddr_cycles %d " , geometry_p->rowaddr_cycles) ;         /* Row address cycles */
    FNAND_ONFI_DEBUG_D("coladdr_cycles %d " , geometry_p->coladdr_cycles) ;         /* Column address cycles */
    FNAND_ONFI_DEBUG_D("hw_ecc_lengthd %d " , geometry_p->hw_ecc_length) ;         /* 产生硬件ecc校验参数的个数 */
    FNAND_ONFI_DEBUG_D("ecc_offset %d " , geometry_p->ecc_offset) ;         /* obb存放硬件ecc校验参数页位置的偏移 */
    FNAND_ONFI_DEBUG_D("hw_ecc_steps %d " , geometry_p->hw_ecc_steps) ;         /* number of ECC steps per page */
    FNAND_ONFI_DEBUG_D("ecc_step_size %d " , geometry_p->ecc_step_size) ;      /* 进行读写操作时，单次ecc 的步骤的跨度 */

    return FT_SUCCESS;
}


static u32 ftnand_onfi_read_param_page(ftnand *instance_p,  u8 *id_buffer, u32 buffer_length, u32 chip_addr)
{
    u32 ret;
    u8 address = 0x00;
    u32 memcpy_length;
    ftnand_dma_pack_data pack_data =
        {
            .addr_p = &address,
            .addr_length = 1,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = (3 * sizeof(struct onfi_nand_geometry) > FNAND_DMA_MAX_LENGTH)?FNAND_DMA_MAX_LENGTH:(3 * sizeof(struct onfi_nand_geometry)),
            .chip_addr = chip_addr,
            .contiune_dma = 0, };

    ftnand_dma_pack(&cmd_format[CMD_READ_DEVICE_TABLE], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_READ_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    if(buffer_length && id_buffer)
    {
        memcpy_length = (buffer_length > pack_data.phy_bytes_length)?pack_data.phy_bytes_length:buffer_length;
        memcpy(id_buffer,instance_p->dma_data_buffer.data_buffer,memcpy_length);
    }
    return FT_SUCCESS;
}

/**
 * @name: ftnand_onfi_init
 * @msg:  Onfi mode interface initialization
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} chip_addr is chip address
 * @return {u32} FT_SUCCESS 初始化成功 ，FNAND_NOT_FET_TOGGLE_MODE 初始化toggle 模式错误。
 */
u32 ftnand_onfi_init(ftnand *instance_p,u32 chip_addr)
{
    u32 ret;
    char id[5];

    ASSERT(instance_p != NULL);
    struct onfi_nand_geometry *onfi_geometry_p;
    /* step 1 .reset nand chip */
    FNAND_ONFI_DEBUG_E(".reset nand chip ");
    ret = ftnand_flash_reset(instance_p, chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_ONFI_DEBUG_E("ftnand_flash_reset is error");
        return ret;
    }
    FNAND_ONFI_DEBUG_E("step 2. readid operation 20h");
    /* step 2. readid operation 20h */
    ret = ftnand_flash_read_id(instance_p,0x20,id,sizeof(id),chip_addr);

    if(ret != FT_SUCCESS || strncmp(id,"ONFI",sizeof(id) - 1))
    {
        FNAND_ONFI_DEBUG_E("20H read id is %s ",id);
        return FNAND_NOT_FET_TOGGLE_MODE;
    }

    instance_p->inter_mode[chip_addr] = FNAND_ASYN_SDR;


    ftnand_timing_interface_update(instance_p,chip_addr);
    /* step 3. read device id table */

    ret = ftnand_onfi_read_param_page(instance_p, NULL, 0, chip_addr);
    if (ret != FT_SUCCESS)
    {
        FNAND_ONFI_DEBUG_E("read device id table is error");
        return FNAND_NOT_FET_TOGGLE_MODE;
    }

    /* step 4. device id table parse */
    onfi_geometry_p = (struct onfi_nand_geometry *)instance_p->dma_data_buffer.data_buffer;

    return ftnand_onfi_detect_jedec(instance_p,onfi_geometry_p,&instance_p->nand_geometry[chip_addr]);
}



u32 ftnand_flash_read_id(ftnand *instance_p, u8 address, u8 *id_buffer, u32 buffer_length, u32 chip_addr)
{
    u32 ret;
    u32 memcpy_length;
    ftnand_dma_pack_data pack_data =
        {
            .addr_p = &address,
            .addr_length = 1,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = (buffer_length > FNAND_DMA_MAX_LENGTH) ? FNAND_DMA_MAX_LENGTH : buffer_length,
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    memset((struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], 0, sizeof(struct ftnand_dma_descriptor));
    ftnand_dma_pack(&cmd_format[CMD_READ_ID], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_CMD_TYPE);
    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    if(buffer_length && id_buffer)
    {
        memcpy_length = (buffer_length > pack_data.phy_bytes_length)?pack_data.phy_bytes_length:buffer_length;
        memcpy(id_buffer,instance_p->dma_data_buffer.data_buffer,memcpy_length);
    }

   return FT_SUCCESS;
}


static u32 ftnand_flash_read_status(ftnand *instance_p,u32 chip_addr)
{
    u32 ret;
    ASSERT(instance_p != NULL);
    ftnand_dma_pack_data pack_data =
    {
        .addr_p = NULL,
        .addr_length = 0,
        .phy_address =  (unsigned long)instance_p->dma_data_buffer.data_buffer,
        .phy_bytes_length = 4,
        .chip_addr = chip_addr,
        .contiune_dma  = 0,
    };

    memset((struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], 0, sizeof(struct ftnand_dma_descriptor));
    ftnand_dma_pack(&cmd_format[CMD_READ_STATUS], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data); /* FNAND_CMDCTRL_TYPE_RESET */
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_READ_PAGE_TYPE);
    if(ret != FT_SUCCESS)
    {
        return ret;
    }

    FNAND_COMMON_DEBUG_I("read status is 0x%x", instance_p->dma_data_buffer.data_buffer[0]);

    return (instance_p->dma_data_buffer.data_buffer[0] == 0xe0)? FT_SUCCESS:FNAND_IS_BUSY;
}

u32 ftnand_flash_reset(ftnand *instance_p,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ftnand_dma_pack_data pack_data =
    {
        .addr_p = NULL,
        .addr_length = 0,
        .phy_address =  (unsigned long)NULL,
        .phy_bytes_length = 0,
        .chip_addr = chip_addr,
        .contiune_dma  = 0,
    };

    memset((struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], 0, sizeof(struct ftnand_dma_descriptor));
    ftnand_dma_pack(&cmd_format[CMD_RESET], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data); /* FNAND_CMDCTRL_TYPE_RESET */
    return ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_CMD_TYPE);
}


u32 ftnand_flash_erase_block(ftnand *instance_p,u32 page_addr,u32 chip_addr)
{
    u8 addr_buf[3] = {0};
    u32 ret;
    u32 nand_state;
	u32 count=0;
    // printf("ftnand_flash_erase_block %d \r\n", page_addr);
   
    /* read operation */


    addr_buf[0] = page_addr;
	addr_buf[1] = page_addr >> 8;
	addr_buf[2] = page_addr >> 16;

    ftnand_dma_pack_data pack_data =
    {
        .addr_p = addr_buf,
        .addr_length = 3,
        .phy_address = (unsigned long)NULL,
        .phy_bytes_length = 0,
        .chip_addr = chip_addr,
        .contiune_dma  = 0,
    };

    ftnand_dma_pack(&cmd_format[CMD_BLOCK_ERASE], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_CMD_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);
    while (nand_state & FNAND_STATE_BUSY_OFFSET) /* wait busy state is over */
    {
        nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);
    }
    // printf("erase after nand_state 0x%x \r\n",nand_state);

	while (ftnand_flash_read_status(instance_p, chip_addr) != FT_SUCCESS)
	{
		if(count++ > 0xff)
			return FNAND_IS_BUSY;
	}; /* wait i/o idle */

    return FT_SUCCESS;
}

static u32 ftnand_page_read(ftnand *instance_p,u32 page_addr,u8* buf,u32 page_copy_offset,u32 length, u32 chip_addr)
{
    u8 addr_buf[5] = {0};
    u32 memcpy_length;
    u32 ret;
    addr_buf[4] = (page_addr >> 16);
    addr_buf[3] = (page_addr >> 8);
    addr_buf[2] = (page_addr);

    ftnand_dma_pack_data pack_data =
    {
        .addr_p = addr_buf,
        .addr_length = 5,
        .phy_address =  (unsigned long)instance_p->dma_data_buffer.data_buffer,
        .phy_bytes_length = instance_p->nand_geometry[chip_addr].bytes_per_page,
        .chip_addr = chip_addr,
        .contiune_dma  = 0,
    };

    ftnand_dma_pack(&cmd_format[CMD_READ_OPTION_NEW], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_READ_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    if (length && buf)
    {
        memcpy_length = (length > (pack_data.phy_bytes_length - page_copy_offset))?(pack_data.phy_bytes_length - page_copy_offset):length;
        memcpy(buf,instance_p->dma_data_buffer.data_buffer + page_copy_offset,memcpy_length);
    }

    return FT_SUCCESS;
}

static u32 ftnand_page_write(ftnand *instance_p,u32 page_addr,u8* buf,u32 page_copy_offset,u32 length, u32 chip_addr)
{
    u8 addr_buf[5] = {0};
    u32 ret;
    u32 bytes_per_page = 0;
	u32 count=0;
    addr_buf[4] = (page_addr >> 16);
    addr_buf[3] = (page_addr >> 8);
    addr_buf[2] = (page_addr);
    bytes_per_page = instance_p->nand_geometry[chip_addr].bytes_per_page;
    ftnand_dma_pack_data pack_data =
        {
            .addr_p = addr_buf,
            .addr_length = 5,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = bytes_per_page,
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    memcpy(instance_p->dma_data_buffer.data_buffer + page_copy_offset ,buf,((bytes_per_page - page_copy_offset) > length)?length:(bytes_per_page - page_copy_offset));

    ftnand_dma_pack(&cmd_format[CMD_PAGE_PROGRAM], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_WRITE_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }
    while(ftnand_flash_read_status(instance_p,chip_addr) != FT_SUCCESS)
	{
		if(count++ > 0xff)
			return FNAND_IS_BUSY;
	};
    return FT_SUCCESS;
}

static u32 ftnand_page_write_hw_ecc(ftnand *instance_p,u32 page_addr,u8* buf,u32 page_copy_offset,u32 length, u32 chip_addr)
{
    u32 ret;
    u32 nand_state = 0;
    u8 addr_buf[5] = {0};
    u32 ecc_offset = 0;
    u32 bytes_per_page = 0;
    u32 spare_bytes_per_page = 0;
	u32 count=0;
    /* read operation */
    addr_buf[2] = page_addr;
	addr_buf[3] = page_addr >> 8;
	addr_buf[4] = page_addr >> 16;

    ecc_offset = instance_p->nand_geometry[chip_addr].ecc_offset;
    bytes_per_page = instance_p->nand_geometry[chip_addr].bytes_per_page;
    spare_bytes_per_page = instance_p->nand_geometry[chip_addr].spare_bytes_per_page;

    ftnand_dma_pack_data pack_data =
        {
            .addr_p = addr_buf,
            .addr_length = 5,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = bytes_per_page,
            .chip_addr = chip_addr,
            .contiune_dma = 1,
        };

    memset(instance_p->dma_data_buffer.data_buffer,0xff,FNAND_DMA_MAX_LENGTH);
    memcpy(instance_p->dma_data_buffer.data_buffer + page_copy_offset, buf, ((bytes_per_page - page_copy_offset) < length) ? (bytes_per_page - page_copy_offset) : length);

    ftnand_dma_pack(&cmd_format[CMD_PAGE_PROGRAM_WITH_OTHER],(struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],&pack_data) ;
    /*  Random Data Input */

    /* 写入存储硬件ecc 偏移位置参数 */
    memset(addr_buf,0,sizeof(addr_buf));
    addr_buf[0] = bytes_per_page + ecc_offset;
    addr_buf[1] = (bytes_per_page + ecc_offset) >> 8;
    memset((instance_p->dma_data_buffer.data_buffer + bytes_per_page) ,0xff,spare_bytes_per_page);
    ftnand_dma_pack_data pack_data2 =
        {
            .addr_p = addr_buf,
            .addr_length = 2,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer + bytes_per_page + ecc_offset,
            .phy_bytes_length = instance_p->nand_geometry[chip_addr].hw_ecc_length,
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    ftnand_dma_pack(&cmd_format[CMD_COPY_BACK_PROGRAM],(struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[FNAND_DESCRIPTORS_SIZE],&pack_data2) ;
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_WRITE_PAGE_TYPE);
    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);
    while (nand_state & FNAND_STATE_BUSY_OFFSET) /* wait busy state is over */
    {
        nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);
    }
    // printf("write after nand_state 0x%x \r\n",nand_state);
    while (ftnand_flash_read_status(instance_p, chip_addr) != FT_SUCCESS)
    {
		if(count++ > 0xff)
			return FNAND_IS_BUSY;
	}; /* wait i/o idle */

    return FT_SUCCESS;
}

static u32 ftnand_page_read_oob(ftnand *instance_p,u32 page_addr,u8* buf,u32 page_copy_offset,u32 length, u32 chip_addr)
{
    u32 ret;
    u8 addr_buf[5] = {0};
    u32 bytes_per_page = 0;
    u32 spare_bytes_per_page = 0;
    u32 memcpy_length = 0;
    bytes_per_page = instance_p->nand_geometry[chip_addr].bytes_per_page ;
    spare_bytes_per_page = instance_p->nand_geometry[chip_addr].spare_bytes_per_page ;

    ftnand_dma_pack_data pack_data =
        {
            .addr_p = addr_buf,
            .addr_length = 5,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = spare_bytes_per_page, /* 读取所有的oob 数据 */
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    addr_buf[4] = (page_addr >> 16);
    addr_buf[3] = (page_addr >> 8);
    addr_buf[2] = (page_addr);
    addr_buf[1] = ((bytes_per_page >> 8) & 0xff); /* 从oob 位置读取 */
    addr_buf[0] = (bytes_per_page & 0xff);

    ftnand_dma_pack(&cmd_format[CMD_READ_OPTION_NEW], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_READ_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    if(length && buf)
    {
        memcpy_length = (length > (spare_bytes_per_page - page_copy_offset)) ? (spare_bytes_per_page - page_copy_offset) : length;
        memcpy(buf, instance_p->dma_data_buffer.data_buffer + page_copy_offset, memcpy_length);
    }

    return FT_SUCCESS;
}

static u32 ftnand_page_write_oob(ftnand *instance_p,u32 page_addr,u8* buf,u32 spare_page_offset,u32 length, u32 chip_addr)
{
    u32 ret;
    u8 addr_buf[5] = {0};
    u32 bytes_per_page = 0;
    u32 spare_bytes_per_page = 0;
	u32 count=0;

    bytes_per_page = instance_p->nand_geometry[chip_addr].bytes_per_page;
    spare_bytes_per_page = instance_p->nand_geometry[chip_addr].spare_bytes_per_page ;
    memset(instance_p->dma_data_buffer.data_buffer, 0xff, spare_bytes_per_page);

    ftnand_dma_pack_data pack_data =
        {
            .addr_p = addr_buf,
            .addr_length = 5,
            .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer,
            .phy_bytes_length = (spare_bytes_per_page > length) ? length : spare_bytes_per_page,
            .chip_addr = chip_addr,
            .contiune_dma = 0,
        };

    addr_buf[4] = (page_addr >> 16);
    addr_buf[3] = (page_addr >> 8);
    addr_buf[2] = (page_addr);
    addr_buf[1] = ((bytes_per_page >> 8) & 0xff);
    addr_buf[0] = (bytes_per_page & 0xff);

    memcpy(instance_p->dma_data_buffer.data_buffer + spare_page_offset, buf ,((spare_bytes_per_page - spare_page_offset) >length)?length : (spare_bytes_per_page - spare_page_offset) );

    ftnand_dma_pack(&cmd_format[CMD_PAGE_PROGRAM], (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0], &pack_data);

    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_WRITE_PAGE_TYPE);

    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }
    while(ftnand_flash_read_status(instance_p,chip_addr) != FT_SUCCESS)
	{
		if(count++ > 0xff)
			return FNAND_IS_BUSY;
	};
    return FT_SUCCESS ;
}

static u32 ftnand_page_read_hw_ecc(ftnand *instance_p,u32 page_addr,u8* buf,u32 page_copy_offset,u32 length, u32 chip_addr)
{
    u32 ret;
    u32 nand_state = 0;
    u8 addr_buf[5] = {0};
    u32 memcpy_length = 0;
    volatile u32 wait_cnt;
    /* read operation */
    addr_buf[2] = page_addr;
	addr_buf[3] = page_addr >> 8;
	addr_buf[4] = page_addr >> 16;

    ftnand_dma_pack_data pack_data =
    {
        .addr_p = addr_buf,
        .addr_length = 5,
        .phy_address =  (unsigned long)instance_p->dma_data_buffer.data_buffer,
        .phy_bytes_length = instance_p->nand_geometry[chip_addr].bytes_per_page,
        .chip_addr = chip_addr,
        .contiune_dma  = 1,
    };

    ftnand_dma_pack(&cmd_format[CMD_READ_OPTION_NEW],(struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],&pack_data) ;

    /* Random Data Output */
    memset(addr_buf,0,sizeof(addr_buf));
    /* 读取存储硬件ecc 偏移位置参数 */
    addr_buf[0] = instance_p->nand_geometry[chip_addr].bytes_per_page + instance_p->nand_geometry[chip_addr].ecc_offset;
    addr_buf[1] = (instance_p->nand_geometry[chip_addr].bytes_per_page + instance_p->nand_geometry[chip_addr].ecc_offset) >> 8;

    ftnand_dma_pack_data pack_data2 =
    {
        .addr_p = addr_buf,
        .addr_length = 2,
        .phy_address = (unsigned long)instance_p->dma_data_buffer.data_buffer + instance_p->nand_geometry[chip_addr].bytes_per_page ,
        .phy_bytes_length = instance_p->nand_geometry[chip_addr].hw_ecc_length,
        .chip_addr = chip_addr,
        .contiune_dma = 0,
    };

    ftnand_dma_pack(&cmd_format[CMD_RANDOM_DATA_OUT],(struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[16],&pack_data2) ;
    ret = ftnand_send_cmd(instance_p, (struct ftnand_dma_descriptor *)&instance_p->descriptor_buffer.data_buffer[0],FNAND_WAIT_ECC_TYPE);
    if(ret != FT_SUCCESS)
    {
        return FNAND_ERR_OPERATION;
    }

    for(wait_cnt=0;wait_cnt< 0xffffff;wait_cnt++)
        ;
    // fsleep_microsec(100);
    /* 增加判断bit(16) 是否ecc 正忙 */
    nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);

    while ((nand_state &FNAND_STATE_ECC_BUSY_OFFSET) && ((nand_state &FNAND_STATE_ECC_FINISH_OFFSET)==0) )
    {
        nand_state = ftnand_readreg(instance_p->config.base_address, FNAND_STATE_OFFSET);
    }

    if (nand_state & FNAND_STATE_ECC_ERROVER_OFFSET)
    {
        FNAND_COMMON_DEBUG_E("FNAND_STATE_ECC_ERROVER %x ,page is %x \n",0,page_addr) ;
        return  FNAND_ERR_READ_ECC ;
    }
    else if(nand_state & FNAND_STATE_ECC_ERR_OFFSET)
    {
        int correct_num;
        FNAND_COMMON_DEBUG_W("FNAND ecc correct is in \r\n");
        correct_num = ftnand_correct_ecc(instance_p->config.base_address,instance_p->nand_geometry[chip_addr].ecc_step_size,
                        instance_p->nand_geometry[chip_addr].hw_ecc_steps,instance_p->dma_data_buffer.data_buffer,
                        instance_p->nand_geometry[chip_addr].bytes_per_page);

        if(correct_num < 0 )
        {
            FNAND_COMMON_DEBUG_W("CRC ECC IS ERROR \n") ;
            return FNAND_ERR_READ_ECC;
        }

        FNAND_COMMON_DEBUG_W("FNAND_STATE_ECC_ERR %x ,page is %x \n",correct_num,page_addr) ;
    }

    if(length && buf)
    {
        memcpy_length = (length > (pack_data.phy_bytes_length - page_copy_offset)) ? (pack_data.phy_bytes_length - page_copy_offset) : length;
        memcpy(buf, instance_p->dma_data_buffer.data_buffer + page_copy_offset, memcpy_length);
    }

    return FT_SUCCESS;
}

u32 ftnand_flash_read_page_raw(ftnand *instance_p,ftnand_op_data *op_data_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);
    u32 ret;
    ret = ftnand_page_read(instance_p, op_data_p->page_addr, op_data_p->page_buf, op_data_p->page_offset, op_data_p->page_length, op_data_p->chip_addr);

    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_read is error %x",__func__,ret);
        return ret;
    }

    if(op_data_p->obb_required)
    {
        ret = ftnand_page_read_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);

        if(ret != FT_SUCCESS)
        {
            FNAND_COMMON_DEBUG_E("%s,ftnand_page_read_oob is error %x",__func__,ret);
            return ret;
        }
    }

    return FT_SUCCESS;
}

u32 ftnand_flash_read_page_hw_ecc(ftnand *instance_p,ftnand_op_data *op_data_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);
    u32 ret;
    ftnand_config *config_p;
    config_p = &instance_p->config;
    //ftnand_flash_read_status(instance_p,op_data_p->chip_addr);

    ftnand_enable_hw_ecc(config_p->base_address);
    ret =ftnand_page_read_hw_ecc(instance_p,op_data_p->page_addr,op_data_p->page_buf,op_data_p->page_offset, op_data_p->page_length, op_data_p->chip_addr);
    ftnand_disable_hw_ecc(config_p->base_address);

    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_read_hw_ecc is error %x",__func__,ret);
        return ret;
    }


    if(op_data_p->obb_required)
    {
        ret = ftnand_page_read_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);
        if(ret != FT_SUCCESS)
        {
            FNAND_COMMON_DEBUG_E("%s,ftnand_page_read_oob is error %x",__func__,ret);
            return ret;
        }
    }

    return  FT_SUCCESS;
}

u32 ftnand_flash_write_page_raw(ftnand *instance_p,ftnand_op_data *op_data_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);
    u32 ret;
    if(op_data_p->obb_required)
    {
        ret = ftnand_page_write_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);

        if(ret != FT_SUCCESS)
        {
            FNAND_COMMON_DEBUG_E("%s,ftnand_page_write_oob is error %x",__func__,ret);
            return ret;
        }
    }

    ret = ftnand_page_write(instance_p, op_data_p->page_addr, op_data_p->page_buf,op_data_p->page_offset, op_data_p->page_length, op_data_p->chip_addr);

    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_write is error %x",__func__,ret);
        return ret;
    }

    return FT_SUCCESS;
}


u32 ftnand_flash_write_page_raw_hw_ecc(ftnand *instance_p,ftnand_op_data *op_data_p)
{
    u32 ret;
    ftnand_config *config_p;
    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);

    ftnand_flash_read_status(instance_p,op_data_p->chip_addr);

    config_p = &instance_p->config;
    if(op_data_p->obb_required)
    {
        ret = ftnand_page_write_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);

        if(ret != FT_SUCCESS)
        {
            FNAND_COMMON_DEBUG_E("%s,ftnand_page_write_oob is error %x",__func__,ret);
            return ret;
        }
    }
    ftnand_enable_hw_ecc(config_p->base_address);
    ret = ftnand_page_write_hw_ecc(instance_p, op_data_p->page_addr, op_data_p->page_buf,op_data_p->page_offset, op_data_p->page_length, op_data_p->chip_addr);
    ftnand_disable_hw_ecc(config_p->base_address);

    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_write is error %x",__func__,ret);
        return ret;
    }

    return FT_SUCCESS;
}


u32 ftnand_flash_oob_read(ftnand *instance_p,ftnand_op_data *op_data_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);
    u32 ret;
    ftnand_config *config_p;
    config_p = &instance_p->config;

    ret = ftnand_page_read_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_read_oob is error %x",__func__,ret);
        return ret;
    }

    return FT_SUCCESS;
}



u32 ftnand_flash_oob_write(ftnand *instance_p,ftnand_op_data *op_data_p)
{

    ASSERT(instance_p != NULL);
    ASSERT(op_data_p != NULL);
    u32 ret;

    ret = ftnand_page_write_oob(instance_p,op_data_p->page_addr,op_data_p->oob_buf,op_data_p->oob_offset,op_data_p->oob_length,op_data_p->chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_COMMON_DEBUG_E("%s,ftnand_page_write_oob is error %x",__func__,ret);

    }
	return ret;
}


void ftnand_flash_func_register(ftnand *instance_p)
{
    ASSERT(instance_p != NULL);
    instance_p->write_p = ftnand_flash_write_page_raw ;
    instance_p->read_p  = ftnand_flash_read_page_raw ;
    instance_p->erase_p = ftnand_flash_erase_block ;
    instance_p->write_hw_ecc_p = ftnand_flash_write_page_raw_hw_ecc ;
    instance_p->read_hw_ecc_p = ftnand_flash_read_page_hw_ecc ;
    instance_p->write_oob_p = ftnand_flash_oob_write;
    instance_p->read_oob_p = ftnand_flash_oob_read;
}


static int ftnand_id_has_period(u8 *id_data_p, int arrlen, int period)
{
	int i, j;
	for (i = 0; i < period; i++)
		for (j = i + period; j < arrlen; j += period)
			if (id_data_p[i] != id_data_p[j])
				return 0;
	return 1;
}

static int ftnand_id_len(u8 *id_data_p,int data_length)
{
    int last_nonzero, period;

	for (last_nonzero = data_length - 1; last_nonzero >= 0; last_nonzero--)
		if (id_data_p[last_nonzero])
			break;

    /* All zeros */
	if (last_nonzero < 0)
		return 0;

    /* Calculate wraparound period */
	for (period = 1; period < data_length; period++)
		if (ftnand_id_has_period(id_data_p, data_length, period))
			break;

    /* There's a repeated pattern */
	if (period < data_length)
		return period;

    /* There are trailing zeros */
	if (last_nonzero < data_length - 1)
		return last_nonzero + 1;

    /* No pattern detected */
	return data_length;

}


const ftnand_manufacturer *ftnand_get_manufacturer(u8 id)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(ftnand_manufacturers); i++)
    {
        if (ftnand_manufacturers[i].id == id)
        {
            return &ftnand_manufacturers[i];
        }
    }
	return NULL;
}


static u32 ftnand_id_detect(ftnand *instance_p,u32 chip_addr)
{
    u32 ret;
    u32 i;
    ftnand_id nand_id;
    u8* id_data = (u8*)&nand_id.data;
    u8 maf_id, dev_id;
    const ftnand_manufacturer *manufacturer_p ;

	/* step1  reset device */
    ret = ftnand_flash_reset(instance_p,chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_ID_DEBUG_E("ftnand_flash_reset is error");
        return ret;
    }

    /* step2  read device ID */
	memset(id_data,0,sizeof(nand_id.data));
    ret = ftnand_flash_read_id(instance_p,0,id_data,2,chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_ID_DEBUG_E("ftnand_flash_read_id is error");
        return ret;
    }

    for(i = 0; i < sizeof(nand_id.data); i++)
    {
        FNAND_ID_DEBUG_I("id_data[%d] 0x%x",i,id_data[i]);
    }

    /* Read manufacturer and device IDs */
	maf_id = id_data[0];
	dev_id = id_data[1];


    /* step 3 get entire device ID*/
    ret = ftnand_flash_read_id(instance_p,0,id_data,sizeof(nand_id.data),chip_addr);
    if(ret != FT_SUCCESS)
    {
        FNAND_ID_DEBUG_E("ftnand_flash_read_id is error");
        return ret;
    }

	for(i = 0; i < sizeof(nand_id.data); i++)
    {
        FNAND_ID_DEBUG_I("id_data[%d] 0x%x",i,id_data[i]);
    }
     /* step 5 compare ID string and device id */
    if (id_data[0] != maf_id || id_data[1] != dev_id) {
		FNAND_ID_DEBUG_E("second ID read did not match %02x,%02x against %02x,%02x\n",
			maf_id, dev_id, id_data[0], id_data[1]);
		return FNAND_ERR_NOT_MATCH;
	}

    nand_id.len = ftnand_id_len(id_data, ARRAY_SIZE(nand_id.data));

    /* step 6 通过maf_id获取对应的参数 */
    manufacturer_p = ftnand_get_manufacturer(maf_id);

    if(manufacturer_p == NULL)
    {
        FNAND_ID_DEBUG_E("Not find manufacturer");
        return FNAND_ERR_NOT_MATCH;
    }
    FNAND_ID_DEBUG_I("find manufacturer");

    return FT_SUCCESS;
}

u32 ftnand_detect(ftnand *instance_p)
{
    u32 ret;
    u32 i = 0;

    for (i = 0; i < FNAND_CONNECT_MAX_NUM; i++)
    {
    	ret = ftnand_onfi_init(instance_p, i);
        if(ret != FT_SUCCESS)
        {
            FNAND_ID_DEBUG_W("ftnand_onfi_init is error");
        }
        else
        {
            //FNAND_ID_DEBUG_I("Scan %d nand is onfi mode",i);
            instance_p->nand_flash_interface[i] = FNAND_ONFI_MODE;
            ret = ftnand_timing_interface_update(instance_p, i);
            if(ret != FT_SUCCESS)
            {
                FNAND_ID_DEBUG_E("ftnand_timing_interface_update is error");
                return ret;
            }
            /* open ecc length config ,需要确保 ecc 校验的空间必须小于oob 的空间*/


            ftnand_flash_func_register(instance_p) ;
            //continue;
        }

        ret = ftnand_id_detect(instance_p, i);
        if(ret != FT_SUCCESS)
        {
            FNAND_ID_DEBUG_W("ftnand_id_detect is error");
        }
        else
        {
            //FNAND_ID_DEBUG_I("ftnand_id_detect is ok",i);
            ftnand_flash_func_register(instance_p) ;
            continue;
        }


        ret = ftnand_toggle_init(instance_p, i); /* toggle 1.0 */
        if(ret != FT_SUCCESS)
        {
            FNAND_ID_DEBUG_W("ftnand_toggle_init is error");
        }
        else
        {

            instance_p->nand_flash_interface[i] = FNAND_TOGGLE_MODE;
            ret = ftnand_timing_interface_update(instance_p, i);
            if(ret != FT_SUCCESS)
            {
                FNAND_ID_DEBUG_E("ftnand_timing_interface_update is error");
                return ret;
            }
            /* open ecc length config */


            ftnand_flash_func_register(instance_p) ;
            continue;
        }

    }
    return FT_SUCCESS;
}


/**
 * @name: ftnand_init_bbt_desc
 * @msg:  This function initializes the Bad Block Table(BBT) descriptors with a
 * predefined pattern for searching Bad Block Table(BBT) in flash.
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @return {*}
 * @note:
 */
void ftnand_init_bbt_desc(ftnand *instance_p)
{
	u32 i;
	int index;
	ASSERT(instance_p != NULL);
	ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);

	for (i = 0; i < FNAND_CONNECT_MAX_NUM; i++)
	{
		/*
        * Initialize primary Bad Block Table(BBT)
        */
		instance_p->bbt_manager[i].bbt_desc.page_offset = FNAND_BBT_DESC_PAGE_OFFSET;
		instance_p->bbt_manager[i].bbt_desc.sig_offset = FNAND_BBT_DESC_SIG_OFFSET;
		instance_p->bbt_manager[i].bbt_desc.ver_offset = FNAND_BBT_DESC_VER_OFFSET;
		instance_p->bbt_manager[i].bbt_desc.max_blocks = FNAND_BBT_DESC_MAX_BLOCKS;
		instance_p->bbt_manager[i].bbt_desc.sig_length = FNAND_BBT_DESC_SIG_LEN;
		strcpy(&instance_p->bbt_manager[i].bbt_desc.signature[0], "Bbt0");
		instance_p->bbt_manager[i].bbt_desc.version = 0;
		instance_p->bbt_manager[i].bbt_desc.valid = 0;
		/*
        * Initialize mirror Bad Block Table(BBT)
        */
		instance_p->bbt_manager[i].bbt_mirror_desc.page_offset = FNAND_BBT_DESC_PAGE_OFFSET;
		instance_p->bbt_manager[i].bbt_mirror_desc.sig_offset = FNAND_BBT_DESC_SIG_OFFSET;
		instance_p->bbt_manager[i].bbt_mirror_desc.ver_offset = FNAND_BBT_DESC_VER_OFFSET;
		instance_p->bbt_manager[i].bbt_mirror_desc.sig_length = FNAND_BBT_DESC_SIG_LEN;
		instance_p->bbt_manager[i].bbt_mirror_desc.max_blocks = FNAND_BBT_DESC_MAX_BLOCKS;
		strcpy(&instance_p->bbt_manager[i].bbt_mirror_desc.signature[0], "1tbB");
		instance_p->bbt_manager[i].bbt_mirror_desc.version = 0;
		instance_p->bbt_manager[i].bbt_mirror_desc.valid = 0;

		/*
        * Initialize Bad block search pattern structure
        */
		if (instance_p->nand_geometry[i].bytes_per_page > 512)
		{
			/* For flash page size > 512 bytes */
			instance_p->bbt_manager[i].bb_pattern.options = FNAND_BBT_SCAN_2ND_PAGE;
			instance_p->bbt_manager[i].bb_pattern.offset =
				FNAND_BB_PATTERN_OFFSET_LARGE_PAGE;
			instance_p->bbt_manager[i].bb_pattern.length =
				FNAND_BB_PATTERN_LENGTH_LARGE_PAGE;
		}
		else
		{
			instance_p->bbt_manager[i].bb_pattern.options = FNAND_BBT_SCAN_2ND_PAGE;
			instance_p->bbt_manager[i].bb_pattern.offset =
				FNAND_BB_PATTERN_OFFSET_SMALL_PAGE;
			instance_p->bbt_manager[i].bb_pattern.length =
				FNAND_BB_PATTERN_LENGTH_SMALL_PAGE;
		}
		for (index = 0; index < FNAND_BB_PATTERN_LENGTH_LARGE_PAGE; index++)
		{
			instance_p->bbt_manager[i].bb_pattern.pattern[index] = FNAND_BB_PATTERN;
		}
	}
}

/**
 * @name: ftnand_convert_bbt
 * @msg:  Convert bitmask read in flash to information stored in RAM
 * @return {*}
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u8} *buf is buffer to store the bitmask
 * @param {u32} chip_addr is chip address
 */
static void ftnand_convert_bbt(ftnand *instance_p, u8 *buf, u32 chip_addr)
{
	u32 block_offset;
	u32 block_shift;
	u32 data;
	u8 block_type;
	u32 block_index;
	u32 bbtlen = instance_p->nand_geometry[chip_addr].num_blocks >>
				 FNAND_BBT_BLOCK_SHIFT;

	for (block_offset = 0; block_offset < bbtlen; block_offset++)
	{
		data = buf[block_offset];

	/*
	* Clear the RAM based Bad Block Table(BBT) contents
	*/
		instance_p->bbt_manager[chip_addr].bbt[block_offset] = 0x0;
	FNAND_BBM_DEBUG_E("current block num is %d \r\n",block_offset << FNAND_BBT_BLOCK_SHIFT);
	/*
	* Loop through the every 4 blocks in the bitmap
	*/
		for (block_index = 0; block_index < FNAND_BBT_ENTRY_NUM_BLOCKS;
			 block_index++)
		{
			block_shift = FNAND_BBTBLOCKSHIFT(block_index);
			block_type = (data >> block_shift) &
						 FNAND_BLOCK_TYPE_MASK;
			switch (block_type)
			{
			case FNAND_FLASH_BLOCK_FACTORY_BAD:
				/* Factory bad block */
				instance_p->bbt_manager[chip_addr].bbt[block_offset] |=
					FNAND_BLOCK_FACTORY_BAD << block_shift;
				break;
			case FNAND_FLASH_BLOCK_RESERVED:
				/* Reserved block */
				instance_p->bbt_manager[chip_addr].bbt[block_offset] |=
					FNAND_BLOCK_RESERVED << block_shift;
				break;
			case FNAND_FLASH_BLOCK_BAD:
				/* Bad block due to wear */
				instance_p->bbt_manager[chip_addr].bbt[block_offset] |=
					FNAND_BLOCK_BAD << block_shift;
				break;
			default:
				/* Good block */
				/* The BBT entry already defaults to
						* zero */
				break;
			}
		}
	}
}

static u32 ftnand_write_bbt(ftnand *instance_p,
							ftnand_bbt_desc *desc_p,
							ftnand_bbt_desc *mirror_desc_p,
							u32 chip_addr)
{

	u32 block = instance_p->nand_geometry[chip_addr].num_blocks - 1;
	u8 buf[FNAND_MAX_BLOCKS >> FNAND_BBT_BLOCK_SHIFT];
	u8 sparebuf[FNAND_MAX_SPARE_SIZE];
	u8 mask[4] = {0x00, 0x01, 0x02, 0x03};
	u8 Data;
	u32 block_offset;
	u32 block_shift;
	u32 ret;
	u32 block_index;
	u32 index;
	u8 block_type;
	u32 bbtlen = instance_p->nand_geometry[chip_addr].num_blocks >>
				 FNAND_BBT_BLOCK_SHIFT;
	FNAND_BBM_DEBUG_I("ftnand_write_bbt bbtlen is %d",bbtlen);

	/*
	 * Find a valid block to write the Bad Block Table(BBT)
	 */
	if (!desc_p->valid)
	{
		for (index = 0; index < desc_p->max_blocks; index++)
		{
			block = (block - index);
			block_offset = block >> FNAND_BBT_BLOCK_SHIFT;
			block_shift = FNAND_BBTBLOCKSHIFT(block);
			block_type = (instance_p->bbt_manager[chip_addr].bbt[block_offset] >>
						  block_shift) &
						 FNAND_BLOCK_TYPE_MASK;
			switch (block_type)
			{
			case FNAND_BLOCK_BAD:
			case FNAND_BLOCK_FACTORY_BAD:
				continue;
			default:
				/* Good Block */
				break;
			}
			desc_p->page_offset = block *
								  instance_p->nand_geometry[chip_addr].pages_per_block;
			if (desc_p->page_offset != mirror_desc_p->page_offset)
			{
				/* Free block found */
				desc_p->valid = 1;
				break;
			}
		}

		/*
		 * Block not found for writing Bad Block Table(BBT)
		 */
		if (index >= desc_p->max_blocks)
		{
			return FNAND_VALUE_FAILURE;
		}
	}
	else
	{
		block = desc_p->page_offset / instance_p->nand_geometry[chip_addr].pages_per_block;
	}

	/*
	 * Convert the memory based BBT to flash based table
	 */
	memset(buf, 0xff, bbtlen);

	/*
	 * Loop through the number of blocks
	 */
	for (block_offset = 0; block_offset < bbtlen; block_offset++)
	{
		Data = instance_p->bbt_manager[chip_addr].bbt[block_offset];
		/*
		 * Calculate the bit mask for 4 blocks at a time in loop
		 */
		for (block_index = 0; block_index < FNAND_BBT_ENTRY_NUM_BLOCKS;
			 block_index++)
		{
			block_shift = FNAND_BBTBLOCKSHIFT(block_index);
			buf[block_offset] &= ~(mask[Data &
										FNAND_BLOCK_TYPE_MASK]
								   << block_shift);
			Data >>= FNAND_BBT_BLOCK_SHIFT;
		}
		// FNAND_BBM_DEBUG_I("buf[%d] 0x%x",block_offset,buf[block_offset]);
	}

	/*
	 * Write the Bad Block Table(BBT) to flash
	 */
	ret = ftnand_erase_block(instance_p,block,chip_addr);
	if (ret != FT_SUCCESS)
	{
		return ret;
	}

	/*
	 * Write the signature and version in the spare data area
	 */
	memset(sparebuf, 0xff, instance_p->nand_geometry[chip_addr].spare_bytes_per_page);
	memcpy(sparebuf + desc_p->sig_offset, &desc_p->signature[0],
		   desc_p->sig_length);
	memcpy(sparebuf + desc_p->ver_offset, &desc_p->version, 1);

	/*
	 * Write the BBT to page offset
	 */
	ftnand_write_page(instance_p,desc_p->page_offset,&buf[0],0,bbtlen,sparebuf,0,sizeof(sparebuf),chip_addr);
	if (ret != FT_SUCCESS)
	{
		return ret;
	}
	return FT_SUCCESS;
}

static u32 ftnand_update_bbt(ftnand *instance_p, u32 chip_addr)
{
	u32 ret;
	u8 version;

	/*
	* Update the version number
	*/
	version = instance_p->bbt_manager[chip_addr].bbt_desc.version;
	instance_p->bbt_manager[chip_addr].bbt_desc.version = (version + 1) % 256;
	version = instance_p->bbt_manager[chip_addr].bbt_mirror_desc.version;
	instance_p->bbt_manager[chip_addr].bbt_mirror_desc.version = (version + 1) % 256;

	/*
	* Update the primary Bad Block Table(BBT) in flash
	*/
	ret = ftnand_write_bbt(instance_p,
						&instance_p->bbt_manager[chip_addr].bbt_desc,
						&instance_p->bbt_manager[chip_addr].bbt_mirror_desc, chip_addr);
	if (ret != FT_SUCCESS)
	{
		return ret;
	}

	/*
	* Update the mirrored Bad Block Table(BBT) in flash
	*/
	ret = ftnand_write_bbt(instance_p,
						&instance_p->bbt_manager[chip_addr].bbt_mirror_desc,
						&instance_p->bbt_manager[chip_addr].bbt_desc, chip_addr);
	if (ret != FT_SUCCESS)
	{
		return ret;
	}

	return FT_SUCCESS;
}

static u32 ftnand_mark_bbt(ftnand *instance_p,
						   ftnand_bbt_desc *desc_p,
						   u32 chip_addr)
{

	u32 block_index;
	u32 block_offset;
	u8 block_shift;
	u8 old_val;
	u8 new_val;
	u32 ret;
	u32 updatebbt = 0;
	u32 index;

	/*
	 * Mark the last four blocks as Reserved
	 */
	block_index = instance_p->nand_geometry[chip_addr].num_blocks - desc_p->max_blocks;

	for (index = 0; index < desc_p->max_blocks; index++, block_index++)
	{
		block_offset = block_index >> FNAND_BBT_BLOCK_SHIFT;
		block_shift = FNAND_BBTBLOCKSHIFT(block_index);
		old_val = instance_p->bbt_manager[chip_addr].bbt[block_offset];
		new_val = old_val | (FNAND_BLOCK_RESERVED << block_shift);
		instance_p->bbt_manager[chip_addr].bbt[block_offset] = new_val;

		if (old_val != new_val)
		{
			updatebbt = 1;
		}
	}

	/*
	 * Update the BBT to flash
	 */
	if (updatebbt)
	{
		ret = ftnand_update_bbt(instance_p, chip_addr);
		if (ret != FT_SUCCESS)
		{
			return ret;
		}
	}
	return FT_SUCCESS;
}


static void ftnand_create_bbt(ftnand *instance_p, u32 chip_addr)
{
	u32 block_index;
	u32 page_index;
	u32 length;
	u32 block_offset;
	u32 block_shift;
	u32 num_pages;
	u32 page;
	u8 buf[FNAND_MAX_SPARE_SIZE];
	u32 bbt_len = instance_p->nand_geometry[chip_addr].num_blocks >>
				  FNAND_BBT_BLOCK_SHIFT;
	u32 ret;
	/*
	 * Number of pages to search for bad block pattern
	 */
	if (instance_p->bbt_manager[chip_addr].bb_pattern.options & FNAND_BBT_SCAN_2ND_PAGE)
	{
		num_pages = 2;
	}
	else
	{
		num_pages = 1;
	}

	/*
	 * Zero the RAM based Bad Block Table(BBT) entries
	 */
	memset(&instance_p->bbt_manager[chip_addr].bbt[0], 0, bbt_len);

	/*
	 * Scan all the blocks for factory marked bad blocks
	 */
	for (block_index = 0; block_index <
						  instance_p->nand_geometry[chip_addr].num_blocks;
		 block_index++)
	{
		/*
		 * Block offset in Bad Block Table(BBT) entry
		 */
		block_offset = block_index >> FNAND_BBT_BLOCK_SHIFT;
		/*
		 * Block shift value in the byte
		 */
		block_shift = FNAND_BBTBLOCKSHIFT(block_index);
		page = block_index * instance_p->nand_geometry[chip_addr].pages_per_block;

		/*
		 * Search for the bad block pattern
		 */
		for (page_index = 0; page_index < num_pages; page_index++)
		{
			ret = ftnand_read_page_oob(instance_p,page + page_index,buf,0,sizeof(buf),chip_addr);
			if (ret != FT_SUCCESS)
			{
				FNAND_BBM_DEBUG_E("%s ftnand_read_page_oob is error",__func__);
				/* Marking as bad block */
				instance_p->bbt_manager[chip_addr].bbt[block_offset] |=
					(FNAND_BLOCK_FACTORY_BAD << block_shift);
				break;
			}

			/*
			 * Read the spare bytes to check for bad block
			 * pattern
			 */
			for (length = 0; length <
							 instance_p->bbt_manager[chip_addr].bb_pattern.length;
				 length++)
			{
				if (buf[instance_p->bbt_manager[chip_addr].bb_pattern.offset + length] !=
					instance_p->bbt_manager[chip_addr].bb_pattern.pattern[length])
				{
					/* Bad block found */
					instance_p->bbt_manager[chip_addr].bbt[block_offset] |=
						(FNAND_BLOCK_FACTORY_BAD << block_shift);
					FNAND_BBM_DEBUG_E("Bad block found block is %d",page + page_index);
					break;
				}
			}
		}
	}
}

u32 ftnand_search_bbt(ftnand *instance_p, ftnand_bbt_desc *desc, u32 chip_addr)
{
	u32 start_block;
	u32 sig_offset;
	u32 ver_offset;
	u32 max_blocks;
	u32 pageoff;
	u32 sig_length;
	u8 buf[FNAND_MAX_SPARE_SIZE];
	u32 block;
	u32 offset;
	u32 ret;

	start_block = instance_p->nand_geometry[chip_addr].num_blocks - 1; /* start block is last block start */
	sig_offset = desc->sig_offset;
	ver_offset = desc->ver_offset;
	max_blocks = desc->max_blocks;
	sig_length = desc->sig_length;
	FNAND_BBM_DEBUG_I("ftnand_search_bbt is start 0x%x",start_block);
	FNAND_BBM_DEBUG_I("pages_per_block is start %d",instance_p->nand_geometry[chip_addr].pages_per_block);
	for (block = 0; block < max_blocks; block++)
	{
		pageoff = (start_block - block) *
				  instance_p->nand_geometry[chip_addr].pages_per_block;
		FNAND_BBM_DEBUG_I("block 0x%x",block);
		FNAND_BBM_DEBUG_I("%s, pageoff is 0x%x", __func__, pageoff);
		ret = ftnand_read_page_oob(instance_p, pageoff, buf, 0, sizeof(buf), chip_addr);
		if (ret != FT_SUCCESS)
		{
			continue;
		}

		/*
		 * Check the Bad Block Table(BBT) signature
		 */
		for (offset = 0; offset < sig_length; offset++)
		{
			if (buf[offset + sig_offset] != desc->signature[offset])
			{
				break; /* Check the next blocks */
			}
		}
		if (offset >= sig_length)
		{
			/*
			 * Bad Block Table(BBT) found
			 */
			desc->page_offset = pageoff;
			desc->version = buf[ver_offset];
			desc->valid = 1;
			return FT_SUCCESS;
		}
	}
	/*
	 * Bad Block Table(BBT) not found
	 */
	return FNAND_VALUE_ERROR;
}

/**
 * @name:
 * @msg:
 * @return {*}
 * @note:
 * @param {ftnand} *instance_p
 * @param {u32} chip_addr
 */
u32 ftnand_read_bbt(ftnand *instance_p, u32 chip_addr)
{

	u8 buf[FNAND_MAX_BLOCKS >> FNAND_BBT_BLOCK_SHIFT];
	u32 status1;
	u32 status2;
	u32 ret;
	u32 bbtlen;


	ftnand_bbt_desc *desc_p = &instance_p->bbt_manager[chip_addr].bbt_desc;
	ftnand_bbt_desc *mirror_desc_p = &instance_p->bbt_manager[chip_addr].bbt_mirror_desc;

	bbtlen = instance_p->nand_geometry[chip_addr].num_blocks >> FNAND_BBT_BLOCK_SHIFT; /* 根据nand 介质信息获取的总块数，除以4 的含义为每字节存储4个块信息 */
	FNAND_BBM_DEBUG_I("ftnand_read_bbt ,bbtlen is %d",bbtlen);

	status1 = ftnand_search_bbt(instance_p, desc_p, chip_addr);
	status2 = ftnand_search_bbt(instance_p, mirror_desc_p, chip_addr);

	if ((status1 != FT_SUCCESS) && (status2 != FT_SUCCESS))
	{
		FNAND_BBM_DEBUG_E("ftnand_read_bbt error status1 %x, status2 %x", status1, status2);
		return FNAND_VALUE_FAILURE;
	}

	/*
	 * Bad Block Table found
	 */

	if (desc_p->valid && mirror_desc_p->valid)
	{
		if (desc_p->version > mirror_desc_p->version)
		{
			/*
			* Valid BBT & Mirror BBT found
			*/
			FNAND_BBM_DEBUG_I("desc_p->version > mirror_desc_p->version read page is %d",desc_p->page_offset);
			ret = ftnand_read_page(instance_p,desc_p->page_offset,buf,0,bbtlen,NULL,0,0,chip_addr);
			if (ret != FT_SUCCESS)
			{
				FNAND_BBM_DEBUG_I("desc_p->version > mirror_desc_p->version read page is error 0x%x",ret);
				return ret;
			}
			/*
			 * Convert flash BBT to memory based BBT
			*/
			ftnand_convert_bbt(instance_p, &buf[0], chip_addr);
			mirror_desc_p->version = desc_p->version;

			/*
			 * Write the BBT to Mirror BBT location in flash
			 */
			ret = ftnand_write_bbt(instance_p, mirror_desc_p,
								desc_p, chip_addr);
			if (ret != FT_SUCCESS)
			{
				return ret;
			}
		}
		else if (desc_p->version < mirror_desc_p->version)
		{
			FNAND_BBM_DEBUG_I("desc_p->version < mirror_desc_p->version read page is %d",mirror_desc_p->page_offset);
			ret = ftnand_read_page(instance_p,mirror_desc_p->page_offset,buf,0,bbtlen,NULL,0,0,chip_addr);
			if (ret != FT_SUCCESS)
			{
				FNAND_BBM_DEBUG_I("desc_p->version < mirror_desc_p->version read page is error 0x%x",ret);
				return ret;
			}
			/*
			 * Convert flash BBT to memory based BBT
			 */
			ftnand_convert_bbt(instance_p, &buf[0], chip_addr);
			desc_p->version = mirror_desc_p->version;

			/*
			 * Write the BBT to Mirror BBT location in flash
			 */
			ret = ftnand_write_bbt(instance_p, desc_p,
								mirror_desc_p, chip_addr);
			if (ret != FT_SUCCESS)
			{
				return ret;
			}
		}
		else
		{
			/* Both are up-to-date */
			FNAND_BBM_DEBUG_I("Both are up-to-date read page is %d",desc_p->page_offset);

			ret = ftnand_read_page(instance_p,desc_p->page_offset,buf,0,bbtlen,NULL,0,0,chip_addr);
			if (ret != FT_SUCCESS)
			{
				FNAND_BBM_DEBUG_I("Both are up-to-date read page is error 0x%x",ret);
				return ret;
			}

			/*
			 * Convert flash BBT to memory based BBT
			*/
			ftnand_convert_bbt(instance_p, &buf[0], chip_addr);
		}
	}
	else if (desc_p->valid)
	{
		/*
		* Valid Primary BBT found
		*/

		FNAND_BBM_DEBUG_I("Valid Primary BBT found is %d",desc_p->page_offset);

		ret = ftnand_read_page(instance_p,desc_p->page_offset,buf,0,bbtlen,NULL,0,0,chip_addr);
		if (ret != FT_SUCCESS)
		{
			FNAND_BBM_DEBUG_I("Valid Primary BBT found is error 0x%x",ret);
			return ret;
		}


		/*
			* Convert flash BBT to memory based BBT
		*/
		ftnand_convert_bbt(instance_p, &buf[0], chip_addr);
		desc_p->version = mirror_desc_p->version;

		/*
		* Write the BBT to Mirror BBT location in flash
		*/
		ret = ftnand_write_bbt(instance_p, mirror_desc_p,
							desc_p, chip_addr);
		if (ret != FT_SUCCESS)
		{
			return ret;
		}
	}
	else
	{
		/*
		 * Valid Mirror BBT found
		 */

		FNAND_BBM_DEBUG_I("Valid Mirror BBT found is %d",mirror_desc_p->page_offset);

		ret = ftnand_read_page(instance_p,mirror_desc_p->page_offset,buf,0,bbtlen,NULL,0,0,chip_addr);
		if (ret != FT_SUCCESS)
		{
			FNAND_BBM_DEBUG_I("Valid Mirror BBT found is error 0x%x",ret);
			return ret;
		}

		/*
		* Convert flash BBT to memory based BBT
		*/
		ftnand_convert_bbt(instance_p, &buf[0], chip_addr);
		desc_p->version = mirror_desc_p->version;

		/*
		* Write the BBT to Mirror BBT location in flash
		*/
		ret = ftnand_write_bbt(instance_p, desc_p,
							mirror_desc_p, chip_addr);
		if (ret != FT_SUCCESS)
		{
			return ret;
		}
	}
	return FT_SUCCESS;
}

void ft_dump_hex_word(const u32 *ptr, u32 buflen)
{
    u32 *buf = (u32 *)ptr;
    u8 *char_data = (u8 *)ptr;
    size_t i, j;
    buflen = buflen / 4;
    for (i = 0; i < buflen; i += 4)
    {
        //printf("%p: ", ptr + i);

        for (j = 0; j < 4; j++)
        {
            if (i + j < buflen)
            {
                printf("%08x ", buf[i + j]);
            }
            else
            {
                printf("   ");
            }
        }

        printf(" ");

        for (j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", (char)(__is_print(char_data[i + j]) ? char_data[i + j] : '.'));

        printf("\r\n");
    }
}


static void ftnand_bbt_dump_debug(ftnand *instance_p)
{
	int i;
	FNAND_BBM_DEBUG_W("/********************* master bbt descriptor **********************/");

	FNAND_BBM_DEBUG_E("page_offset 0x%x",instance_p->bbt_manager[0].bbt_desc.page_offset);   /* Page offset where BBT resides */
	FNAND_BBM_DEBUG_E("sig_offset 0x%x",instance_p->bbt_manager[0].bbt_desc.sig_offset);    /* Signature offset in Spare area */
	FNAND_BBM_DEBUG_E("ver_offset 0x%x",instance_p->bbt_manager[0].bbt_desc.ver_offset);    /* Offset of BBT version */
	FNAND_BBM_DEBUG_E("sig_length 0x%x",instance_p->bbt_manager[0].bbt_desc.sig_length);    /* Length of the signature */
	FNAND_BBM_DEBUG_E("max_blocks 0x%x",instance_p->bbt_manager[0].bbt_desc.max_blocks);    /* Max blocks to search for BBT */
	for (i = 0; i < 4; i++)
	{
		FNAND_BBM_DEBUG_E("signature[%d] %c",i,instance_p->bbt_manager[0].bbt_desc.signature[i]);
	}
	FNAND_BBM_DEBUG_E("version 0x%x",instance_p->bbt_manager[0].bbt_desc.version);        /* BBT version */
	FNAND_BBM_DEBUG_E("valid 0x%x",instance_p->bbt_manager[0].bbt_desc.valid);         /* BBT descriptor is valid or not */

	FNAND_BBM_DEBUG_W("/********************* mirroe bbt descriptor **********************/");
	FNAND_BBM_DEBUG_E("page_offset 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.page_offset);   /* Page offset where BBT resides */
	FNAND_BBM_DEBUG_E("sig_offset 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.sig_offset);    /* Signature offset in Spare area */
	FNAND_BBM_DEBUG_E("ver_offset 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.ver_offset);    /* Offset of BBT version */
	FNAND_BBM_DEBUG_E("sig_length 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.sig_length);    /* Length of the signature */
	FNAND_BBM_DEBUG_E("max_blocks 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.max_blocks);    /* Max blocks to search for BBT */
	for (i = 0; i < 4; i++)
	{
		FNAND_BBM_DEBUG_E("signature[%d] %c",i,instance_p->bbt_manager[0].bbt_mirror_desc.signature[i]);
	}
	FNAND_BBM_DEBUG_E("version 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.version);        /* BBT version */
	FNAND_BBM_DEBUG_E("valid 0x%x",instance_p->bbt_manager[0].bbt_mirror_desc.valid);         /* BBT descriptor is valid or not */


	FNAND_BBM_DEBUG_W("/********************* bbt info **********************/");
	ft_dump_hex_word((const u32 *)instance_p->bbt_manager[0].bbt,instance_p->nand_geometry[0].num_blocks >>
				 FNAND_BBT_BLOCK_SHIFT);
}

/**
 * @name: ftnand_scan_bbt
 * @msg:  This function reads the Bad Block Table(BBT) if present in flash.
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} chip_addr is chip address
 * @return {u32}
 */
u32 ftnand_scan_bbt(ftnand *instance_p, u32 chip_addr)
{
	u32 ret;
	ASSERT(instance_p != NULL);
	ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);

	if (ftnand_read_bbt(instance_p, chip_addr) != FT_SUCCESS)
	{
		/*
		* Create memory based Bad Block Table(BBT)
		*/
		ftnand_create_bbt(instance_p, chip_addr);

		/*
		* Write the Bad Block Table(BBT) to the flash
		*/
		ret = ftnand_write_bbt(instance_p,
							&instance_p->bbt_manager[chip_addr].bbt_desc,
							&instance_p->bbt_manager[chip_addr].bbt_mirror_desc, chip_addr);
		if (ret != FT_SUCCESS)
		{
			return ret;
		}

		/*
		* Write the Mirror Bad Block Table(BBT) to the flash
		*/
		ret = ftnand_write_bbt(instance_p,
							&instance_p->bbt_manager[chip_addr].bbt_mirror_desc,
							&instance_p->bbt_manager[chip_addr].bbt_desc, chip_addr);
		if (ret != FT_SUCCESS)
		{
			return ret;
		}

		/*
		* Mark the blocks containing Bad Block Table(BBT) as Reserved
		*/
		ftnand_mark_bbt(instance_p, &instance_p->bbt_manager[chip_addr].bbt_desc, chip_addr);
		ftnand_mark_bbt(instance_p, &instance_p->bbt_manager[chip_addr].bbt_mirror_desc, chip_addr);

		FNAND_BBM_DEBUG_I("New bbt is ready");

		ftnand_bbt_dump_debug(instance_p) ;

	}
	else
	{
		FNAND_BBM_DEBUG_I("old bbt is valid");
		ftnand_bbt_dump_debug(instance_p) ;
	}

	return FT_SUCCESS;
}

/**
 * @name:  ftnand_is_block_bad
 * @msg:   This function checks whether a block is bad or not.
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} block is the ftnand flash block number
 * @param {u32} chip_addr is chip address
 * @return {u32}  FT_SUCCESS if block is bad
 */
u32 ftnand_is_block_bad(ftnand *instance_p, u32 block, u32 chip_addr)
{
	u8 data;
	u8 block_shift;
	u8 BlockType;
	u32 BlockOffset;

	ASSERT(instance_p != NULL);
	ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
	//FASSERT_MSG(block < instance_p->nand_geometry[chip_addr].num_blocks,"block is %d,num_blocks is %d",block,instance_p->nand_geometry[chip_addr].num_blocks);

	BlockOffset = block >> FNAND_BBT_BLOCK_SHIFT;
	block_shift = FNAND_BBTBLOCKSHIFT(block);
	data = instance_p->bbt_manager[chip_addr].bbt[BlockOffset]; /* Block information in BBT */
	BlockType = (data >> block_shift) & FNAND_BLOCK_TYPE_MASK;

	if (BlockType != FNAND_BLOCK_GOOD)
		return FT_SUCCESS;
	else
		return FNAND_VALUE_FAILURE;
}



/**
 * @name: ftnand_mark_block_bad
 * @msg:  This function marks a block as bad in the RAM based Bad Block Table(BBT).
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} block is the ftnand flash block number
 * @param {u32} chip_addr is chip address
 * @return {*} FT_SUCCESS if successful
 */
u32 ftnand_mark_block_bad(ftnand *instance_p, u32 block, u32 chip_addr)
{
	u8 data;
	u8 block_shift;
	u32 block_offset;
	u8 oldval;
	u8 newval;
	u32 Status;

	ASSERT(instance_p != NULL);
	ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
	ASSERT(block < instance_p->nand_geometry[chip_addr].num_blocks);

	block_offset = block >> FNAND_BBT_BLOCK_SHIFT;
	block_shift = FNAND_BBTBLOCKSHIFT(block);
	data = instance_p->bbt_manager[chip_addr].bbt[block_offset]; /* Block information in BBT */

	/*
	 * Mark the block as bad in the RAM based Bad Block Table
	 */
	oldval = data;
	data &= ~(FNAND_BLOCK_TYPE_MASK << block_shift);
	data |= (FNAND_BLOCK_BAD << block_shift);
	newval = data;
	instance_p->bbt_manager[chip_addr].bbt[block_offset] = data;

	/*
	 * Update the Bad Block Table(BBT) in flash
	 */
	if (oldval != newval)
	{
		Status = ftnand_update_bbt(instance_p, chip_addr);
		if (Status != FT_SUCCESS)
		{
			return Status;
		}
	}
	return FT_SUCCESS;
}

/**
 * @name: ftnand_scan
 * @msg:  Nand scanning
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @return {FT_SUCCESS} Scan nand is ok
 */
u32 ftnand_scan(ftnand *instance_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    return ftnand_detect(instance_p);

}

u32 ftnand_check_busy(ftnand *instance_p)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ftnand_config *config_p;
    config_p = &instance_p->config;

    return ftnand_readreg(config_p->base_address, FNAND_STATE_OFFSET) & FNAND_STATE_BUSY_OFFSET;
}

void ftnand_enable(uintptr_t base_address)
{
    ftnand_setbit(base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_EN_MASK); /* 使能控制器 */
}


void ftnand_hw_reset(uintptr_t base_address)
{
    ftnand_writereg(base_address,FNAND_INTRMASK_OFFSET,FNAND_INTRMASK_ALL_INT_MASK); /* 屏蔽所有中断位 */
    ftnand_writereg(base_address, FNAND_STATE_OFFSET, FNAND_STATE_ALL_BIT);      /* 清空所有状态位 */
    ftnand_writereg(base_address, FNAND_ERROR_CLEAR_OFFSET, FNAND_ERROR_ALL_CLEAR);  /* 清除所有错误位 */
    ftnand_writereg(base_address, FNAND_FIFO_FREE_OFFSET, FNAND_FIFO_FREE_MASK);     /* 清空fifo */
}


void ftnand_hw_init(uintptr_t base_address,ftnand_inter_mode inter_mode)
{
    ftnand_setbit(base_address, FNAND_CTRL1_OFFSET, FNAND_CTRL1_SAMPL_PHASE_MAKE(5UL));
    ftnand_clearbit(base_address, FNAND_CTRL1_OFFSET, FNAND_CTRL1_ECC_DATA_FIRST_EN_MASK);

    ftnand_writereg(base_address, FNAND_INTERVAL_OFFSET, FNAND_INTERVAL_TIME_MAKE(1UL)); /* 命令 、地址、数据之间的时间间隔 */
    ftnand_writereg(base_address, FNAND_FIFO_LEVEL0_FULL_OFFSET, FNAND_FIFO_LEVEL0_FULL_THRESHOLD_MASK & 4);   /* 满阈值配置 1/2 full, default 0 */
    ftnand_writereg(base_address, FNAND_FIFO_LEVEL1_EMPTY_OFFSET, FNAND_FIFO_LEVEL1_EMPTY_THRESHOLD_MASK & 4); /* 空阈值配置 1/2 empty, default 0 */
    ftnand_writereg(base_address, FNAND_FIFO_FREE_OFFSET, FNAND_FIFO_FREE_MASK);   /* 清空fifo 操作 */
    ftnand_writereg(base_address, FNAND_ERROR_CLEAR_OFFSET, FNAND_ERROR_CLEAR_DSP_ERR_CLR_MASK); /* 清除所有错误 */
    ftnand_writereg(base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(1UL) | FNAND_CTRL0_INTER_MODE((unsigned long)(inter_mode))); /*  纠错为2位 ，使用配置的模式 */
}

/**
 * @name: ftnand_isr_enable
 * @msg:  Enable the corresponding interrupt based on the interrupt mask
 * @return {*}
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance
 * @param {u32} int_mask is interrupt mask
 */
void ftnand_isr_enable(ftnand *instance_p, u32 int_mask)
{

    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ftnand_config *config_p;
    config_p = &instance_p->config;

    ftnand_clearbit(config_p->base_address, FNAND_INTRMASK_OFFSET, int_mask);
}

/**
 * @name: ftnand_irq_disable
 * @msg:  Disable the corresponding interrupt based on the interrupt mask
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance
 * @param {u32} int_mask is interrupt mask
 * @return {*}
 */
void ftnand_irq_disable(ftnand *instance_p, u32 int_mask)
{

    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ftnand_config *config_p;
    config_p = &instance_p->config;

    ftnand_setbit(config_p->base_address, FNAND_INTRMASK_OFFSET, int_mask);
}


/**
 * @name: ftnand_set_isr_handler
 * @msg:  Initializes isr event callback function
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {FnandIrqEventHandler} event_p is callback function used to respond to the interrupt event
 * @param {void} *irq_args  is the arguments of event callback
 * @return {*}
 */
void ftnand_set_isr_handler(ftnand *instance_p,FnandIrqEventHandler event_p,void *irq_args )
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    instance_p->irq_event_fun_p = event_p;
    instance_p->irq_args = irq_args;
}


/**
 * @name: ftnand_irq_handler
 * @msg:  Nand driver isr handler
 * @note:
 * @param {int} vector is interrupt number
 * @param {void} *param is argument
 * @return {*}
 */
void ftnand_irq_handler(int vector, void *param)
{
    ftnand *instance_p = (ftnand *)param;
    ftnand_config *config_p;
    u32 status;
    u32 en_irq;
    (void)vector;
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    config_p = &instance_p->config;

    status = ftnand_readreg(config_p->base_address, FNAND_INTR_OFFSET);
    en_irq = (~ftnand_readreg(config_p->base_address, FNAND_INTRMASK_OFFSET)) & FNAND_INTRMASK_ALL_INT_MASK;

    if ((status & en_irq) == 0)
    {
        FNAND_INTR_DEBUG_E("No irq exit");
        return;
    }

    ftnand_irq_disable(instance_p, status & FNAND_INTRMASK_ALL_INT_MASK);
    ftnand_writereg(config_p->base_address, 0xfd0, 0);
    ftnand_writereg(config_p->base_address, FNAND_INTR_OFFSET, status);

    if(instance_p->irq_event_fun_p)
    {
        if(status & FNAND_INTR_BUSY_MASK)
        {

            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_BUSY_EVENT);
        }

        if(status & FNAND_INTR_DMA_BUSY_MASK)
        {

            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_DMA_BUSY_EVENT);
        }

        if(status & FNAND_INTR_DMA_PGFINISH_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_DMA_PGFINISH_EVENT);
        }

        if(status & FNAND_INTR_DMA_FINISH_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_DMA_FINISH_EVENT);
        }

        if(status & FNAND_INTR_FIFO_EMP_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_FIFO_EMP_EVENT);
        }

        if(status & FNAND_INTR_FIFO_FULL_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_FIFO_FULL_EVENT);
        }

        if(status & FNAND_INTR_FIFO_TIMEOUT_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_FIFO_TIMEOUT_EVENT);
        }

        if(status & FNAND_INTR_CMD_FINISH_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_CMD_FINISH_EVENT);
        }

        if(status & FNAND_INTR_PGFINISH_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_PGFINISH_EVENT);
        }

        if(status & FNAND_INTR_RE_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_RE_EVENT);
        }

        if(status & FNAND_INTR_DQS_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_DQS_EVENT);
        }

        if(status & FNAND_INTR_RB_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_RB_EVENT);
        }

        if(status & FNAND_INTR_ECC_FINISH_MASK)
        {
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_ECC_FINISH_EVENT);
        }

        if(status & FNAND_INTR_ECC_ERR_MASK)
        {
            ftnand_writereg(config_p->base_address, FNAND_ERROR_CLEAR_OFFSET, FNAND_ERROR_CLEAR_ECC_ERR_CLR_MASK);
            ftnand_writereg(config_p->base_address, FNAND_FIFO_FREE_OFFSET, FNAND_FIFO_FREE_MASK);
            instance_p->irq_event_fun_p(instance_p->irq_args, FNAND_IRQ_ECC_ERR_EVENT);
        }
    }
}

u32 ftnand_send_cmd(ftnand *instance_p, struct ftnand_dma_descriptor *descriptor_p,ftnand_operation_type isr_type)
{
    ftnand_config *config_p;
    u32 timeout_cnt = 0;
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(isr_type < FNAND_TYPE_NUM);

    config_p = &instance_p->config;

    ftnand_hw_reset(config_p->base_address);

    if (0 != ftnand_check_busy(instance_p))
    {
        FNAND_DEBUG_E("Nand is busy");
        return FNAND_IS_BUSY;
    }

    /* write dma addr to register */
    ftnand_writereg(config_p->base_address, FNAND_MADDR0_OFFSET, ((unsigned long)descriptor_p) & FNAND_MADDR0_DT_LOW_ADDR_MASK);

#ifdef __aarch64__
    /* 将高位地址填入寄存器 */
    ftnand_clearbit(config_p->base_address, FNAND_MADDR1_OFFSET, FNAND_MADDR1_DT_HIGH_8BITADDR_MASK);
    ftnand_setbit(config_p->base_address, FNAND_MADDR1_OFFSET, ((unsigned long)descriptor_p >> 32) & FNAND_MADDR1_DT_HIGH_8BITADDR_MASK);
#else
    ftnand_clearbit(config_p->base_address, FNAND_MADDR1_OFFSET, FNAND_MADDR1_DT_HIGH_8BITADDR_MASK);
#endif
    /* 中断模式操作 */
    if (instance_p->work_mode == FNAND_WORK_MODE_ISR)
    {
        if(isr_type == FNAND_CMD_TYPE)
        {
            ftnand_isr_enable(instance_p, FNAND_INTRMASK_CMD_FINISH_MASK);
        }
        else if(isr_type == FNAND_WRITE_PAGE_TYPE)
        {
            ftnand_isr_enable(instance_p, FNAND_INTRMASK_PGFINISH_MASK);
        }
        else if(isr_type == FNAND_READ_PAGE_TYPE)
        {
            ftnand_isr_enable(instance_p, FNAND_INTRMASK_DMA_PGFINISH_MASK);
        }
        else if(isr_type == FNAND_WAIT_ECC_TYPE)
        {
            ftnand_isr_enable(instance_p, FNAND_INTRMASK_ECC_FINISH_MASK);
        }
    }

    ftnand_setbit(config_p->base_address, FNAND_MADDR1_OFFSET, FNAND_MADDR1_DMA_EN_MASK);

    if(instance_p->work_mode == FNAND_WORK_MODE_ISR && (instance_p->wait_irq_fun_p != NULL))
    {
        if(instance_p->wait_irq_fun_p)
        {
            if(instance_p->wait_irq_fun_p(instance_p->wait_args) != FT_SUCCESS)
            {
                FNAND_DEBUG_E("wait_irq_fun_p is failed");
                return FNAND_ERR_IRQ_OP_FAILED;
            }
        }
        else
        {
            FNAND_DEBUG_E("The lack of wait_irq_fun_p");
            ftnand_writereg(config_p->base_address, FNAND_INTRMASK_OFFSET, FNAND_INTRMASK_ALL_INT_MASK);
            return FNAND_ERR_IRQ_LACK_OF_CALLBACK;
        }

        return  FT_SUCCESS ;
    }
    else
    {
        if(isr_type == FNAND_CMD_TYPE)
        {
            while (0 == (ftnand_readreg(config_p->base_address, FNAND_STATE_OFFSET) & FNAND_STATE_CMD_PGFINISH_OFFSET))
            {
                if (timeout_cnt++ >= 0xffffff)
                {
                    FNAND_DEBUG_E("FNAND_CMD_TYPE is send timeout");
                    return FNAND_OP_TIMEOUT;
                }
            }
        }
        else if(isr_type == FNAND_WRITE_PAGE_TYPE)
        {
            while (0 == (ftnand_readreg(config_p->base_address, FNAND_STATE_OFFSET) & FNAND_STATE_PG_PGFINISH_OFFSET))
            {
                if (timeout_cnt++ >= 0xffffff)
                {
                    FNAND_DEBUG_E("FNAND_WRITE_PAGE_TYPE is send timeout");
                    return FNAND_OP_TIMEOUT;
                }
            }
        }
        else if(isr_type == FNAND_READ_PAGE_TYPE)
        {
            while (0 == (ftnand_readreg(config_p->base_address, FNAND_STATE_OFFSET) & FNAND_STATE_DMA_PGFINISH_OFFSET))
            {
                if (timeout_cnt++ >= 0xffffff)
                {
                    FNAND_DEBUG_E("FNAND_READ_PAGE_TYPE is send timeout");
                    return FNAND_OP_TIMEOUT;
                }
            }
        }
        else if(isr_type == FNAND_WAIT_ECC_TYPE)
        {
            while (0 == (ftnand_readreg(config_p->base_address, FNAND_STATE_OFFSET) & FNAND_STATE_ECC_FINISH_OFFSET))
            {
                if (timeout_cnt++ >= 0xffffff)
                {
                    FNAND_DEBUG_E("FNAND_WAIT_ECC_TYPE is send timeout");
                    return FNAND_OP_TIMEOUT;
                }
            }
        }
    }

    return FT_SUCCESS;
}


/**
 * @name:  ftnand_operation_wait_irq_register
 * @msg:   When nand is sent in interrupt mode, the action that waits while the operation completes
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {FNandOperationWaitIrqCallback} wait_irq_fun_p , When the user adds this function, return FT_SUCCESS reports success, otherwise failure
 * @param {void} *wait_args
 * @return {*}
 */
void ftnand_operation_wait_irq_register(ftnand *instance_p,FNandOperationWaitIrqCallback wait_irq_fun_p ,void *wait_args)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);

    instance_p->wait_irq_fun_p = wait_irq_fun_p;
    instance_p->wait_args = wait_args;
}



/**
 * @name: ftnand_cfg_initialize
 * @msg:  Initialize the NAND controller
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {ftnand_config} * points to the ftnand device configuration structure.
 * @return {u32} FT_SUCCESS if successful
 * @note:
 */
u32 ftnand_cfg_initialize(ftnand *instance_p,
                          ftnand_config *config_p)
{
    u32 i;
    u32 ret;
    /* Assert arguments */
    ASSERT(instance_p != NULL);
    ASSERT(config_p != NULL);

    /* Clear instance memory and make copy of configuration */
    memset(instance_p, 0, sizeof(ftnand));
    instance_p->config = *config_p;
    instance_p->is_ready = FT_COMPONENT_IS_READY;

    /* lsd config */
    ftnand_clearbit(FLSD_CONFIG_BASE,0xc0,1);

    instance_p->work_mode = FNAND_WORK_MODE_POLL ; /* 默认采用中断模式 */
    for (i = 0; i < FNAND_CONNECT_MAX_NUM;i++)
    {
        instance_p->inter_mode[i] = FNAND_ASYN_SDR; /* 初始化阶段以异步模式启动 */
        instance_p->timing_mode[i] = FNAND_TIMING_MODE0 ;
        /*  初始化时序配置 */
        ret  = ftnand_timing_interface_update(instance_p,i);
        if(ret != FT_SUCCESS)
        {
            FNAND_DEBUG_E("%s, ftnand_timing_interface_update is error",__func__);
            return ret;
        }
    }

    ftnand_hw_init(instance_p->config.base_address,instance_p->inter_mode[0]);
    ftnand_hw_reset(instance_p->config.base_address);

    /* init ecc strength */
    ftnand_clearbit(instance_p->config.base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(7UL)); /*  clear all ecc_correct  */
    if(instance_p->config.ecc_strength == 0x8)
    {
        ftnand_setbit(instance_p->config.base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(7UL));
    }
    else if(instance_p->config.ecc_strength == 0x4)
    {
        ftnand_setbit(instance_p->config.base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(3UL));
    }
    else if(instance_p->config.ecc_strength == 0x2)
    {
        ftnand_setbit(instance_p->config.base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(1UL));
    }
    else
    {
        ftnand_setbit(instance_p->config.base_address, FNAND_CTRL0_OFFSET, FNAND_CTRL0_ECC_CORRECT_MAKE(0UL));
    }


    ftnand_enable(instance_p->config.base_address);

    /* init bbm */
    ftnand_init_bbt_desc(instance_p);
    return (FT_SUCCESS);
}


/**
 * @name: ftnand_write_page
 * @msg:  Write operations one page at a time, including writing page data and spare data
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} page_addr is the address to which the page needs to be written
 * @param {u8} *buffer is page writes a pointer to the buffer
 * @param {u32} page_copy_offset is the offset of the page writing , Buffer write data to 0 + page_copy_offset
 * @param {u32} length is page data write length
 * @param {u8} *oob_buffer is the data buffer pointer needs to be written to the spare space
 * @param {u32} oob_copy_offset  is the offset of the spare space writing , Buffer write data to page length + oob_copy_offset
 * @param {u32} oob_length is the length to be written to the spare space
 * @param {u32} chip_addr chip address
 * @return {u32} FT_SUCCESS ,write page is successful
 */
u32 ftnand_write_page(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset ,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );
    ASSERT(instance_p->write_hw_ecc_p);

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0, /* 从offset开始拷贝页数据的长度 */
        .obb_required = 0,                /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = NULL,         /* obb 数据缓存空间 */
        .oob_offset = 0,             /* 从offset开始拷贝页数据 */
        .oob_length = 0,               /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,                   /* 芯片地址 */
    };


    if(buffer && (length > 0))
    {
        op_data.page_buf = buffer;
        op_data.page_length = length;
        op_data.page_offset= page_copy_offset;
    }

    if(oob_buffer && (oob_length > 0))
    {
        op_data.obb_required = 1;
        op_data.oob_buf = oob_buffer;
        op_data.oob_length = oob_length;
        op_data.oob_offset= oob_copy_offset;
    }

    return instance_p->write_hw_ecc_p(instance_p,&op_data);
}

/**
 * @name: ftnand_write_page
 * @msg:  Write operations one page at a time, including writing page data and spare data ,without hw ecc
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} page_addr is the address to which the page needs to be written
 * @param {u8} *buffer is page writes a pointer to the buffer
 * @param {u32} page_copy_offset is the offset of the page writing , Buffer write data to 0 + page_copy_offset
 * @param {u32} length is page data write length
 * @param {u8} *oob_buffer is the data buffer pointer needs to be written to the spare space
 * @param {u32} oob_copy_offset  is the offset of the spare space writing , Buffer write data to page length + oob_copy_offset
 * @param {u32} oob_length is the length to be written to the spare space
 * @param {u32} chip_addr chip address
 * @return {u32} FT_SUCCESS ,write page is successful
 */
u32 ftnand_write_page_raw(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset ,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );
    ASSERT(instance_p->write_hw_ecc_p);

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0, /* 从offset开始拷贝页数据的长度 */
        .obb_required = 0,                /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = NULL,         /* obb 数据缓存空间 */
        .oob_offset = 0,             /* 从offset开始拷贝页数据 */
        .oob_length = 0,               /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,                   /* 芯片地址 */
    };


    if(buffer && (length > 0))
    {
        op_data.page_buf = buffer;
        op_data.page_length = length;
        op_data.page_offset= page_copy_offset;
    }

    if(oob_buffer && (oob_length > 0))
    {
        op_data.obb_required = 1;
        op_data.oob_buf = oob_buffer;
        op_data.oob_length = oob_length;
        op_data.oob_offset= oob_copy_offset;
    }

    return instance_p->write_p(instance_p,&op_data);
}

/**
 * @name: ftnand_read_page
 * @msg:  Read operations one page at a time, including reading page data and spare space data
 * @note:
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} page_addr is the address to which the page needs to be readed
 * @param {u8} *buffer is the buffer used by the user to read page data
 * @param {u32} page_copy_offset  is the offset of the page reading , Buffer read data from 0 + page_copy_offset of per page
 * @param {u32} length is page data read length
 * @param {u8} *oob_buffer is buffer that read data from the spare space
 * @param {u32} oob_copy_offset is the offset of the spare space reading , Buffer reads data from page length + oob_copy_offset
 * @param {u32} oob_length  is the length to be written to the spare space
 * @param {u32} chip_addr chip address
 * @return {u32} FT_SUCCESS is read  successful
 */
u32 ftnand_read_page(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );
    ASSERT(instance_p->read_hw_ecc_p);

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0,            /* 从offset开始拷贝页数据的长度 */
        .obb_required = 0,           /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = NULL,             /* obb 数据缓存空间 */
        .oob_offset = 0,             /* 从offset开始拷贝页数据 */
        .oob_length = 0,             /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,      /* 芯片地址 */
    };

    /* clear buffer */
    if(buffer && (length > 0))
    {
        op_data.page_buf = buffer;
        op_data.page_length = length;
        op_data.page_offset= page_copy_offset;
    }

    if(oob_buffer && (oob_length > 0))
    {
        op_data.obb_required = 1;
        op_data.oob_buf = oob_buffer;
        op_data.oob_length = oob_length;
        op_data.oob_offset= oob_copy_offset;
    }

    return instance_p->read_hw_ecc_p(instance_p,&op_data);
}

u32 ftnand_read_page_raw(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );
    ASSERT(instance_p->read_hw_ecc_p);

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0,            /* 从offset开始拷贝页数据的长度 */
        .obb_required = 0,           /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = NULL,             /* obb 数据缓存空间 */
        .oob_offset = 0,             /* 从offset开始拷贝页数据 */
        .oob_length = 0,             /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,      /* 芯片地址 */
    };

    /* clear buffer */
    if(buffer && (length > 0))
    {
        op_data.page_buf = buffer;
        op_data.page_length = length;
        op_data.page_offset= page_copy_offset;
    }

    if(oob_buffer && (oob_length > 0))
    {
        op_data.obb_required = 1;
        op_data.oob_buf = oob_buffer;
        op_data.oob_length = oob_length;
        op_data.oob_offset= oob_copy_offset;
    }

    return instance_p->read_p(instance_p,&op_data);
}

/**
 * @name: ftnand_erase_block
 * @msg:  erase block data
 * @note: 擦除之后增加read status 命令进行检查。（70h）
 * @param {ftnand} *instance_p is the pointer to the ftnand instance.
 * @param {u32} block   is block number
 * @param {u32} chip_addr is chip address
 * @return {u32} FT_SUCCESS is erase is successful
 */
u32 ftnand_erase_block(ftnand *instance_p,u32 block,u32 chip_addr)
{
    u32 page_address;
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );
    page_address = block * instance_p->nand_geometry[chip_addr].pages_per_block;
    // printf("pages_per_block %d ,block %d \r\n", instance_p->nand_geometry[chip_addr].pages_per_block,block);
    return instance_p->erase_p(instance_p, page_address, chip_addr);
}



/**
 * @name: ftnand_read_page_oob
 * @msg:  Read spare space fo per page
 * @note:
 * @param {ftnand} *instance_p is the instance pointer
 * @param {u32} page_addr is the Row Address of the spare space needs to be read
 * @param {u8} *oob_buffer is the buffer used by the user to read spare space data
 * @param {u32} oob_copy_offset is the offset of the spare space reading , Buffer reads data from page length + page_copy_offset
 * @param {u32} oob_length is the length of data retrieved from spare space
 * @param {u32} chip_addr is chip address
 * @return {u32} FT_SUCCESS is read  successful
 */
u32 ftnand_read_page_oob(ftnand *instance_p,u32 page_addr,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0, /* 从offset开始拷贝页数据的长度 */
        .obb_required = 1,                /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = oob_buffer,         /* obb 数据缓存空间 */
        .oob_offset = oob_copy_offset,             /* 从offset开始拷贝页数据 */
        .oob_length = oob_length,               /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,                   /* 芯片地址 */
    };

    return instance_p->read_oob_p(instance_p,&op_data);
}

/**
 * @name: ftnand_write_page_oob
 * @msg:  write data to the spare space
 * @note:
 * @param {ftnand} *instance_p is the instance pointer
 * @param {u32} page_addr  is the Row Address of the spare space needs to be write
 * @param {u8} *oob_buffer is buffer that writes data to the spare space
 * @param {u32} page_copy_offset is the offset of the spare space writing , Buffer write data to page length + page_copy_offset
 * @param {u32} oob_length is the length to be written to the spare space
 * @param {u32} chip_addr is chip address
 * @return {u32} FT_SUCCESS is write  successful
 */
u32 ftnand_write_page_oob(ftnand *instance_p,u32 page_addr,u8 *oob_buffer,u32 page_copy_offset,u32 oob_length,u32 chip_addr)
{
    ASSERT(instance_p != NULL);
    ASSERT(instance_p->is_ready == FT_COMPONENT_IS_READY);
    ASSERT(chip_addr < FNAND_CONNECT_MAX_NUM );

    ftnand_op_data op_data =
    {
        .page_addr = page_addr,
        .page_buf = NULL,            /* page 数据缓存空间 */
        .page_offset = 0,            /* 从offset开始拷贝页数据 */
        .page_length = 0, /* 从offset开始拷贝页数据的长度 */
        .obb_required = 1,                /* obb 是否读取的标志位,1 需要操作oob 区域 */
        .oob_buf = oob_buffer,         /* obb 数据缓存空间 */
        .oob_offset = page_copy_offset,             /* 从offset开始拷贝页数据 */
        .oob_length = oob_length,               /* 从offset开始拷贝页数据的长度 */
        .chip_addr = chip_addr,                   /* 芯片地址 */
    };

    return instance_p->write_hw_ecc_p(instance_p,&op_data);
}


static inline u32 fio_pad_read(fpin_index pin)
{
    return ftin32(FIOPAD_BASE_ADDR + pin.reg_off);
}

static inline void fio_pad_write(fpin_index pin, u32 reg_val)
{
    ftout32(FIOPAD_BASE_ADDR + pin.reg_off, reg_val);
    return;
}

void fpin_set_config(const fpin_index pin, fpin_func func, fpin_pull pull, fpin_drive drive)
{
#if 0

    u32 reg_val = fio_pad_read(pin);

    reg_val &= ~FIOPAD_X_REG0_FUNC_MASK;
    reg_val |= FIOPAD_X_REG0_FUNC_SET(func);

    reg_val &= ~FIOPAD_X_REG0_PULL_MASK;
    reg_val |= FIOPAD_X_REG0_PULL_SET(pull);

    reg_val &= ~FIOPAD_X_REG0_DRIVE_MASK;
    reg_val |= FIOPAD_X_REG0_DRIVE_SET(drive);

    fio_pad_write(pin, reg_val);
#endif
	return;
}

ftnand_config *ftnand_lookup_config(u32 instance_id)
{
    ftnand_config *ptr = NULL;
    u32 index;

    for (index = 0; index < (u32)FNAND_NUM; index++)
    {
        if (ftnand_configtable[index].instance_id == instance_id)
        {
            ptr = &ftnand_configtable[index];
            break;
        }
    }

    return (ftnand_config *)ptr;

}

static u32 ftnand_test_init(void)
{
    u32 ret = 0;

    ret = ftnand_cfg_initialize(&ftnand_instance, ftnand_lookup_config(FNAND_INSTANCE0));
    if(ret != 0)
    {
        printf("ftnand_cfg_initialize is failed \r\n");
        return -1;
    }

    ret = ftnand_scan(&ftnand_instance);

    if(ret != 0)
    {
        printf("ftnand_scan is failed \r\n");
        return -2;
    }

    return ret;
}

int ftnand_test_init_entry(void)
{
    int ret = 0;
    *(u32 *)0x2807E0C0 = 0x0;

	fpin_set_config(FIOPAD_AL43,FPIN_FUNC1,FPIN_PULL_DOWN,FPIN_DRV4); /* nfc_dqs */
	fpin_set_config(FIOPAD_AL45,FPIN_FUNC1,FPIN_PULL_NONE,FPIN_DRV4); /* nfc_wp_n */

	fpin_set_config(FIOPAD_AC51, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_rb_n[0] */
	fpin_set_config(FIOPAD_AC49, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_wen_clk */
	fpin_set_config(FIOPAD_AE47, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_ren_wrn */
	fpin_set_config(FIOPAD_W47, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_cle */
	fpin_set_config(FIOPAD_W51, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_ale */
	fpin_set_config(FIOPAD_W49, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[0] */
	fpin_set_config(FIOPAD_U51, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[1] */
	fpin_set_config(FIOPAD_U49, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[2] */
	fpin_set_config(FIOPAD_AE45, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[3] */
	fpin_set_config(FIOPAD_AC45, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[4] */

	fpin_set_config(FIOPAD_AE43, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[5] */
	fpin_set_config(FIOPAD_AA43, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[6] */
	fpin_set_config(FIOPAD_AA45, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_data[7] */
	fpin_set_config(FIOPAD_W45, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_ce_n[0] */
	fpin_set_config(FIOPAD_AA47, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_ce_n[1] */
	fpin_set_config(FIOPAD_U45, FPIN_FUNC0, FPIN_PULL_NONE, FPIN_DRV7); /* nfc_rb_n[1] */


    ret = ftnand_test_init();
    if(ret != 0)
    {
        printf("Eerror ftnand_scan is failed \r\n");
        return ret;
    }

    return ret;
}

/**
 * @name: ftnand_erase_skip_block
 * @msg:  通过检查bbm表中的坏块信息，确定是否擦除特定的块
 * @param undefined
 * @param undefined
 * @return {*}
 * @note:
 */
u32 ftnand_erase_skip_block(u64 page_addr,u32 length,u32 chip_addr)
{
    u32 block_size;
    u32 start_block;
	u32 num_of_blocks;
    u32 block_num;
    u32 ret;

    block_size = ftnand_instance.nand_geometry[chip_addr].block_size;

    if( page_addr &(block_size - 1) ) /* 地址必须符合块对齐 */
    {
        printf(" Address must align on block boundary ");
        return -1 ;
    }

    if( length &(block_size - 1) )   /* 长度必须符合块对齐 */
    {
        printf(" length must align on block boundary ");
        return -2 ;
    }

    start_block = (page_addr & ~(block_size - 1))/block_size;
    num_of_blocks = (length & ~(block_size - 1)) / block_size;

    for (block_num = start_block; block_num < (start_block + num_of_blocks);block_num ++ )
    {

        ret =  ftnand_is_block_bad(&ftnand_instance,block_num,chip_addr);
        if( ret == FT_SUCCESS)
        {
            num_of_blocks++;
            if((start_block  + num_of_blocks) >= ftnand_instance.nand_geometry[chip_addr].num_blocks)
            {
                printf(" block is over max block number ");
                return -3;
            }
            printf("Block[%d] is bad ",block_num);
            continue;
        }

        ret = ftnand_erase_block(&ftnand_instance,block_num,chip_addr);
        if(ret != FT_SUCCESS)
        {
            printf("ftnand_erase_block operation is error ");
            return -4 ;
        }
    }

    return FT_SUCCESS;
}

u32 ftnand_block_write(u64 byte_addr,u32 length,unsigned long buf_p,u32 chip_addr)
{

    u32 page,col;
    u32 partial_bytes;
    u32 page_bytes;
    u32 copy_of_addr;
    u8 *ptr = (u8 *)buf_p;
    u32 ret;

    page = byte_addr / ftnand_instance.nand_geometry[chip_addr].bytes_per_page;
    col = byte_addr &(ftnand_instance.nand_geometry[chip_addr].bytes_per_page - 1);
    partial_bytes =  ftnand_instance.nand_geometry[chip_addr].bytes_per_page -col ; /* 如果 col 非0 ，则写入的部分 并非从页首开始 ，写入的内容小于等于一页 */
    page_bytes = (partial_bytes < length)? partial_bytes:length;
    copy_of_addr = ftnand_instance.nand_geometry[chip_addr].bytes_per_page - partial_bytes;

    while (length)
    {
        /* 调用接口写入 */

        ret = ftnand_write_page(&ftnand_instance,page,ptr,copy_of_addr,page_bytes,NULL,0,0,chip_addr);
        if(ret != FT_SUCCESS)
        {
            printf("%s:ftnand_write_page write is error",__func__);
            return -1;
        }

        ptr += page_bytes;
        length -= page_bytes;
        page++;
        page_bytes = (length > ftnand_instance.nand_geometry[chip_addr].bytes_per_page) ? ftnand_instance.nand_geometry[chip_addr].bytes_per_page : length;
        copy_of_addr = 0;
    }

    return FT_SUCCESS;
}

u32 ftnand_skip_write(u64 byte_addr,u32 length,unsigned long buf_p,u32 chip_addr)
{
    u32 bytes_left = length ;
    u32 block_size = ftnand_instance.nand_geometry[chip_addr].block_size ;
    u32 block_addr ;
    u32 block_write_length ;
    u32 write_length;
    u32 block_num;

    u32 ret;

    while (bytes_left)
    {
        /* 根据块为单位写入参数 ,如果byte_addr没按照block size 对齐，则第一个块写入的内容为不对称部分的内容 */
        block_addr = byte_addr & (block_size - 1);
        block_num = (byte_addr & ~(block_size - 1)) / block_size;
        block_write_length = block_size - block_addr;

        /* 检查此块是否为坏块 */
        ret = ftnand_is_block_bad(&ftnand_instance,block_num,chip_addr) ;
        if(ret == FT_SUCCESS)
        {
            /* move to next block */
            byte_addr += block_write_length ;
            continue ;
        }

        /*  */
        if(bytes_left < block_write_length)
        {
            write_length = bytes_left;
        }
        else
        {
            write_length = block_write_length ;
        }

        /* 按照块为单位调用页写入接口依次写入 */

        ret = ftnand_block_write(byte_addr,write_length,buf_p,chip_addr);
        if (ret != FT_SUCCESS)
        {
            return ret;
        }

        bytes_left -= write_length;
        byte_addr += write_length;
        buf_p += write_length;
    }

    return FT_SUCCESS;
}

u32 ftnand_block_read(u64 byte_addr,u32 length,unsigned long buf_p,u32 chip_addr)
{

    u32 page,col;
    u32 partial_bytes;
    u32 page_bytes;
    u32 copy_of_addr;
    u8 *ptr = (u8 *)buf_p;
    u32 bytes_per_page;
    u32 ret;

    bytes_per_page = ftnand_instance.nand_geometry[chip_addr].bytes_per_page;
    page = byte_addr / bytes_per_page;
    col = byte_addr &(bytes_per_page - 1);
    partial_bytes =  bytes_per_page -col ; /* 如果 col 非0 ，则写入的部分 并非从页首开始 ，写入的内容小于等于一页 */
    page_bytes = (partial_bytes < length)? partial_bytes:length;
    copy_of_addr = bytes_per_page - partial_bytes;

    while (length)
    {
        /* 调用接口读取 */

        ret = ftnand_read_page(&ftnand_instance,page,ptr,copy_of_addr,page_bytes,NULL,0,0,chip_addr);
        if(ret != FT_SUCCESS)
        {
            printf("%s:ftnand_write_page write is error",__func__);
            return -1;
        }

        ptr += page_bytes;
        length -= page_bytes;
        page++;
        page_bytes = (length > bytes_per_page) ? bytes_per_page : length;
        copy_of_addr = 0;
    }

    return FT_SUCCESS;
}

u32 ftnand_skip_read(u64 byte_addr,u32 length,unsigned long buf_p,u32 chip_addr)
{
    u32 bytes_left = length ;
    u32 block_size = ftnand_instance.nand_geometry[chip_addr].block_size ;
    u32 block_addr ;
    u32 block_read_length ;
    u32 read_length;
    u32 block_num;
    u32 ret;

    while (bytes_left)
    {
        /* 根据块为单位写入参数 ,如果byte_addr没按照block size 对齐，则第一个块写入的内容为不对称部分的内容 */
        block_addr = byte_addr & (block_size - 1);
        block_num = (byte_addr & ~(block_size - 1)) / block_size;
        block_read_length = block_size - block_addr;

        /* 检查此块是否为坏块 */
        ret = ftnand_is_block_bad(&ftnand_instance,block_num,chip_addr) ;
        if(ret == FT_SUCCESS)
        {
            /* move to next block */
            byte_addr += block_read_length ;
            continue ;
        }

        /*  */
        if(bytes_left < block_read_length)
        {
            read_length = bytes_left;
        }
        else
        {
            read_length = block_read_length ;
        }

        /* 按照块为单位调用页写入接口依次读取 */

        ret = ftnand_block_read(byte_addr,read_length,buf_p,chip_addr);
        if (ret != FT_SUCCESS)
        {
            return ret;
        }

        bytes_left -= read_length;
        byte_addr += read_length;
        buf_p += read_length;
    }

    return FT_SUCCESS;
}


u32 ftnand_skip_example(int argc, char * const argv[])
{
    u32 ret;
    int i;
    printf("Nand Flash skip block example test \r\n");


    /* step1 initalize the nand controller */
    ret = ftnand_cfg_initialize(&ftnand_instance, ftnand_lookup_config(FNAND_INSTANCE0));
    if(ret != FT_SUCCESS)
    {
        printf("%s ,ftnand_cfg_initialize is error",__func__);
        return -1;
    }

    /* step2 init nand flash */
    printf("step3 nandflash init  \r\n");
    ret = ftnand_scan(&ftnand_instance);
    if(ret != FT_SUCCESS)
    {
        printf("%s ,ftnand_scan is error %x",__func__,ret);
        return -2;
    }

    ftnand_init_bbt_desc(&ftnand_instance);
    ftnand_scan_bbt(&ftnand_instance,0); /* init bbm table */


    /* step3 start to erase block */
    printf("start to erase block \r\n");
    ret = ftnand_erase_skip_block(NAND_TEST_OFFSET,NAND_TEST_LENGTH,0);
    if(ret != FT_SUCCESS)
    {
        printf("%s ,ftnand_erase_skip_block is error",__func__);
        return -3 ;
    }

    /* step4 write the block */
    printf("start to write block \r\n");
    for (i = 0; i < NAND_TEST_LENGTH; i++)
    {
        write_buffer[i] = rand()% 0xff;
    }
    ret = ftnand_skip_write(NAND_TEST_OFFSET,NAND_TEST_LENGTH,(unsigned long)write_buffer, 0);
    if(ret != FT_SUCCESS)
    {
        printf("%s ,ftnand_skip_write is error",__func__);
        return -4 ;
    }

    /* step5 read the block */
    printf("start to read block \r\n");
    for (i = 0; i < NAND_TEST_LENGTH; i++)
    {
        read_buffer[i] = 0;
    }

    ret = ftnand_skip_read(NAND_TEST_OFFSET,NAND_TEST_LENGTH,(unsigned long)read_buffer, 0);
    if(ret != FT_SUCCESS)
    {
        printf("%s ,ftnand_skip_read is error",__func__);
        return -5 ;
    }

	/* step6 Compare the data read against the data Written. */
	for (i = 0; i < NAND_TEST_LENGTH; i++)
    {
		if (read_buffer[i] != write_buffer[i])
        {
        	printf("readbuf[%d]:0x%x, does not match the written value : 0x%x\n",i,read_buffer[i],write_buffer[i]);
            printf("%s ,ftnand_skip_read is error",__func__);
			return -6;
		}
	}

    printf("\r\n%s ,ftnand_skip_example is ok\n",__func__);


    return FT_SUCCESS;
}

static int ftnand_erase_test(int argc, char *const argv[])
{
    u32 block = 0 ;
    u32 ret;
    switch (argc)
    {
        case 3:
            block = simple_strtoul(argv[2], NULL, 16);
            printf("erase block is %d \r\n",block);
        case 2:
            ret = ftnand_erase_block(&ftnand_instance,block,0);

            if(ret != FT_SUCCESS )
            {
            	printf("ftnand erase fail\n");
                return -3 ;
            }

            break;
        default:
			printf("usage:    ftnand erase <block>\r\n");
            break ;
    }

	printf("ftnand erase is ok\n");
    return  0;
}

static u32 ftnand_read_hw_ecc_test(int argc, char * const argv[])
{
    u32 page = 0 ;
    u64 memaddr=0;
	u32 length_mem=0;
	u64 oobaddr=0;
	u32 length_oob=0;
    u32 ret;
    switch (argc)
    {
        case 7:
			page = simple_strtoul(argv[2], NULL, 16);
			printf("read_hw_ecc page is 0x%x \n",page);
			memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
			oobaddr=simple_strtoul(argv[5], NULL, 16);
			printf("oobaddr is 0x%llx \n",oobaddr);
			length_oob=simple_strtoul(argv[6], NULL, 16);
			printf("ooblength is 0x%x \n",length_oob);
			if((length_oob<1)||(length_oob>64))
			{
				printf("The length exceeds the range [1,64]\n");
				return -2;
			}
			ret = ftnand_read_page(&ftnand_instance,page,(u8 *)memaddr,0,length_mem,(u8 *)oobaddr,0,length_oob,0);

			if(ret != FT_SUCCESS)
			{
				printf("read data fail \r\n");
				return -3;
			}

            printf("read data is ok \r\n");
            //ft_dump_hex_word((const u32 *)memaddr,length_mem);
            printf("read oob data is ok \r\n");
            //ft_dump_hex_word((const u32 *)oobaddr,length_oob);

            break;
        default:
			printf("usage:    ftnand r_hw_ecc page memaddr length_mem oobaddr length_obb\r\n");
            break ;
    }

    return  0;
}

static u32 ftnand_read_hw_ecc_test_only(int argc, char *const argv[])
{
    u32 page = 0 ;
    u32 ret;
	u64 memaddr=0;
	u32 length_mem=0;
    switch (argc)
    {
        case 5:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("read_only_hw_ecc page is 0x%x \n",page);
        	memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
            ret = ftnand_read_page(&ftnand_instance,page,(u8 *)memaddr,0,length_mem,NULL,0,0,0);

            if(ret != FT_SUCCESS)
            {
            	printf("read data fail \r\n");
                return -2 ;
            }

            printf("read data is ok \r\n");
            //ft_dump_hex_word((const u32 *)memaddr,length_mem);
            break;
        default:
			printf("usage:    ftnand r_o_hw_ecc page memaddr length_mem\r\n");
            break ;
    }

    return  0;
}

static u32 ftnand_read_raw_test_only(int argc, char *const argv[])
{
    u32 page = 0 ;
    u32 ret;
	u64 memaddr=0;
	u32 length_mem=0;
    switch (argc)
    {
        case 5:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("read_raw page is 0x%x \n",page);
       		memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
            ret = ftnand_read_page_raw(&ftnand_instance,page,(u8 *)memaddr,0,length_mem,NULL,0,0,0);

            if(ret != FT_SUCCESS)
            {
            	printf("read data fail \r\n");
                return -2 ;
            }

            printf("read data is ok \r\n");
            //ft_dump_hex_word((const u32 *)memaddr,length_mem);
            break;
        default:
			printf("usage:    ftnand r_raw page memaddr length_mem\r\n");
            break ;
    }

    return  0;
}

static int ftnand_write_hw_ecc_test(int argc, char *const argv[])
{
    u32 page = 0 ;
    u32 ret;
    u64 memaddr=0;
	u32 length_mem=0;
	u64 oobaddr=0;
	u32 length_oob=0;

    switch (argc)
    {
        case 7:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("write_hw_ecc page is 0x%x \n",page);
      		memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
			oobaddr=simple_strtoul(argv[5], NULL, 16);
			printf("oobaddr is 0x%llx \n",oobaddr);
			length_oob=simple_strtoul(argv[6], NULL, 16);
			printf("ooblength is 0x%x \n",length_oob);
			if((length_oob<1)||(length_oob>64))
			{
				printf("The length exceeds the range [1,64]\n");
				return -2;
			}

            ret = ftnand_write_page(&ftnand_instance,page,(u8 *)memaddr,0,length_mem,(u8 *)oobaddr,0,length_oob,0);

            if(ret != FT_SUCCESS)
            {
            	printf("write data fail\n");
                return -3;
            }
			printf("write data is ok\n");
            break;
        default:
			printf("usage:    ftnand w_hw_ecc page memaddr length_mem oobaddr length_obb\r\n");
            break ;
    }

    return  0;
}

static int ftnand_write_hw_ecc_test_only(int argc, char *const argv[])
{
    u32 page = 0 ;
    u32 ret;
    u64 memaddr=0;
	u32 length_mem=0;

    switch (argc)
    {
        case 5:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("write_only_hw_ecc page is 0x%x \n",page);
       		memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
            ret = ftnand_write_page(&ftnand_instance,page,(u8*)memaddr,0,length_mem,NULL,0,0,0);

            if(ret != FT_SUCCESS)
            {
            	printf("write data fail\n");
                return -2;
            }
			printf("write data is ok\n");
            break;
        default:
			printf("usage:    ftnand w_o_hw_ecc page memaddr length_mem\r\n");
            break ;
    }

    return  0;
}

static u32 ftnand_write_raw_test_only(int argc, char *const argv[])
{
	u32 page = 0 ;
	u32 ret;
	u64 memaddr=0;
	u32 length_mem=0;

	switch (argc)
	{
		case 5:
			page = simple_strtoul(argv[2], NULL, 16);
			printf("write_raw page is 0x%x \n",page);
			memaddr=simple_strtoul(argv[3], NULL, 16);
			printf("memaddr is 0x%llx \n",memaddr);
			length_mem=simple_strtoul(argv[4], NULL, 16);
			printf("memlength is 0x%x \n",length_mem);
			if((length_mem<1)||(length_mem>2048))
			{
				printf("The length exceeds the range [1,2048]\n");
				return -1;
			}
			ret = ftnand_write_page_raw(&ftnand_instance,page,(u8 *)memaddr,0,length_mem,NULL,0,0,0);

			if(ret != FT_SUCCESS)
			{
				printf("write data fail\n");
				return -2;
			}
			printf("write data is ok\n");
			break;
		default:
			printf("usage:    ftnand w_raw page memaddr length_mem\r\n");
			break ;
	}

	return	0;

}

u32 ftnand_read(int argc, char * const argv[])
{
	u32 page = 0;
	u32 ret;
	u64 addr=0;
	u64 length=0;
	u64 chip_len=0;

    if(argc==5)
    {
		page = simple_strtoul(argv[2], NULL, 16);
		printf("read start page is 0x%x \n",page);

		addr=simple_strtoul(argv[3], NULL, 16);
		printf("memaddr is %lld \n",addr);

		length=simple_strtoul(argv[4], NULL, 16);
		printf("memlength is %lld \n",length);

		chip_len=ftnand_instance.nand_geometry[0].block_size*ftnand_instance.nand_geometry[0].blocks_per_lun*ftnand_instance.nand_geometry[0].num_lun;
		if((length<1)||(length>chip_len))
		{
			printf("The length exceeds the range [1,0x%llx]\n",chip_len);
			return -1;
		}
		
		printf("start to read block\n");
    	ret = ftnand_skip_read(page,length,(unsigned long)addr, 0);
    	if(ret != FT_SUCCESS)
    	{
        	printf("%s ,ftnand_skip_read is error\n",__func__);
        	return -1;
    	}

    	printf("\r\n%s , is ok\n",__func__);
    }
	else
	{
		printf("usage:    ftnand read page memaddr length_mem \r\n");
	}

    return FT_SUCCESS;
}

u32 ftnand_write(int argc, char * const argv[])
{
	u32 page = 0;
	u32 ret;
	u64 addr=0;
	u64 length=0;
	u64 chip_len=0;

    if(argc==5)
    {

		page = simple_strtoul(argv[2], NULL, 16);
		printf("write start page is 0x%x \n",page);

		addr=simple_strtoul(argv[3], NULL, 16);
		printf("memaddr is %lld \n",addr);

		length=simple_strtoul(argv[4], NULL, 16);
		printf("memlength is %lld \n",length);

		chip_len=ftnand_instance.nand_geometry[0].block_size*ftnand_instance.nand_geometry[0].blocks_per_lun*ftnand_instance.nand_geometry[0].num_lun;
		if((length<1)||(length>chip_len))
		{
			printf("The length exceeds the range [1,0x%llx]\n",chip_len);
			return -1;
		}

    	printf("start to write block\n");
    	ret = ftnand_skip_write(page,length,(unsigned long)addr, 0);
    	if(ret != FT_SUCCESS)
    	{
        	printf("%s ,ftnand_skip_write is error\n",__func__);
        	return -1 ;
    	}

    	printf("\r\n%s , is ok\n",__func__);
    }
	else
	{
		printf("usage:    ftnand write page memaddr length_mem \r\n");
	}

    return FT_SUCCESS;
}

u32 ftnand_image(int argc, char * const argv[])
{
	u32 page = 0;
	u32 ret;
    int i;
	u64 addr=0;
	u32 length=0;

    if(argc==5)
    {
    	printf("image start \r\n");

		page = simple_strtoul(argv[2], NULL, 16);
		printf("image page is 0x%x \n",page);

		addr=simple_strtoul(argv[3], NULL, 16);
		printf("image addr is %lld \n",addr);

		length=simple_strtoul(argv[4], NULL, 16);
		printf("image length is %d \n",length);
		if((length<1)||(length>NAND_IMAGE_LENGTH))
		{
			printf("The length exceeds the range [1,0x%x]\n",NAND_IMAGE_LENGTH);
			return -1;
		}

    	printf("start to erase block\n");

    	ret = ftnand_erase_skip_block(page,length,0);
    	if(ret != FT_SUCCESS)
    	{
        	printf("%s ,ftnand_erase_skip_block is error\n",__func__);
        	return -1 ;
    	}

    	printf("start to write block\n");
    	ret = ftnand_skip_write(page,length,(unsigned long)addr, 0);
    	if(ret != FT_SUCCESS)
    	{
        	printf("%s ,ftnand_skip_write is error\n",__func__);
        	return -2 ;
    	}

    	printf("start to read block\n");
    	for (i = 0; i < length; i++)
    	{
        	read_image_buffer[i] = 0;
    	}

    	ret = ftnand_skip_read(page,length,(unsigned long)read_image_buffer, 0);
    	if(ret != FT_SUCCESS)
    	{
        	printf("%s ,ftnand_skip_read is error\n",__func__);
        	return -3 ;
    	}


		for (i = 0; i < length; i++)
    	{
			if (read_image_buffer[i] != (*((u8 *)addr+i)))
        	{
        		printf("readbuf[%d]:0x%x, does not match the written value : 0x%x\n",i,read_image_buffer[i],(*((u8 *)addr+i)));
            	printf("%s ,ftnand_skip_read is error\n",__func__);
				return -4;
			}
		}



    	printf("\r\n%s , is ok\n",__func__);
    }
	else
	{
		printf("usage:    ftnand image page memaddr length_mem \r\n");
	}


    return FT_SUCCESS;
}

static int ftnand_read_oob_test(int argc, char * const argv[])
{
    u32 page = 0;
    u32 ret;
	u64 oobaddr=0;
	u32 length_oob=0;
    switch (argc)
    {
        case 5:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("read_oob page is 0x%x \n",page);
        	oobaddr=simple_strtoul(argv[3], NULL, 16);
			printf("oobaddr is 0x%llx \n",oobaddr);
			length_oob=simple_strtoul(argv[4], NULL, 16);
			printf("ooblength is 0x%x \n",length_oob);
			if((length_oob<1)||(length_oob>64))
			{
				printf("The length exceeds the range [1,64]\n");
				return -1;
			}
            ret = ftnand_read_page_oob(&ftnand_instance,page,(u8 *)oobaddr,0,length_oob,0);

            if(ret != FT_SUCCESS)
            {
                return -2;
            }

            printf("read oob data is ok \r\n");
            //ft_dump_hex_word((const u32 *)oobaddr,length_oob);

        break ;
        default:
			printf("usage:    ftnand r_oob page oobaddr length_obb\r\n");
            break ;
    }

    return  0;
}

static u32 ftnand_write_oob_test(int argc, char *const argv[])
{
    u32 page = 0 ;
    u32 ret;
	u64 oobaddr=0;
	u32 length_oob=0;

    switch (argc)
    {
        case 5:
            page = simple_strtoul(argv[2], NULL, 16);
            printf("write_oob page is 0x%x \n",page);
        	oobaddr=simple_strtoul(argv[3], NULL, 16);
			printf("oobaddr is 0x%llx \n",oobaddr);
			length_oob=simple_strtoul(argv[4], NULL, 16);
			printf("ooblength is 0x%x \n",length_oob);
			if((length_oob<1)||(length_oob>64))
			{
				printf("The length exceeds the range [1,64]\n");
				return -1;
			}
            ret = ftnand_write_page_oob(&ftnand_instance,page,(u8 *)oobaddr,0,length_oob,0);
            if(ret != FT_SUCCESS)
            {
                return -2;
            }
            break;
        default:
			printf("usage:    ftnand w_oob page oobaddr length_obb\r\n");
            break ;
    }

    return  0;
}

static int ftnand_erase_all_test(int argc, char * const argv[])
{
    u32 block = 0 ;
    u32 ret;
    for (block = 0; block < ftnand_instance.nand_geometry[0].blocks_per_lun - 0x10;block ++)
    {
        ret = ftnand_erase_block(&ftnand_instance,block,0);
		if(ret != FT_SUCCESS)
        {
            printf("erase block %d is error \r\n",block);
        }
    }
	printf("erase block is ok \r\n");

    return  0;
}

static void ftnand_usage(void)
{
    printf("usage:\r\n");
    printf("    ftnand init\r\n");
    printf("        -- init fnand driver\r\n");
    printf("    ftnand erase <block>\r\n");
    printf("        -- erase page ,page num is Page plus block address \r\n");
    printf("    ftnand r_oob page oobaddr length_obb\r\n");
    printf("        -- read page oob space,page num is Page plus block address \r\n");
    printf("    ftnand w_oob page oobaddr length_obb\r\n");
    printf("        -- write page oob space,page num is Page plus block address \r\n");
    printf("    ftnand r_hw_ecc page memaddr length_mem oobaddr length_obb\r\n");
    printf("        -- read page with oob ,page num is Page plus block address \r\n");
    printf("    ftnand w_hw_ecc page memaddr length_mem oobaddr length_obb\r\n");
    printf("        -- write page with oob ,page num is Page plus block address \r\n");
    printf("    ftnand w_o_hw_ecc page memaddr length_mem\r\n");
    printf("        -- write page with ecc ,page num is Page plus block address \r\n");
    printf("    ftnand r_o_hw_ecc page memaddr length_mem\r\n");
    printf("        -- read page with ecc ,page num is Page plus block address \r\n");
    printf("    ftnand w_raw page memaddr length_mem\r\n");
    printf("        -- write page without ecc ,page num is Page plus block address \r\n");
    printf("    ftnand r_raw page memaddr length_mem\r\n");
    printf("        -- read page without ecc ,page num is Page plus block address \r\n");
    printf("    ftnand skip \r\n");
    printf("        -- start skip test \r\n");
    printf("    ftnand eraseall \r\n");
    printf("        -- start erase all blocks test \r\n");
    printf("    ftnand image page memaddr length_mem \r\n");
    printf("        -- start loading image from memory \r\n");
	printf("    ftnand write page memaddr length_mem \r\n");
    printf("        -- write data from memory \r\n");
	printf("    ftnand read page memaddr length_mem \r\n");
    printf("        -- read data to memory \r\n");

}

static int ftnand_test_cmd_entry(struct cmd_tbl *cmdtp, int flag, int argc,char *const argv[])
{
    static int init_flg = 0 ;
    int ret = 0 ;

	if (argc < 2) {
        ftnand_usage();
		return CMD_RET_USAGE;
	}

	if (!strcmp(argv[1], "init"))
    {
        ret = ftnand_test_init_entry();
        init_flg = 1;
    }
	else if(init_flg == 1)
	{
		if(!strcmp(argv[1], "skip"))
    	{
        	ret = ftnand_skip_example(argc,argv);
    	}
		else if(!strcmp(argv[1], "erase"))
        {
            ret = ftnand_erase_test(argc,argv);
        }
		else if(!strcmp(argv[1], "r_oob"))
        {
            ret = ftnand_read_oob_test(argc,argv);
        }
        else if(!strcmp(argv[1], "w_oob"))
        {
            ret = ftnand_write_oob_test(argc,argv);
        }
		else if(!strcmp(argv[1], "r_hw_ecc"))
        {
            ret = ftnand_read_hw_ecc_test(argc,argv);
        }
        else if(!strcmp(argv[1], "r_o_hw_ecc"))
        {
            ret = ftnand_read_hw_ecc_test_only(argc,argv);
        }
        else if(!strcmp(argv[1], "w_hw_ecc"))
        {
            ret = ftnand_write_hw_ecc_test(argc,argv);
        }
        else if(!strcmp(argv[1], "w_o_hw_ecc"))
        {
            ret = ftnand_write_hw_ecc_test_only(argc,argv);
        }
		else if(!strcmp(argv[1], "r_raw"))
        {
            ret = ftnand_read_raw_test_only(argc,argv);
        }
		else if(!strcmp(argv[1], "w_raw"))
        {
            ret = ftnand_write_raw_test_only(argc,argv);
        }
		else if (!strcmp(argv[1], "eraseall"))
        {
            ret = ftnand_erase_all_test(argc, argv);
        }
		else if(!strcmp(argv[1], "image"))
		{
			ret =ftnand_image(argc,argv);
		}
		else if(!strcmp(argv[1], "read"))
		{
			ret =ftnand_read(argc,argv);
		}
		else if(!strcmp(argv[1], "write"))
		{
			ret =ftnand_write(argc,argv);
		}
		else
		{
			printf("ftnand cmd error\n");
			ftnand_usage();
			return CMD_RET_USAGE;

		}
	}
    else
    {
        if(init_flg  == 0)
        {
            printf("please enter ftnand init ,initialize nand controler");
        }

        return -1;
    }


    return 0;
}


U_BOOT_CMD(
	ftnand,	7,	1,	ftnand_test_cmd_entry,
	"init ftnand driver",
	""
);


