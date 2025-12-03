// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium Nand flash driver
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#ifndef _FT_NAND_H
#define _FT_NAND_H

/* 系统错误码模块定义 */
typedef enum
{
    ErrorModGeneral = 0,
    ErrModBsp,
    ErrModAssert,
    ErrModPort,
    StatusModBsp,
    ErrModMaxMask = 255,

} ft_error_code_module_mask;

/* BSP模块的错误子模块定义 */
typedef enum
{
    ErrBspGeneral = 0,
    ErrBspClk,
    ErrBspRtc,
    ErrBspTimer,
    ErrBspUart,
    ErrBspGpio,
    ErrBspSpi,
    ErrBspEth,
    ErrCan,
    ErrPcie,
    ErrBspQSpi,
    ErrBspMio,
    ErrBspI2c,
    ErrBspMmc,
    ErrBspWdt,
    ErrGic,
    ErrGdma,
    ErrNand,
    ErrIoMux,
    ErrBspSata,
    ErrUsb,
	ErrEthPhy,
	ErrDdma,
	ErrBspAdc,
    ErrBspPwm,	
    ErrSema,
	
    ErrBspModMaxMask = 255
} ft_err_code_bsp_mask;
	
#undef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
		
#define _INLINE inline
#define ASSERT(c) assert(c)
	
#define FT_ERRCODE_SYS_MODULE_OFFSET (u32)24
#define FT_ERRCODE_SUB_MODULE_OFFSET (u32)16
					
#define FT_ERRCODE_SYS_MODULE_MASK ((u32)0xff << FT_ERRCODE_SYS_MODULE_OFFSET) /* bit 24 .. 31 */
#define FT_ERRCODE_SUB_MODULE_MASK ((u32)0xff << FT_ERRCODE_SUB_MODULE_OFFSET) /* bit 16 .. 23 */
#define FT_ERRCODE_TAIL_VALUE_MASK ((u32)0xffff)                               /* bit  1 .. 15 */

#define FT_SUCCESS 0

					/* Offset error code */
#define FT_ERRCODE_OFFSET(code, offset, mask) \
						(((code) << (offset)) & (mask))
					
					/* Assembly error code */
#define FT_MAKE_ERRCODE(sys_mode, sub_mode, tail)                                                   \
						((FT_ERRCODE_OFFSET((u32)sys_mode, FT_ERRCODE_SYS_MODULE_OFFSET, FT_ERRCODE_SYS_MODULE_MASK)) | \
						 (FT_ERRCODE_OFFSET((u32)sub_mode, FT_ERRCODE_SUB_MODULE_OFFSET, FT_ERRCODE_SUB_MODULE_MASK)) | \
						 ((u32)tail & FT_ERRCODE_TAIL_VALUE_MASK))
#define FT_CODE_ERR FT_MAKE_ERRCODE
					
#define ERR_SUCCESS FT_MAKE_ERRCODE(ErrorModGeneral, ErrBspGeneral, 0) /* 成功 */
#define ERR_GENERAL FT_MAKE_ERRCODE(ErrorModGeneral, ErrBspGeneral, 1) /* 一般错误 */

#define FNAND_ERR_OPERATION FT_CODE_ERR(ErrModBsp, ErrNand, 0x1u)
#define FNAND_ERR_INVAILD_PARAMETER FT_CODE_ERR(ErrModBsp, ErrNand, 0x2u)
#define FNAND_IS_BUSY FT_CODE_ERR(ErrModBsp, ErrNand, 0x3u)
#define FNAND_OP_TIMEOUT FT_CODE_ERR(ErrModBsp, ErrNand, 0x4u)
#define FNAND_VALUE_ERROR FT_CODE_ERR(ErrModBsp, ErrNand, 0x7u)
#define FNAND_VALUE_FAILURE FT_CODE_ERR(ErrModBsp, ErrNand, 0x8u)
#define FNAND_NOT_FET_TOGGLE_MODE FT_CODE_ERR(ErrModBsp, ErrNand, 0xCu)
#define FNAND_ERR_READ_ECC  FT_CODE_ERR(ErrModBsp, ErrNand, 0xBu)
#define FNAND_ERR_IRQ_LACK_OF_CALLBACK  FT_CODE_ERR(ErrModBsp, ErrNand, 0xCu)
#define FNAND_ERR_IRQ_OP_FAILED  FT_CODE_ERR(ErrModBsp, ErrNand, 0xdu)
#define FNAND_ERR_NOT_MATCH  FT_CODE_ERR(ErrModBsp, ErrNand, 0xEu)

#define FT_COMPONENT_IS_READY 0x11111111U
#define FT_COMPONENT_IS_STARTED 0x22222222U


#define FNAND_MAX_BLOCKS 32768    /* Max number of Blocks */
#define FNAND_MAX_PAGE_SIZE 16384 /* Max page size of NAND \
                    flash */
#define FNAND_MAX_SPARE_SIZE 1024  /* Max spare bytes of a NAND \
                    flash page */


/* dma */
#define FNAND_DMA_MAX_LENGTH (32*1024)

/* options */
/* These constants are used as option to ftnand_set_option()  */
#define FNAND_OPS_INTER_MODE_SELECT 1U      /*  */


/* These constants are used as parameters to ftnand_set_isr_handler() */
#define FNAND_WORK_MODE_POLL 0U
#define FNAND_WORK_MODE_ISR  1U


/* NAND Flash Interface */

#define FNAND_ONFI_MODE     0U
#define FNAND_TOGGLE_MODE   1U

/* NAND */
#define FNAND_NUM 1U
#define FNAND_INSTANCE0 0U
#define FNAND_BASEADDRESS 0x28002000U
#define FNAND_IRQ_NUM (106U)
#define FNAND_CONNECT_MAX_NUM 1U

#define FIOPAD_BASE_ADDR            0x32B30000U

typedef enum
{
	FT_LOG_NONE,   /* No log output */
	FT_LOG_ERROR,  /* Critical errors, software module can not recover on its own */
	FT_LOG_WARN,   /* Error conditions from which recovery measures have been taken */
	FT_LOG_INFO,   /* Information messages which describe normal flow of events */
	FT_LOG_DEBUG,  /* Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
	FT_LOG_VERBOSE /* Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
} ft_log_level_t;
	
#define LOG_COLOR_BLACK "30"
#define LOG_COLOR_RED "31"
#define LOG_COLOR_GREEN "32"
#define LOG_COLOR_BROWN "33"
#define LOG_COLOR_BLUE "34"
#define LOG_COLOR_PURPLE "35"
#define LOG_COLOR_CYAN "36"
#define LOG_COLOR(COLOR) "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR) "\033[1;" COLOR "m"
#define LOG_RESET_COLOR "\033[0m"
#define LOG_COLOR_E LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I LOG_COLOR(LOG_COLOR_GREEN)
#define LOG_COLOR_D LOG_COLOR(LOG_COLOR_CYAN)
#define LOG_COLOR_V LOG_COLOR(LOG_COLOR_PURPLE)

//#define CONFIG_LOG_VERBOS
//#define CONFIG_LOG_ERROR
//#define CONFIG_LOG_WARN
//#define CONFIG_LOG_INFO
#define CONFIG_LOG_DEBUG

/* select debug log level */
#ifdef CONFIG_LOG_VERBOS
#define LOG_LOCAL_LEVEL FT_LOG_VERBOSE
#endif
	
#ifdef CONFIG_LOG_ERROR
#define LOG_LOCAL_LEVEL FT_LOG_ERROR
#endif
	
#ifdef CONFIG_LOG_WARN
#define LOG_LOCAL_LEVEL FT_LOG_WARN
#endif
	
#ifdef CONFIG_LOG_INFO
#define LOG_LOCAL_LEVEL FT_LOG_INFO
#endif
	
#ifdef CONFIG_LOG_DEBUG
#define LOG_LOCAL_LEVEL FT_LOG_DEBUG
#endif
	
#define LOG_FORMAT(letter, format) LOG_COLOR_##letter " %s: " format LOG_RESET_COLOR "\r\n"


#ifndef CONFIG_LOG_EXTRA_INFO
#define LOG_EARLY_IMPL(tag, format, log_level, log_tag_letter, ...)           \
    do                                                                        \
    {                                                                         \
        if (LOG_LOCAL_LEVEL < log_level)                                      \
            break;                                                            \
        printf(LOG_FORMAT(log_tag_letter, format), tag, ##__VA_ARGS__); \
    } while (0)
#else
#include <string.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
/* print debug information with source file name and source code line num. */
#define LOG_EARLY_IMPL(tag, format, log_level, log_tag_letter, ...)           \
    do                                                                        \
    {                                                                         \
        if (LOG_LOCAL_LEVEL < log_level)                                      \
            break;                                                            \
        printf(LOG_FORMAT(log_tag_letter, format" @%s:%d"), tag, ##__VA_ARGS__, __FILENAME__, __LINE__); \
    } while (0)
#endif

#define EARLY_LOGE(tag, format, ...) LOG_EARLY_IMPL(tag, format, FT_LOG_ERROR, E, ##__VA_ARGS__)
#define EARLY_LOGI(tag, format, ...) LOG_EARLY_IMPL(tag, format, FT_LOG_INFO, I, ##__VA_ARGS__)
#define EARLY_LOGD(tag, format, ...) LOG_EARLY_IMPL(tag, format, FT_LOG_DEBUG, D, ##__VA_ARGS__)
#define EARLY_LOGW(tag, format, ...) LOG_EARLY_IMPL(tag, format, FT_LOG_WARN, W, ##__VA_ARGS__)
#define EARLY_LOGV(tag, format, ...) LOG_EARLY_IMPL(tag, format, FT_LOG_VERBOSE, W, ##__VA_ARGS__)

/* do not compile log if define CONFIG_LOG_NONE */
#ifndef CONFIG_LOG_NONE
#define FT_DEBUG_PRINT_I(TAG, format, ...) EARLY_LOGI(TAG, format, ##__VA_ARGS__)
#define FT_DEBUG_PRINT_E(TAG, format, ...) EARLY_LOGE(TAG, format, ##__VA_ARGS__)
#define FT_DEBUG_PRINT_D(TAG, format, ...) EARLY_LOGD(TAG, format, ##__VA_ARGS__)
#define FT_DEBUG_PRINT_W(TAG, format, ...) EARLY_LOGW(TAG, format, ##__VA_ARGS__)
#define FT_DEBUG_PRINT_V(TAG, format, ...) EARLY_LOGV(TAG, format, ##__VA_ARGS__)
#else
#define FT_DEBUG_PRINT_I(TAG, format, ...)
#define FT_DEBUG_PRINT_E(TAG, format, ...)
#define FT_DEBUG_PRINT_D(TAG, format, ...)
#define FT_DEBUG_PRINT_W(TAG, format, ...)
#define FT_DEBUG_PRINT_V(TAG, format, ...)
#endif

#define CONFIG_FNAND_ID_DEBUG_EN
#define FNAND_ID_DEBUG_TAG "FNAND_ID"
#ifdef CONFIG_FNAND_ID_DEBUG_EN

#define FNAND_ID_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_ID_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ID_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_ID_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ID_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_ID_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ID_DEBUG_D(format, ...) FT_DEBUG_PRINT_D(FNAND_ID_DEBUG_TAG, format, ##__VA_ARGS__)
#else
#define FNAND_ID_DEBUG_I(format, ...)
#define FNAND_ID_DEBUG_W(format, ...)
#define FNAND_ID_DEBUG_E(format, ...)
#define FNAND_ID_DEBUG_D(format, ...)
#endif

#ifdef CONFIG_FNAND_DMA_DEBUG_EN
#define FNAND_DMA_DEBUG_TAG "FNAND_DMA"
#define FNAND_DMA_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_DMA_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_DMA_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_DMA_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_DMA_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_DMA_DEBUG_TAG, format, ##__VA_ARGS__)
#else
#define FNAND_DMA_DEBUG_I(format, ...) 
#define FNAND_DMA_DEBUG_W(format, ...) 
#define FNAND_DMA_DEBUG_E(format, ...) 
#endif
#define FNAND_COMMON_DEBUG_TAG "FNAND_COMMON"
//#define CONFIG_FNAND_COMMON_DEBUG_EN
#ifdef CONFIG_FNAND_COMMON_DEBUG_EN
#define FNAND_COMMON_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_COMMON_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_COMMON_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_COMMON_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_COMMON_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_COMMON_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_COMMON_DEBUG_D(format, ...) FT_DEBUG_PRINT_D(FNAND_COMMON_DEBUG_TAG, format, ##__VA_ARGS__)
#else
#define FNAND_COMMON_DEBUG_I(format, ...)
#define FNAND_COMMON_DEBUG_W(format, ...)
#define FNAND_COMMON_DEBUG_E(format, ...)
#define FNAND_COMMON_DEBUG_D(format, ...)
#endif
#define CONFIG_FNAND_TOGGLE_DEBUG_EN
#define FNAND_TOGGLE_DEBUG_TAG "FNAND_TOGGLE"
#ifdef CONFIG_FNAND_TOGGLE_DEBUG_EN

#define FNAND_TOGGLE_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_TOGGLE_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_TOGGLE_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_TOGGLE_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_TOGGLE_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_TOGGLE_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_TOGGLE_DEBUG_D(format, ...) FT_DEBUG_PRINT_D(FNAND_TOGGLE_DEBUG_TAG, format, ##__VA_ARGS__)
#else
#define FNAND_TOGGLE_DEBUG_I(format, ...)
#define FNAND_TOGGLE_DEBUG_W(format, ...)
#define FNAND_TOGGLE_DEBUG_E(format, ...)
#define FNAND_TOGGLE_DEBUG_D(format, ...)
#endif
#define FNAND_ONFI_DEBUG_TAG "FNAND_ONFI"

//#define CONFIG_FNAND_ONFI_DEBUG_EN
#ifdef CONFIG_FNAND_ONFI_DEBUG_EN

#define FNAND_ONFI_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_ONFI_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ONFI_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_ONFI_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ONFI_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_ONFI_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ONFI_DEBUG_D(format, ...) FT_DEBUG_PRINT_D(FNAND_ONFI_DEBUG_TAG, format, ##__VA_ARGS__)
#else
#define FNAND_ONFI_DEBUG_I(format, ...)
#define FNAND_ONFI_DEBUG_W(format, ...)
#define FNAND_ONFI_DEBUG_E(format, ...)
#define FNAND_ONFI_DEBUG_D(format, ...)
#endif

#define FNAND_DEBUG_TAG "FNAND"
#define FNAND_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_DEBUG_TAG, format, ##__VA_ARGS__)

#define FNAND_INTR_DEBUG_TAG "FNAND_INTR"
#define FNAND_INTR_ERROR(format, ...) FT_DEBUG_PRINT_E(FNAND_INTR_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_INTR_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_INTR_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_INTR_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_INTR_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_INTR_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_INTR_DEBUG_TAG, format, ##__VA_ARGS__)

#define FNAND_ECC_DEBUG_TAG "FNAND_ECC"
#define FNAND_ECC_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_ECC_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ECC_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_ECC_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_ECC_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_ECC_DEBUG_TAG, format, ##__VA_ARGS__)

#define FNAND_TIMING_DEBUG_TAG "FNAND_TIMING"
#define FNAND_TIMING_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_TIMING_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_TIMING_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_TIMING_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_TIMING_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_TIMING_DEBUG_TAG, format, ##__VA_ARGS__)

#define FNAND_BBM_DEBUG_TAG "FNAND_BBM"
#define FNAND_BBM_DEBUG_I(format, ...) FT_DEBUG_PRINT_I(FNAND_BBM_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_BBM_DEBUG_W(format, ...) FT_DEBUG_PRINT_W(FNAND_BBM_DEBUG_TAG, format, ##__VA_ARGS__)
#define FNAND_BBM_DEBUG_E(format, ...) FT_DEBUG_PRINT_E(FNAND_BBM_DEBUG_TAG, format, ##__VA_ARGS__)


#define FNAND_TIMING_ASY_NUM 12
#define FNAND_TIMING_SYN_NUM 14
#define FNAND_TIMING_TOG_NUM 12

#define ONFI_DYN_TIMING_MAX ((u16)~0U)

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_MICRON     0x2c
#define NAND_MFR_SKHYNIX    0x01


/* Cell info constants */
#define NAND_CI_CHIPNR_MSK	0x03
#define NAND_CI_CELLTYPE_MSK	0x0C
#define NAND_CI_CELLTYPE_SHIFT	2


#define NAND_MAX_ID_LEN 8

#define FNAND_ADDR_CYCLE_NUM0 0
#define FNAND_ADDR_CYCLE_NUM1 1
#define FNAND_ADDR_CYCLE_NUM2 2
#define FNAND_ADDR_CYCLE_NUM3 3
#define FNAND_ADDR_CYCLE_NUM4 4
#define FNAND_ADDR_CYCLE_NUM5 5

#define FNAND_COMMON_CRC_BASE	0x4F4E 
#define FNAND_TOGGLE_CRC_BASE	0x4F4E
#define FNAND_ONFI_CRC_BASE	0x4F4E


#define FNAND_CTRL_ECC_EN 1
#define FNAND_CTRL_ECC_DIS 0

#define FNAND_CTRL_AUTO_AUTO_RS_EN 1
#define FNAND_CTRL_AUTO_AUTO_RS_DIS 0
/*
 * Special handling must be done for the WAITRDY timeout parameter as it usually
 * is either tPROG (after a prog), tR (before a read), tRST (during a reset) or
 * tBERS (during an erase) which all of them are u64 values that cannot be
 * divided by usual kernel macros and must be handled with the special
 * DIV_ROUND_UP_ULL() macro.
 *
 * Cast to type of dividend is needed here to guarantee that the result won't
 * be an unsigned long long when the dividend is an unsigned long (or smaller),
 * which is what the compiler does when it sees ternary operator with 2
 * different return types (picks the largest type to make sure there's no
 * loss).
 */
#define __DIVIDE(dividend, divisor) ({						\
	(__typeof__(dividend))(sizeof(dividend) <= sizeof(unsigned long) ?	\
			       DIV_ROUND_UP(dividend, divisor) :		\
			       DIV_ROUND_UP_ULL(dividend, divisor)); 		\
	})
#define PSEC_TO_NSEC(x) __DIVIDE(x, 1000)
#define PSEC_TO_MSEC(x) __DIVIDE(x, 1000000000)


#define ONFI_CMD_READ_ID 0x90             /* ONFI Read ID \
                              command */
#define ONFI_CMD_READ_PARAM_PAGE 0xEC     /* ONFI Read \
                          Parameter Page                   \
                          command */


#define ONFI_END_CMD_NONE 0xfff /* No End command */

/*
 * Mandatory commands
 */

#define NAND_CMD_READ1 0x00  
#define NAND_CMD_READ2 0x30 /* READ PAGE */

#define NAND_CMD_CHANGE_READ_COLUMN1 0x05 /* NAND Random data Read \
                      Column command (1st                         \
                      cycle) */
#define NAND_CMD_CHANGE_READ_COLUMN2 0xE0 /* NAND Random data Read \
                      Column command (2nd                         \
                      cycle) */
#define NAND_CMD_BLOCK_ERASE1 0x60        /* NAND Block Erase \
                          (1st cycle) */
#define NAND_CMD_BLOCK_ERASE2 0xD0        /* NAND Block Erase \
                          (2nd cycle) */

#define NAND_CMD_PAGE_PROG1 0x80          /* NAND Page Program \
                          command (1st cycle)                      \
                          */
#define NAND_CMD_PAGE_PROG2 0x10          /* NAND Page Program \
                          command (2nd cycle)                      \
                          */

#define NAND_CMD_CHANGE_WRITE_COLUMN 0x85 /* NAND Change Write \
                         Column command */
#define NAND_CMD_READ_ID 0x90             /* NAND Read ID \
                              command */
#define NAND_CMD_READ_PARAM_PAGE 0xEC     /* NAND Read \
                          Parameter Page                   \
                          command */
#define NAND_CMD_RESET 0xFF               /* NAND Reset \
                          command */

#define NAND_END_CMD_NONE 0xfff /* No End command */

#define NAND_CMD_READ_STATUS 0x70 /* Read status */

#define FNAND_CMDCTRL_TYPE_RESET 0x00	 /* reset */
#define FNAND_CMDCTRL_TYPE_SET_FTR 0x01	 /* Set features */
#define FNAND_CMDCTRL_TYPE_GET_FTR 0x02	 /* Get features */
#define FNAND_CMDCTRL_TYPE_READ_ID 0x03	 /* Read ID */
#define FNAND_CMDCTRL_TYPE_READ_COL 0x03 /* Read Column */
#define FNAND_CMDCTRL_TYPE_PAGE_PRO 0x04 /* Page program */
#define FNAND_CMDCTRL_TYPE_ERASE 0x05	 /* Block Erase */
#define FNAND_CMDCTRL_TYPE_READ 0x06	 /* Read */
#define FNAND_CMDCTRL_TYPE_TOGGLE 0x07	 /* Toggle Two_plane */

#define FNAND_CMDCTRL_READ_PARAM 0x02
#define FNAND_CMDCTRL_READ_STATUS 0x03

#define FNAND_CMDCTRL_CH_READ_COL 0x03
#define FNAND_CMDCTRL_CH_ROW_ADDR 0x01
#define FNAND_CMDCTRL_CH_WR_COL 0x01

#define FNAND_NFC_ADDR_MAX_LEN 0x5

#define FNAND_DESCRIPTORS_SIZE 16

/*
* Mandatory commands
*/
	
#define TOGGLE_CMD_READ1 0x00
#define TOGGLE_CMD_READ2 0x30
	
#define TOGGLE_CMD_CHANGE_READ_COLUMN1 0x05 /* TOGGLE Change Read \
						  Column command (1st						  \
						  cycle) */
#define TOGGLE_CMD_CHANGE_READ_COLUMN2 0xE0 /* TOGGLE Change Read \
						  Column command (2nd						  \
						  cycle) */
#define TOGGLE_CMD_BLOCK_ERASE1 0x60        /* TOGGLE Block Erase \
							  (1st cycle) */
#define TOGGLE_CMD_BLOCK_ERASE2 0xD0        /* TOGGLE Block Erase \
							  (2nd cycle) */
#define TOGGLE_CMD_READ_STATUS 0x70         /* TOGGLE Read status \
							  command */
#define TOGGLE_CMD_PAGE_PROG1 0x80          /* TOGGLE Page Program \
							  command (1st cycle)					   \
							  */
#define TOGGLE_CMD_PAGE_PROG2 0x10          /* TOGGLE Page Program \
							  command (2nd cycle)					   \
							  */
#define TOGGLE_CMD_CHANGE_WRITE_COLUMN 0x85 /* TOGGLE Change Write \
							 Column command */
#define TOGGLE_CMD_READ_ID 0x90             /* TOGGLE Read ID \
								  command */
#define TOGGLE_CMD_READ_PARAM_PAGE 0xEC     /* TOGGLE Read \
							  Parameter Page				   \
							  command */
#define TOGGLE_CMD_RESET 0xFF               /* TOGGLE Reset \
							  command */
	
#define TOGGLE_END_CMD_NONE 0xfff /* No End command */
	
#define FNAND_CTRL0_OFFSET 0x00000000U
#define FNAND_CTRL1_OFFSET 0x00000004U
#define FNAND_MADDR0_OFFSET 0x00000008U
#define FNAND_MADDR1_OFFSET 0x0000000CU
	/* ASY */
#define FNAND_ASY_TIMING0_OFFSET 0x00000010U
#define FNAND_ASY_TIMING1_OFFSET 0x00000014U
#define FNAND_ASY_TIMING2_OFFSET 0x00000018U
#define FNAND_ASY_TIMING3_OFFSET 0x0000001CU
#define FNAND_ASY_TIMING4_OFFSET 0x00000020U
#define FNAND_ASY_TIMING5_OFFSET 0x00000024U
	/* SYN */
#define FNAND_SYN_TIMING6_OFFSET 0x00000028U
#define FNAND_SYN_TIMING7_OFFSET 0x0000002CU
#define FNAND_SYN_TIMING8_OFFSET 0x00000030U
#define FNAND_SYN_TIMING9_OFFSET 0x00000034U
#define FNAND_SYN_TIMING10_OFFSET 0x00000038U
#define FNAND_SYN_TIMING11_OFFSET 0x0000003CU
#define FNAND_SYN_TIMING12_OFFSET 0x00000040U
	/* TOG */
#define FNAND_TOG_TIMING13_OFFSET 0x00000044U
#define FNAND_TOG_TIMING14_OFFSET 0x00000048U
#define FNAND_TOG_TIMING15_OFFSET 0x0000004CU
#define FNAND_TOG_TIMING16_OFFSET 0x00000050U
#define FNAND_TOG_TIMING17_OFFSET 0x00000054U
#define FNAND_TOG_TIMING18_OFFSET 0x00000058U
	
#define FNAND_FIFORSTA_OFFSET 0x0000005CU
#define FNAND_INTERVAL_OFFSET 0x00000060U
#define FNAND_CMDINTERVAL_OFFSET 0x00000064U
#define FNAND_FIFO_TIMEOUT_OFFSET 0x00000068U
#define FNAND_FIFO_LEVEL0_FULL_OFFSET 0x0000006CU
#define FNAND_FIFO_LEVEL1_EMPTY_OFFSET 0x00000070U
#define FNAND_WP_OFFSET 0x00000074U
#define FNAND_FIFO_FREE_OFFSET 0x00000078U
#define FNAND_STATE_OFFSET 0x0000007CU
#define FNAND_INTRMASK_OFFSET 0x00000080U
#define FNAND_INTR_OFFSET 0x00000084U
#define FNAND_ERROR_CLEAR_OFFSET 0x0000008CU
#define FNAND_ERROR_LOCATION_OFFSET 0x000000B8U
	
	/* FNAND_CTRL0_OFFSET */
#define FNAND_CTRL0_EN_MASK BIT(0)
#define FNAND_CTRL0_WIDTH_MASK BIT(1)                            /* DQ width, only for ONFI async mode. 0: 8bits, 1: 16bits*/
#define FNAND_CTRL0_INTER_MODE(x) (min((x), (0x3UL)) << 2)         /* Nand Flash interface mode. 00: ONFI Async; 01: ONFI Sync; 10: Toggle Async*/
#define FNAND_CTRL0_ECC_EN_MASK BIT(4)                           /* Nand Flash hardware ECC enable */
#define FNAND_CTRL0_ECC_CORRECT_MAKE(x) (min((x), (0x7UL)) << 5) /* Nand Flash ECC strength. 3'h2: 2bits; 3'h4: 4bits */
#define FNAND_CTRL0_SPARE_SIZE_EN_MASK BIT(8)                    /* Data with spare  */
#define FNAND_CTRL0_SPARE_SIZE_MASK GENMASK(31, 9)               /* Spare size */
	
	/* FNAND_CTRL1_OFFSET */
	// #define FNAND_CTRL1_SAMPL_PHASE_MASK GENMASK(15,0)	/* when onfi synchronization or toggle mode, the cycle of receive data sampling phase */
#define FNAND_CTRL1_SAMPL_PHASE_MAKE(x) (min((x), GENMASK(15, 0))) /* when onfi synchronization or toggle mode, the cycle of receive data sampling phase */
#define FNAND_CTRL1_ECC_DATA_FIRST_EN_MASK BIT(16)                 /* ECC data is read first and then the corresponding data is read */
#define FNAND_CTRL1_RB_SHARE_EN_MASK BIT(17)                       /* The R/B signal sharing function of four devices is enabled. Write 1 is enabled */
#define FNAND_CTRL1_ECC_BYPASS_MASK BIT(18)                        /* When the received ECC encoded address data is 13'hff, the ECC verification function is bypass. 1 indicates that the function is enabled */
	
	/* FNAND_MADDR0_OFFSET */
#define FNAND_MADDR0_DT_LOW_ADDR_MASK GENMASK(31, 0) /* The lower 32 bits of the descriptor table header address in memory storage */
	
	/* FNAND_MADDR1_OFFSET */
#define FNAND_MADDR1_DT_HIGH_8BITADDR_MASK GENMASK(7, 0)   /* The high 8 bits of the first address of the descriptor table stored in memory */
#define FNAND_MADDR1_DMA_EN_MASK BIT(8)                    /* DMA transfer enable bit. This bit is 1 for the controller to start DMA transfers */
#define FNAND_MADDR1_DMA_READ_LENGTH_MASK GENMASK(23, 16)  /* Sets the length to which dma reads data */
#define FNAND_MADDR1_DMA_WRITE_LENGTH_MASK GENMASK(31, 24) /* Sets the length to which dma writes data */
	
	/* FNAND_ASY_TIMING0_OFFSET */
#define FNAND_ASY_TIMING0_TCLS_TWP_MASK GENMASK(31, 16) /* tCL-tWP */
#define FNAND_ASY_TIMING0_TCLS_TCS_MASK GENMASK(15, 0)  /* tCS-tCLS */
	
	/* FNAND_ASY_TIMING1_OFFSET */
#define FNAND_ASY_TIMING1_TWH_MASK GENMASK(31, 16) /* tWH */
#define FNAND_ASY_TIMING1_TWP_MASK GENMASK(15, 0)  /* tWP */
	
	/* FNAND_ASY_TIMING2_OFFSET */
#define FNAND_ASY_TIMING2_TCLH_TWH_MASK GENMASK(31, 16) /* tCLH-tWH */
#define FNAND_ASY_TIMING2_TCH_TCLH_MASK GENMASK(15, 0)  /* tCH-tCLH */
	
	/* FNAND_ASY_TIMING3_OFFSET */
#define FNAND_ASY_TIMING3_TCH_TWH_MASK GENMASK(31, 16) /* tCH-tWH */
#define FNAND_ASY_TIMING3_TDQ_EN_MASK GENMASK(15, 0)
	
	/* FNAND_ASY_TIMING4_OFFSET */
#define FNAND_ASY_TIMING4_TREH_MASK GENMASK(31, 16) /* TREH */
#define FNAND_ASY_TIMING4_TWHR_MASK GENMASK(15, 0)  /* TWHR */
	
	/* FNAND_ASY_TIMING5_OFFSET */
#define FNAND_ASY_TIMING5_TADL_MASK GENMASK(31, 16)
#define FNAND_ASY_TIMING5_TRC_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING6_OFFSET */
#define FNAND_SYN_TIMING6_TCALS_TCH_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING6_TRC_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING7_OFFSET */
#define FNAND_SYN_TIMING7_TDQ_EN_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING7_TCK_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING8_OFFSET */
#define FNAND_SYN_TIMING8_TCK_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING8_TCAD_TCK_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING9_OFFSET */
#define FNAND_SYN_TIMING9_TCCS_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING9_TWHR_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING10_OFFSET */
#define FNAND_SYN_TIMING10_TCK_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING10_MTCK_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING11_OFFSET */
#define FNAND_SYN_TIMING11_TCK_TCALS_MASK GENMASK(15, 0)
	
	/* FNAND_SYN_TIMING12_OFFSET */
#define FNAND_SYN_TIMING12_TCKWR_MASK GENMASK(31, 16)
#define FNAND_SYN_TIMING12_TWRCK_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING13_OFFSET */
#define FNAND_TOG_TIMING13_TWRPST_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING13_TWPRE_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING14_OFFSET */
#define FNAND_TOG_TIMING14_TCLS_TWP_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING14_TCS_TCLS_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING15_OFFSET */
#define FNAND_TOG_TIMING15_TWHR_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING15_TADL_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING16_OFFSET */
#define FNAND_TOG_TIMING16_TCLH_TWH_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING16_TCH_TCLH_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING17_OFFSET */
#define FNAND_TOG_TIMING17_TRPST_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING17_TRPRE_MASK GENMASK(15, 0)
	
	/* FNAND_TOG_TIMING18_OFFSET */
#define FNAND_TOG_TIMING18_TRPSTH_MASK GENMASK(31, 16)
#define FNAND_TOG_TIMING18_DSC_MASK GENMASK(15, 0)
	
	/* FNAND_FIFORSTA_OFFSET */
#define FNAND_FIFORSTA_FIFO_FULL_MASK BIT(11)
#define FNAND_FIFORSTA_FIFO_EMPTY_MASK BIT(10)
#define FNAND_FIFORSTA_FIFO_COUNT_MASK GENMASK(9, 0)
	
	/* FNAND_INTERVAL_OFFSET */
	// #define FNAND_INTERVAL_TIME_MASK GENMASK(15,0) /* The interval between commands, addresses, and data. The timeout increases by 2ns for every 1 increase in the write value */
#define FNAND_INTERVAL_TIME_MAKE(x) (min((x), (0xFFUL)))
	
	/* FNAND_CMDINTERVAL_OFFSET */
#define FNAND_CMDINTERVAL_MASK GENMASK(31, 0) /* The interval between requests. The timeout increases by 2ns for every 1 increase in the write value */
	
	/* FNAND_FIFO_TIMEOUT_OFFSET */
#define FNAND_FIFO_TIMEOUT_MASK GENMASK(31, 0) /* FIFO timeout counter, the timeout time increases by 2ns for each increment of the value written */
	
	/* FNAND_FIFO_LEVEL0_FULL_OFFSET */
#define FNAND_FIFO_LEVEL0_FULL_THRESHOLD_MASK GENMASK(3, 0)
	
	/* FNAND_FIFO_LEVEL1_EMPTY_OFFSET */
#define FNAND_FIFO_LEVEL1_EMPTY_THRESHOLD_MASK GENMASK(3, 0)
	
	/* FNAND_WP_OFFSET */
#define FNAND_WP_EN_MASK BIT(0)
	
	/* FNAND_FIFO_FREE_OFFSET */
#define FNAND_FIFO_FREE_MASK BIT(0)
	
	/* FNAND_STATE_OFFSET */
#define FNAND_STATE_BUSY_OFFSET BIT(0)         /* nandflash控制器忙 */
#define FNAND_STATE_DMA_BUSY_OFFSET BIT(1)     /* dma控制器忙 */
#define FNAND_STATE_DMA_PGFINISH_OFFSET BIT(2) /* dma数据操作完成 */
#define FNAND_STATE_DMA_FINISH_OFFSET BIT(3)   /* dma完成 */
#define FNAND_STATE_FIFO_EMP_OFFSET BIT(4)
#define FNAND_STATE_FIFO_FULL_OFFSET BIT(5)
#define FNAND_STATE_FIFO_TIMEOUT_OFFSET BIT(6)
#define FNAND_STATE_CS_OFFSET GENMASK(10, 7)
#define FNAND_STATE_CMD_PGFINISH_OFFSET BIT(11) /* nand接口命令操作完成 */
#define FNAND_STATE_PG_PGFINISH_OFFSET BIT(12)  /* nand接口数据操作完成 */
#define FNAND_STATE_RE_OFFSET BIT(13)           /* re_n门控状态 */
#define FNAND_STATE_DQS_OFFSET BIT(14)          /* dqs门控状态 */
#define FNAND_STATE_RB_OFFSET BIT(15)           /* RB_N接口的状态 */
#define FNAND_STATE_ECC_BUSY_OFFSET BIT(16)
#define FNAND_STATE_ECC_FINISH_OFFSET BIT(17)
#define FNAND_STATE_ECC_RIGHT_OFFSET BIT(18)    
#define FNAND_STATE_ECC_ERR_OFFSET BIT(19)      /* ECC 校验有错 */
#define FNAND_STATE_ECC_ERROVER_OFFSET BIT(20) /* 错误超过可校验范围  */
#define FNAND_STATE_AXI_DSP_ERR_OFFSET BIT(21) /* 描述符错误 */
#define FNAND_STATE_AXI_RD_ERR_OFFSET BIT(22)
#define FNAND_STATE_AXI_WR_ERR_OFFSET BIT(23)
#define FNAND_STATE_RB_N_OFFSET GENMASK(27, 24)
#define FNAND_STATE_PROT_ERR_OFFSET BIT(28)
#define FNAND_STATE_ECCBYPASS_STA_OFFSET BIT(29)
#define FNAND_STATE_ALL_BIT GENMASK(29, 0)
	
	/* FNAND_INTRMASK_OFFSET */
#define FNAND_INTRMASK_ALL_INT_MASK GENMASK(17, 0)
#define FNAND_INTRMASK_BUSY_MASK BIT(0)         /* nandflash控制器忙状态中断屏蔽位 */
#define FNAND_INTRMASK_DMA_BUSY_MASK BIT(1)     /* dma控制器忙状态中断屏蔽位 */
#define FNAND_INTRMASK_DMA_PGFINISH_MASK BIT(2) /* dma页操作完成中断屏蔽位 */
#define FNAND_INTRMASK_DMA_FINISH_MASK BIT(3)   /* dma操作完成中断完成中断屏蔽位 */
#define FNAND_INTRMASK_FIFO_EMP_MASK BIT(4)     /* fifo为空中断屏蔽位 */
#define FNAND_INTRMASK_FIFO_FULL_MASK BIT(5)    /* fifo为满中断屏蔽位 */
#define FNAND_INTRMASK_FIFO_TIMEOUT_MASK BIT(6) /* fifo超时中断屏蔽位 */
#define FNAND_INTRMASK_CMD_FINISH_MASK BIT(7)   /* nand接口命令完成中断屏蔽位 */
#define FNAND_INTRMASK_PGFINISH_MASK BIT(8)     /* nand接口页操作完成中断屏蔽位 */
#define FNAND_INTRMASK_RE_MASK BIT(9)           /* re_n门控打开中断屏蔽位 */
#define FNAND_INTRMASK_DQS_MASK BIT(10)         /* dqs门控打开中断屏蔽位 */
#define FNAND_INTRMASK_RB_MASK BIT(11)          /* rb_n信号busy中断屏蔽位 */
#define FNAND_INTRMASK_ECC_FINISH_MASK BIT(12)  /* ecc完成中断屏蔽位 */
#define FNAND_INTRMASK_ECC_ERR_MASK BIT(13)     /* ecc 中断屏蔽位 */
	
	/* FNAND_INTR_OFFSET */
#define FNAND_INTR_ALL_INT_MASK GENMASK(17, 0)
#define FNAND_INTR_BUSY_MASK BIT(0)         /* nandflash控制器忙状态中断状态位 */
#define FNAND_INTR_DMA_BUSY_MASK BIT(1)     /* dma控制器忙状态中断状态位 */
#define FNAND_INTR_DMA_PGFINISH_MASK BIT(2) /* dma页操作完成中断状态位 */
#define FNAND_INTR_DMA_FINISH_MASK BIT(3)   /* dma操作完成中断完成中断状态位 */
#define FNAND_INTR_FIFO_EMP_MASK BIT(4)     /* fifo为空中断状态位 */
#define FNAND_INTR_FIFO_FULL_MASK BIT(5)    /* fifo为满中断状态位 */
#define FNAND_INTR_FIFO_TIMEOUT_MASK BIT(6) /* fifo超时中断状态位 */
#define FNAND_INTR_CMD_FINISH_MASK BIT(7)   /* nand接口命令完成中断状态位 */
#define FNAND_INTR_PGFINISH_MASK BIT(8)     /* nand接口页操作完成中断状态位 */
#define FNAND_INTR_RE_MASK BIT(9)           /* re_n门控打开中断状态位 */
#define FNAND_INTR_DQS_MASK BIT(10)         /* dqs门控打开中断状态位 */
#define FNAND_INTR_RB_MASK BIT(11)          /* rb_n信号busy中断状态位 */
#define FNAND_INTR_ECC_FINISH_MASK BIT(12)  /* ecc完成中断状态蔽位 */
#define FNAND_INTR_ECC_ERR_MASK BIT(13)     /* ecc正确中断状态蔽位 */
	
	/* FNAND_ERROR_CLEAR_OFFSET */
#define FNAND_ERROR_CLEAR_DSP_ERR_CLR_MASK BIT(0)
#define FNAND_ERROR_CLEAR_AXI_RD_ERR_CLR_MASK BIT(1)
#define FNAND_ERROR_CLEAR_AXI_WR_ERR_CLR_MASK BIT(2)
#define FNAND_ERROR_CLEAR_ECC_ERR_CLR_MASK BIT(3)
#define FNAND_ERROR_ALL_CLEAR GENMASK(3, 0)
	
#define FNAND_SELETED_MAX_NUMBER 4

 /* LSD Config */
#define FLSD_CONFIG_BASE            0x2807E000U
#define FLSD_NAND_MMCSD_HADDR       0xC0U
#define FLSD_CK_STOP_CONFIG0_HADDR  0x10U
 
 /************************** Constant Definitions *****************************/
 /*
  * Block definitions for RAM based Bad Block Table (BBT)
  */
#define FNAND_BLOCK_GOOD 0x0		/* Block is good */
#define FNAND_BLOCK_BAD 0x1			/* Block is bad */
#define FNAND_BLOCK_RESERVED 0x2	/* Reserved block */
#define FNAND_BLOCK_FACTORY_BAD 0x3 /* Factory marked bad block */
 
 /*
  * Block definitions for FLASH based Bad Block Table (BBT)
  */
#define FNAND_FLASH_BLOCK_GOOD 0x3		  /* Block is good */
#define FNAND_FLASH_BLOCK_BAD 0x2		  /* Block is bad */
#define FNAND_FLASH_BLOCK_RESERVED 0x1	  /* Reserved block */
#define FNAND_FLASH_BLOCK_FACTORY_BAD 0x0 /* Factory marked bad block */
 
#define FNAND_BBT_SCAN_2ND_PAGE 0x00000001 /* Scan the \
							  second page				\
							  for bad block 			\
							  information				\
							  */
#define FNAND_BBT_DESC_PAGE_OFFSET 0	   /* Page offset of Bad \
							  Block Table Desc */
#define FNAND_BBT_DESC_SIG_OFFSET 8		   /* Bad Block Table \
							  signature offset */
#define FNAND_BBT_DESC_VER_OFFSET 12	   /* Bad block Table \
							  version offset */
#define FNAND_BBT_DESC_SIG_LEN 4		   /* Bad block Table \
								  signature length */
#define FNAND_BBT_DESC_MAX_BLOCKS 4		   /* Bad block Table \
							  max blocks */
 
#define FNAND_BBT_BLOCK_SHIFT 2				 /* Block shift value \
									for a block in BBT */
#define FNAND_BBT_ENTRY_NUM_BLOCKS 4		 /* Num of blocks in \
								one BBT entry */
#define FNAND_BB_PATTERN_OFFSET_SMALL_PAGE 5 /* Bad block pattern \
							offset in a page */
#define FNAND_BB_PATTERN_LENGTH_SMALL_PAGE 1 /* Bad block pattern \
							length */
#define FNAND_BB_PATTERN_OFFSET_LARGE_PAGE 0 /* Bad block pattern \
							offset in a large					   \
							page */
#define FNAND_BB_PATTERN_LENGTH_LARGE_PAGE 2 /* Bad block pattern \
							length */
#define FNAND_BB_PATTERN 0xFF				 /* Bad block pattern \
								to search in a page 			   \
								*/
#define FNAND_BLOCK_TYPE_MASK 0x03			 /* Block type mask */
#define FNAND_BLOCK_SHIFT_MASK 0x06			 /* Block shift mask \
								for a Bad Block Table			  \
								entry byte */
 
#define FNAND_BBTBLOCKSHIFT(block) \
	 ((block * 2) & FNAND_BLOCK_SHIFT_MASK)
 
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

#define FIOPAD_INDEX(offset) \
	{ \
		 /* reg_off */	 (offset),	\
		 /* reg_bit */	 (0) \
	}
#define FIOPAD_0_FUNC_OFFSET     0x0000U
#define FIOPAD_2_FUNC_OFFSET     0x0004U
#define FIOPAD_3_FUNC_OFFSET     0x0008U
#define FIOPAD_4_FUNC_OFFSET     0x000CU
#define FIOPAD_5_FUNC_OFFSET     0x0010U
#define FIOPAD_6_FUNC_OFFSET     0x0014U
#define FIOPAD_7_FUNC_OFFSET     0x0018U
#define FIOPAD_8_FUNC_OFFSET     0x001CU
#define FIOPAD_9_FUNC_OFFSET     0x0020U
#define FIOPAD_10_FUNC_OFFSET    0x0024U
#define FIOPAD_11_FUNC_OFFSET    0x0028U
#define FIOPAD_12_FUNC_OFFSET    0x002CU
#define FIOPAD_13_FUNC_OFFSET    0x0030U
#define FIOPAD_14_FUNC_OFFSET    0x0034U
#define FIOPAD_15_FUNC_OFFSET    0x0038U
#define FIOPAD_16_FUNC_OFFSET    0x003CU
#define FIOPAD_17_FUNC_OFFSET    0x0040U
#define FIOPAD_18_FUNC_OFFSET    0x0044U
#define FIOPAD_19_FUNC_OFFSET    0x0048U
#define FIOPAD_20_FUNC_OFFSET    0x004CU
#define FIOPAD_21_FUNC_OFFSET    0x0050U
#define FIOPAD_22_FUNC_OFFSET    0x0054U
#define FIOPAD_23_FUNC_OFFSET    0x0058U
#define FIOPAD_24_FUNC_OFFSET    0x005CU
#define FIOPAD_25_FUNC_OFFSET    0x0060U
#define FIOPAD_26_FUNC_OFFSET    0x0064U
#define FIOPAD_27_FUNC_OFFSET    0x0068U
#define FIOPAD_28_FUNC_OFFSET    0x006CU
#define FIOPAD_31_FUNC_OFFSET    0x0070U
#define FIOPAD_32_FUNC_OFFSET    0x0074U
#define FIOPAD_33_FUNC_OFFSET    0x0078U
#define FIOPAD_34_FUNC_OFFSET    0x007CU
#define FIOPAD_35_FUNC_OFFSET    0x0080U
#define FIOPAD_36_FUNC_OFFSET    0x0084U
#define FIOPAD_37_FUNC_OFFSET    0x0088U
#define FIOPAD_38_FUNC_OFFSET    0x008CU
#define FIOPAD_39_FUNC_OFFSET    0x0090U
#define FIOPAD_40_FUNC_OFFSET    0x0094U
#define FIOPAD_41_FUNC_OFFSET    0x0098U
#define FIOPAD_42_FUNC_OFFSET    0x009CU
#define FIOPAD_43_FUNC_OFFSET    0x00A0U
#define FIOPAD_44_FUNC_OFFSET    0x00A4U
#define FIOPAD_45_FUNC_OFFSET    0x00A8U
#define FIOPAD_46_FUNC_OFFSET    0x00ACU
#define FIOPAD_47_FUNC_OFFSET    0x00B0U
#define FIOPAD_48_FUNC_OFFSET    0x00B4U
#define FIOPAD_49_FUNC_OFFSET    0x00B8U
#define FIOPAD_50_FUNC_OFFSET    0x00BCU
#define FIOPAD_51_FUNC_OFFSET    0x00C0U
#define FIOPAD_52_FUNC_OFFSET    0x00C4U
#define FIOPAD_53_FUNC_OFFSET    0x00C8U
#define FIOPAD_54_FUNC_OFFSET    0x00CCU
#define FIOPAD_55_FUNC_OFFSET    0x00D0U
#define FIOPAD_56_FUNC_OFFSET    0x00D4U
#define FIOPAD_57_FUNC_OFFSET    0x00D8U
#define FIOPAD_58_FUNC_OFFSET    0x00DCU
#define FIOPAD_59_FUNC_OFFSET    0x00E0U
#define FIOPAD_60_FUNC_OFFSET    0x00E4U
#define FIOPAD_61_FUNC_OFFSET    0x00E8U
#define FIOPAD_62_FUNC_OFFSET    0x00ECU
#define FIOPAD_63_FUNC_OFFSET    0x00F0U
#define FIOPAD_64_FUNC_OFFSET    0x00F4U
#define FIOPAD_65_FUNC_OFFSET    0x00F8U
#define FIOPAD_66_FUNC_OFFSET    0x00FCU
#define FIOPAD_67_FUNC_OFFSET    0x0100U
#define FIOPAD_68_FUNC_OFFSET    0x0104U
#define FIOPAD_148_FUNC_OFFSET   0x0108U
#define FIOPAD_69_FUNC_OFFSET    0x010CU
#define FIOPAD_70_FUNC_OFFSET    0x0110U
#define FIOPAD_71_FUNC_OFFSET    0x0114U
#define FIOPAD_72_FUNC_OFFSET    0x0118U
#define FIOPAD_73_FUNC_OFFSET    0x011CU
#define FIOPAD_74_FUNC_OFFSET    0x0120U
#define FIOPAD_75_FUNC_OFFSET    0x0124U
#define FIOPAD_76_FUNC_OFFSET    0x0128U
#define FIOPAD_77_FUNC_OFFSET    0x012CU
#define FIOPAD_78_FUNC_OFFSET    0x0130U
#define FIOPAD_79_FUNC_OFFSET    0x0134U
#define FIOPAD_80_FUNC_OFFSET    0x0138U
#define FIOPAD_81_FUNC_OFFSET    0x013CU
#define FIOPAD_82_FUNC_OFFSET    0x0140U
#define FIOPAD_83_FUNC_OFFSET    0x0144U
#define FIOPAD_84_FUNC_OFFSET    0x0148U
#define FIOPAD_85_FUNC_OFFSET    0x014CU
#define FIOPAD_86_FUNC_OFFSET    0x0150U
#define FIOPAD_87_FUNC_OFFSET    0x0154U
#define FIOPAD_88_FUNC_OFFSET    0x0158U
#define FIOPAD_89_FUNC_OFFSET    0x015CU
#define FIOPAD_90_FUNC_OFFSET    0x0160U
#define FIOPAD_91_FUNC_OFFSET    0x0164U
#define FIOPAD_92_FUNC_OFFSET    0x0168U
#define FIOPAD_93_FUNC_OFFSET    0x016CU
#define FIOPAD_94_FUNC_OFFSET    0x0170U
#define FIOPAD_95_FUNC_OFFSET    0x0174U
#define FIOPAD_96_FUNC_OFFSET    0x0178U
#define FIOPAD_97_FUNC_OFFSET    0x017CU
#define FIOPAD_98_FUNC_OFFSET    0x0180U
#define FIOPAD_29_FUNC_OFFSET    0x0184U
#define FIOPAD_30_FUNC_OFFSET    0x0188U
#define FIOPAD_99_FUNC_OFFSET	0x018CU
#define FIOPAD_100_FUNC_OFFSET	0x0190U
#define FIOPAD_101_FUNC_OFFSET	0x0194U
#define FIOPAD_102_FUNC_OFFSET	0x0198U
#define FIOPAD_103_FUNC_OFFSET	0x019CU
#define FIOPAD_104_FUNC_OFFSET	0x01A0U
#define FIOPAD_105_FUNC_OFFSET	0x01A4U
#define FIOPAD_106_FUNC_OFFSET	0x01A8U
#define FIOPAD_107_FUNC_OFFSET	0x01ACU
#define FIOPAD_108_FUNC_OFFSET	0x01B0U
#define FIOPAD_109_FUNC_OFFSET	0x01B4U
#define FIOPAD_110_FUNC_OFFSET	0x01B8U
#define FIOPAD_111_FUNC_OFFSET	0x01BCU
#define FIOPAD_112_FUNC_OFFSET	0x01C0U
#define FIOPAD_113_FUNC_OFFSET	0x01C4U
#define FIOPAD_114_FUNC_OFFSET	0x01C8U
#define FIOPAD_115_FUNC_OFFSET	0x01CCU
#define FIOPAD_116_FUNC_OFFSET	0x01D0U
#define FIOPAD_117_FUNC_OFFSET	0x01D4U
#define FIOPAD_118_FUNC_OFFSET	0x01D8U
#define FIOPAD_119_FUNC_OFFSET	0x01DCU
#define FIOPAD_120_FUNC_OFFSET	0x01E0U
#define FIOPAD_121_FUNC_OFFSET	0x01E4U
#define FIOPAD_122_FUNC_OFFSET	0x01E8U
#define FIOPAD_123_FUNC_OFFSET	0x01ECU
#define FIOPAD_124_FUNC_OFFSET	0x01F0U
#define FIOPAD_125_FUNC_OFFSET	0x01F4U
#define FIOPAD_126_FUNC_OFFSET	0x01F8U
#define FIOPAD_127_FUNC_OFFSET	0x01FCU
#define FIOPAD_128_FUNC_OFFSET	0x0200U
#define FIOPAD_129_FUNC_OFFSET	0x0204U
#define FIOPAD_130_FUNC_OFFSET	0x0208U
#define FIOPAD_131_FUNC_OFFSET	0x020CU
#define FIOPAD_132_FUNC_OFFSET	0x0210U
#define FIOPAD_133_FUNC_OFFSET	0x0214U
#define FIOPAD_134_FUNC_OFFSET	0x0218U
#define FIOPAD_135_FUNC_OFFSET	0x021CU
#define FIOPAD_136_FUNC_OFFSET	0x0220U
#define FIOPAD_137_FUNC_OFFSET	0x0224U
#define FIOPAD_138_FUNC_OFFSET	0x0228U
#define FIOPAD_139_FUNC_OFFSET	0x022CU
#define FIOPAD_140_FUNC_OFFSET	0x0230U
#define FIOPAD_141_FUNC_OFFSET	0x0234U
#define FIOPAD_142_FUNC_OFFSET	0x0238U
#define FIOPAD_143_FUNC_OFFSET	0x023CU
#define FIOPAD_144_FUNC_OFFSET	0x0240U
#define FIOPAD_145_FUNC_OFFSET	0x0244U
#define FIOPAD_146_FUNC_OFFSET	0x0248U
#define FIOPAD_147_FUNC_OFFSET	0x024CU
 
 /* register offset of iopad function / pull / driver strength */
#define FIOPAD_AN55     	(fpin_index)FIOPAD_INDEX(FIOPAD_0_FUNC_OFFSET)
#define FIOPAD_AW43     	(fpin_index)FIOPAD_INDEX(FIOPAD_2_FUNC_OFFSET)
#define FIOPAD_AR51     	(fpin_index)FIOPAD_INDEX(FIOPAD_9_FUNC_OFFSET)
#define FIOPAD_AJ51     	(fpin_index)FIOPAD_INDEX(FIOPAD_10_FUNC_OFFSET)
#define FIOPAD_AL51     	(fpin_index)FIOPAD_INDEX(FIOPAD_11_FUNC_OFFSET)
#define FIOPAD_AL49     	(fpin_index)FIOPAD_INDEX(FIOPAD_12_FUNC_OFFSET)
#define FIOPAD_AN47     	(fpin_index)FIOPAD_INDEX(FIOPAD_13_FUNC_OFFSET)
#define FIOPAD_AR47     	(fpin_index)FIOPAD_INDEX(FIOPAD_14_FUNC_OFFSET)
#define FIOPAD_BA53     	(fpin_index)FIOPAD_INDEX(FIOPAD_15_FUNC_OFFSET)
#define FIOPAD_BA55     	(fpin_index)FIOPAD_INDEX(FIOPAD_16_FUNC_OFFSET)
#define FIOPAD_AW53     	(fpin_index)FIOPAD_INDEX(FIOPAD_17_FUNC_OFFSET)
#define FIOPAD_AW55     	(fpin_index)FIOPAD_INDEX(FIOPAD_18_FUNC_OFFSET)
#define FIOPAD_AU51     	(fpin_index)FIOPAD_INDEX(FIOPAD_19_FUNC_OFFSET)
#define FIOPAD_AN53     	(fpin_index)FIOPAD_INDEX(FIOPAD_20_FUNC_OFFSET)
#define FIOPAD_AL55     	(fpin_index)FIOPAD_INDEX(FIOPAD_21_FUNC_OFFSET)
#define FIOPAD_AJ55     	(fpin_index)FIOPAD_INDEX(FIOPAD_22_FUNC_OFFSET)
#define FIOPAD_AJ53     	(fpin_index)FIOPAD_INDEX(FIOPAD_23_FUNC_OFFSET)
#define FIOPAD_AG55     	(fpin_index)FIOPAD_INDEX(FIOPAD_24_FUNC_OFFSET)
#define FIOPAD_AG53     	(fpin_index)FIOPAD_INDEX(FIOPAD_25_FUNC_OFFSET)
#define FIOPAD_AE55     	(fpin_index)FIOPAD_INDEX(FIOPAD_26_FUNC_OFFSET)
#define FIOPAD_AC55     	(fpin_index)FIOPAD_INDEX(FIOPAD_27_FUNC_OFFSET)
#define FIOPAD_AC53     	(fpin_index)FIOPAD_INDEX(FIOPAD_28_FUNC_OFFSET)
#define FIOPAD_AR45     	(fpin_index)FIOPAD_INDEX(FIOPAD_31_FUNC_OFFSET)
#define FIOPAD_BA51     	(fpin_index)FIOPAD_INDEX(FIOPAD_32_FUNC_OFFSET)
#define FIOPAD_BA49     	(fpin_index)FIOPAD_INDEX(FIOPAD_33_FUNC_OFFSET)
#define FIOPAD_AR55     	(fpin_index)FIOPAD_INDEX(FIOPAD_34_FUNC_OFFSET)
#define FIOPAD_AU55     	(fpin_index)FIOPAD_INDEX(FIOPAD_35_FUNC_OFFSET)
#define FIOPAD_AR53     	(fpin_index)FIOPAD_INDEX(FIOPAD_36_FUNC_OFFSET)
#define FIOPAD_BA45     	(fpin_index)FIOPAD_INDEX(FIOPAD_37_FUNC_OFFSET)
#define FIOPAD_AW51     	(fpin_index)FIOPAD_INDEX(FIOPAD_38_FUNC_OFFSET)
#define FIOPAD_A31          (fpin_index)FIOPAD_INDEX(FIOPAD_39_FUNC_OFFSET)
#define FIOPAD_R53          (fpin_index)FIOPAD_INDEX(FIOPAD_40_FUNC_OFFSET)
#define FIOPAD_R55          (fpin_index)FIOPAD_INDEX(FIOPAD_41_FUNC_OFFSET)
#define FIOPAD_U55          (fpin_index)FIOPAD_INDEX(FIOPAD_42_FUNC_OFFSET)
#define FIOPAD_W55          (fpin_index)FIOPAD_INDEX(FIOPAD_43_FUNC_OFFSET)
#define FIOPAD_U53          (fpin_index)FIOPAD_INDEX(FIOPAD_44_FUNC_OFFSET)
#define FIOPAD_AA53         (fpin_index)FIOPAD_INDEX(FIOPAD_45_FUNC_OFFSET)
#define FIOPAD_AA55         (fpin_index)FIOPAD_INDEX(FIOPAD_46_FUNC_OFFSET)
#define FIOPAD_AW47         (fpin_index)FIOPAD_INDEX(FIOPAD_47_FUNC_OFFSET)
#define FIOPAD_AU47         (fpin_index)FIOPAD_INDEX(FIOPAD_48_FUNC_OFFSET)
#define FIOPAD_A35          (fpin_index)FIOPAD_INDEX(FIOPAD_49_FUNC_OFFSET)
#define FIOPAD_C35          (fpin_index)FIOPAD_INDEX(FIOPAD_50_FUNC_OFFSET)
#define FIOPAD_C33          (fpin_index)FIOPAD_INDEX(FIOPAD_51_FUNC_OFFSET)
#define FIOPAD_A33          (fpin_index)FIOPAD_INDEX(FIOPAD_52_FUNC_OFFSET)
#define FIOPAD_A37          (fpin_index)FIOPAD_INDEX(FIOPAD_53_FUNC_OFFSET)
#define FIOPAD_A39          (fpin_index)FIOPAD_INDEX(FIOPAD_54_FUNC_OFFSET)
#define FIOPAD_A41          (fpin_index)FIOPAD_INDEX(FIOPAD_55_FUNC_OFFSET)
#define FIOPAD_C41          (fpin_index)FIOPAD_INDEX(FIOPAD_56_FUNC_OFFSET)
#define FIOPAD_A43          (fpin_index)FIOPAD_INDEX(FIOPAD_57_FUNC_OFFSET)
#define FIOPAD_A45          (fpin_index)FIOPAD_INDEX(FIOPAD_58_FUNC_OFFSET)
#define FIOPAD_C45          (fpin_index)FIOPAD_INDEX(FIOPAD_59_FUNC_OFFSET)
#define FIOPAD_A47          (fpin_index)FIOPAD_INDEX(FIOPAD_60_FUNC_OFFSET)
#define FIOPAD_A29          (fpin_index)FIOPAD_INDEX(FIOPAD_61_FUNC_OFFSET)
#define FIOPAD_C29          (fpin_index)FIOPAD_INDEX(FIOPAD_62_FUNC_OFFSET)
#define FIOPAD_C27          (fpin_index)FIOPAD_INDEX(FIOPAD_63_FUNC_OFFSET)
#define FIOPAD_A27          (fpin_index)FIOPAD_INDEX(FIOPAD_64_FUNC_OFFSET)
#define FIOPAD_AJ49         (fpin_index)FIOPAD_INDEX(FIOPAD_65_FUNC_OFFSET)
#define FIOPAD_AL45         (fpin_index)FIOPAD_INDEX(FIOPAD_66_FUNC_OFFSET)
#define FIOPAD_AL43         (fpin_index)FIOPAD_INDEX(FIOPAD_67_FUNC_OFFSET)
#define FIOPAD_AN45         (fpin_index)FIOPAD_INDEX(FIOPAD_68_FUNC_OFFSET)
#define FIOPAD_AG47         (fpin_index)FIOPAD_INDEX(FIOPAD_148_FUNC_OFFSET)
#define FIOPAD_AJ47         (fpin_index)FIOPAD_INDEX(FIOPAD_69_FUNC_OFFSET)
#define FIOPAD_AG45         (fpin_index)FIOPAD_INDEX(FIOPAD_70_FUNC_OFFSET)
#define FIOPAD_AE51         (fpin_index)FIOPAD_INDEX(FIOPAD_71_FUNC_OFFSET)
#define FIOPAD_AE49         (fpin_index)FIOPAD_INDEX(FIOPAD_72_FUNC_OFFSET)
#define FIOPAD_AG51         (fpin_index)FIOPAD_INDEX(FIOPAD_73_FUNC_OFFSET)
#define FIOPAD_AJ45         (fpin_index)FIOPAD_INDEX(FIOPAD_74_FUNC_OFFSET)
#define FIOPAD_AC51         (fpin_index)FIOPAD_INDEX(FIOPAD_75_FUNC_OFFSET)
#define FIOPAD_AC49         (fpin_index)FIOPAD_INDEX(FIOPAD_76_FUNC_OFFSET)
#define FIOPAD_AE47         (fpin_index)FIOPAD_INDEX(FIOPAD_77_FUNC_OFFSET)
#define FIOPAD_W47          (fpin_index)FIOPAD_INDEX(FIOPAD_78_FUNC_OFFSET)
#define FIOPAD_W51          (fpin_index)FIOPAD_INDEX(FIOPAD_79_FUNC_OFFSET)
#define FIOPAD_W49          (fpin_index)FIOPAD_INDEX(FIOPAD_80_FUNC_OFFSET)
#define FIOPAD_U51          (fpin_index)FIOPAD_INDEX(FIOPAD_81_FUNC_OFFSET)
#define FIOPAD_U49          (fpin_index)FIOPAD_INDEX(FIOPAD_82_FUNC_OFFSET)
#define FIOPAD_AE45         (fpin_index)FIOPAD_INDEX(FIOPAD_83_FUNC_OFFSET)
#define FIOPAD_AC45         (fpin_index)FIOPAD_INDEX(FIOPAD_84_FUNC_OFFSET)
#define FIOPAD_AE43         (fpin_index)FIOPAD_INDEX(FIOPAD_85_FUNC_OFFSET)
#define FIOPAD_AA43         (fpin_index)FIOPAD_INDEX(FIOPAD_86_FUNC_OFFSET)
#define FIOPAD_AA45         (fpin_index)FIOPAD_INDEX(FIOPAD_87_FUNC_OFFSET)
#define FIOPAD_W45          (fpin_index)FIOPAD_INDEX(FIOPAD_88_FUNC_OFFSET)
#define FIOPAD_AA47         (fpin_index)FIOPAD_INDEX(FIOPAD_89_FUNC_OFFSET)
#define FIOPAD_U45          (fpin_index)FIOPAD_INDEX(FIOPAD_90_FUNC_OFFSET)
#define FIOPAD_G55          (fpin_index)FIOPAD_INDEX(FIOPAD_91_FUNC_OFFSET)
#define FIOPAD_J55          (fpin_index)FIOPAD_INDEX(FIOPAD_92_FUNC_OFFSET)
#define FIOPAD_L53          (fpin_index)FIOPAD_INDEX(FIOPAD_93_FUNC_OFFSET)
#define FIOPAD_C55          (fpin_index)FIOPAD_INDEX(FIOPAD_94_FUNC_OFFSET)
#define FIOPAD_E55          (fpin_index)FIOPAD_INDEX(FIOPAD_95_FUNC_OFFSET)
#define FIOPAD_J53          (fpin_index)FIOPAD_INDEX(FIOPAD_96_FUNC_OFFSET)
#define FIOPAD_L55          (fpin_index)FIOPAD_INDEX(FIOPAD_97_FUNC_OFFSET)
#define FIOPAD_N55          (fpin_index)FIOPAD_INDEX(FIOPAD_98_FUNC_OFFSET)
#define FIOPAD_C53          (fpin_index)FIOPAD_INDEX(FIOPAD_29_FUNC_OFFSET)
#define FIOPAD_E53          (fpin_index)FIOPAD_INDEX(FIOPAD_30_FUNC_OFFSET)
#define FIOPAD_E27          (fpin_index)FIOPAD_INDEX(FIOPAD_99_FUNC_OFFSET)
#define FIOPAD_G27          (fpin_index)FIOPAD_INDEX(FIOPAD_100_FUNC_OFFSET)
#define FIOPAD_N37          (fpin_index)FIOPAD_INDEX(FIOPAD_101_FUNC_OFFSET)
#define FIOPAD_N35          (fpin_index)FIOPAD_INDEX(FIOPAD_102_FUNC_OFFSET)
#define FIOPAD_J29          (fpin_index)FIOPAD_INDEX(FIOPAD_103_FUNC_OFFSET)
#define FIOPAD_N29          (fpin_index)FIOPAD_INDEX(FIOPAD_104_FUNC_OFFSET)
#define FIOPAD_L29          (fpin_index)FIOPAD_INDEX(FIOPAD_105_FUNC_OFFSET)
#define FIOPAD_N41          (fpin_index)FIOPAD_INDEX(FIOPAD_106_FUNC_OFFSET)
#define FIOPAD_N39          (fpin_index)FIOPAD_INDEX(FIOPAD_107_FUNC_OFFSET)
#define FIOPAD_L27          (fpin_index)FIOPAD_INDEX(FIOPAD_108_FUNC_OFFSET)
#define FIOPAD_J27          (fpin_index)FIOPAD_INDEX(FIOPAD_109_FUNC_OFFSET)
#define FIOPAD_J25          (fpin_index)FIOPAD_INDEX(FIOPAD_110_FUNC_OFFSET)
#define FIOPAD_E25          (fpin_index)FIOPAD_INDEX(FIOPAD_111_FUNC_OFFSET)
#define FIOPAD_G25          (fpin_index)FIOPAD_INDEX(FIOPAD_112_FUNC_OFFSET)
#define FIOPAD_N23          (fpin_index)FIOPAD_INDEX(FIOPAD_113_FUNC_OFFSET)
#define FIOPAD_L25          (fpin_index)FIOPAD_INDEX(FIOPAD_114_FUNC_OFFSET)
#define FIOPAD_J33          (fpin_index)FIOPAD_INDEX(FIOPAD_115_FUNC_OFFSET)
#define FIOPAD_J35          (fpin_index)FIOPAD_INDEX(FIOPAD_116_FUNC_OFFSET)
#define FIOPAD_G37          (fpin_index)FIOPAD_INDEX(FIOPAD_117_FUNC_OFFSET)
#define FIOPAD_E39          (fpin_index)FIOPAD_INDEX(FIOPAD_118_FUNC_OFFSET)
#define FIOPAD_L39          (fpin_index)FIOPAD_INDEX(FIOPAD_119_FUNC_OFFSET)
#define FIOPAD_C39          (fpin_index)FIOPAD_INDEX(FIOPAD_120_FUNC_OFFSET)
#define FIOPAD_E37          (fpin_index)FIOPAD_INDEX(FIOPAD_121_FUNC_OFFSET)
#define FIOPAD_L41          (fpin_index)FIOPAD_INDEX(FIOPAD_122_FUNC_OFFSET)
#define FIOPAD_J39          (fpin_index)FIOPAD_INDEX(FIOPAD_123_FUNC_OFFSET)
#define FIOPAD_J37          (fpin_index)FIOPAD_INDEX(FIOPAD_124_FUNC_OFFSET)
#define FIOPAD_L35          (fpin_index)FIOPAD_INDEX(FIOPAD_125_FUNC_OFFSET)
#define FIOPAD_E33          (fpin_index)FIOPAD_INDEX(FIOPAD_126_FUNC_OFFSET)
#define FIOPAD_E31          (fpin_index)FIOPAD_INDEX(FIOPAD_127_FUNC_OFFSET)
#define FIOPAD_G31          (fpin_index)FIOPAD_INDEX(FIOPAD_128_FUNC_OFFSET)
#define FIOPAD_J31          (fpin_index)FIOPAD_INDEX(FIOPAD_129_FUNC_OFFSET)
#define FIOPAD_L33          (fpin_index)FIOPAD_INDEX(FIOPAD_130_FUNC_OFFSET)
#define FIOPAD_N31          (fpin_index)FIOPAD_INDEX(FIOPAD_131_FUNC_OFFSET)
#define FIOPAD_R47          (fpin_index)FIOPAD_INDEX(FIOPAD_132_FUNC_OFFSET)
#define FIOPAD_R45          (fpin_index)FIOPAD_INDEX(FIOPAD_133_FUNC_OFFSET)
#define FIOPAD_N47          (fpin_index)FIOPAD_INDEX(FIOPAD_134_FUNC_OFFSET)
#define FIOPAD_N51          (fpin_index)FIOPAD_INDEX(FIOPAD_135_FUNC_OFFSET)
#define FIOPAD_L51          (fpin_index)FIOPAD_INDEX(FIOPAD_136_FUNC_OFFSET)
#define FIOPAD_J51          (fpin_index)FIOPAD_INDEX(FIOPAD_137_FUNC_OFFSET)
#define FIOPAD_J41          (fpin_index)FIOPAD_INDEX(FIOPAD_138_FUNC_OFFSET)
#define FIOPAD_E43          (fpin_index)FIOPAD_INDEX(FIOPAD_139_FUNC_OFFSET)
#define FIOPAD_G43          (fpin_index)FIOPAD_INDEX(FIOPAD_140_FUNC_OFFSET)
#define FIOPAD_J43          (fpin_index)FIOPAD_INDEX(FIOPAD_141_FUNC_OFFSET)
#define FIOPAD_J45          (fpin_index)FIOPAD_INDEX(FIOPAD_142_FUNC_OFFSET)
#define FIOPAD_N45          (fpin_index)FIOPAD_INDEX(FIOPAD_143_FUNC_OFFSET)
#define FIOPAD_L47          (fpin_index)FIOPAD_INDEX(FIOPAD_144_FUNC_OFFSET)
#define FIOPAD_L45          (fpin_index)FIOPAD_INDEX(FIOPAD_145_FUNC_OFFSET)
#define FIOPAD_N49          (fpin_index)FIOPAD_INDEX(FIOPAD_146_FUNC_OFFSET)
#define FIOPAD_J49          (fpin_index)FIOPAD_INDEX(FIOPAD_147_FUNC_OFFSET)

#define GET_REG32_BITS(x, a, b)                  (u32)((((u32)(x)) & GENMASK(a, b)) >> b)
#define SET_REG32_BITS(x, a, b)                  (u32)((((u32)(x)) << b) & GENMASK(a, b))
 
#define FIOPAD_X_REG0_PULL_MASK        GENMASK(9, 8)    /* 上下拉配置 */
#define FIOPAD_X_REG0_PULL_GET(x)      GET_REG32_BITS((x), 9, 8)
#define FIOPAD_X_REG0_PULL_SET(x)      SET_REG32_BITS((x), 9, 8)
 
#define FIOPAD_X_REG0_DRIVE_MASK       GENMASK(7, 4)    /* 驱动能力配置 */
#define FIOPAD_X_REG0_DRIVE_GET(x)     GET_REG32_BITS((x), 7, 4)  
#define FIOPAD_X_REG0_DRIVE_SET(x)     SET_REG32_BITS((x), 7, 4)  
 
#define FIOPAD_X_REG0_FUNC_MASK        GENMASK(2, 0)   /* 引脚复用配置 */
#define FIOPAD_X_REG0_FUNC_GET(x)      GET_REG32_BITS((x), 2, 0)  
#define FIOPAD_X_REG0_FUNC_SET(x)      SET_REG32_BITS((x), 2, 0)  

#define NAND_TEST_OFFSET 0      /* 起始的操作地址 */
#define NAND_TEST_LENGTH (2 * 0x80000) /*  操作的字节数 */
#define NAND_IMAGE_LENGTH 0x1000000
 

 static _INLINE u8 ftin8(unsigned long addr)
 {
     return *(volatile u8 *)addr;
 }

 static _INLINE u16 ftin16(unsigned long addr)
 {
     return *(volatile u16 *)addr;
 }

 static _INLINE u32 ftin32(unsigned long addr)
 {
     return *(volatile u32 *)addr;
 }

 static _INLINE u64 ftin64(unsigned long addr)
 {
     return *(volatile u64 *)addr;
 }

 static _INLINE void ftout8(unsigned long addr, u8 value)
 {
     volatile u8 *local_addr = (volatile u8 *)addr;
     *local_addr = value;
 }

 static _INLINE void ftout16(unsigned long addr, u16 value)
 {
     volatile u16 *local_addr = (volatile u16 *)addr;
     *local_addr = value;
 }

 static _INLINE void ftout32(unsigned long addr, u32 value)
 {
     volatile u32 *local_addr = (volatile u32 *)addr;
     *local_addr = value;
 }

 static _INLINE void ftout64(unsigned long addr, u64 value)
 {
     volatile u64 *local_addr = (volatile u64 *)addr;
     *local_addr = value;
 }

 static _INLINE void ftsetbit32(unsigned long addr, u32 value)
 {
     volatile u32 last_value;
     last_value = ftin32(addr);
     last_value |= value;
     ftout32(addr, last_value);
 }

 static _INLINE void ftclearbit32(unsigned long addr, u32 value)
 {
     volatile u32 last_value;
     last_value = ftin32(addr);
     last_value &= ~value;
     ftout32(addr, last_value);
 }

 static _INLINE void fttogglebit32(unsigned long addr, u32 toggle_pos)
 {
     volatile u32 value;
     value = ftin32(addr);
     value ^= (1 << toggle_pos);
     ftout32(addr, value);
 }

 static _INLINE u16 ftendianswap16(u16 data)
 {
     return (u16)(((data & 0xFF00U) >> 8U) | ((data & 0x00FFU) << 8U));
 }
#define FT_WRITE32(_reg, _val) (*(volatile uint32_t *)&_reg = _val)
#define FT_READ32(_reg) (*(volatile uint32_t *)&_reg)	
	
/**
*
* This macro reads the given register.
*
* @param	base_addr is the base address of the device.
* @param	reg_offset is the register offset to be read.
*
* @return	The 32-bit value of the register
*
* @note 	None.
*
*****************************************************************************/
#define ftnand_readreg(base_addr, reg_offset) \
		ftin32((base_addr) + (u32)(reg_offset))

/****************************************************************************/
/**
*
* This macro writes the given register.
*
* @param	base_addr is the base address of the device.
* @param	reg_offset is the register offset to be written.
* @param	data is the 32-bit value to write to the register.
*
* @return	None.
*
* @note 	None.
*
*****************************************************************************/
#define ftnand_writereg(base_addr, reg_offset, data) \
		ftout32((base_addr) + (u32)(reg_offset), (u32)(data))
	
#define ftnand_setbit(base_addr, reg_offset, data) \
		ftsetbit32((base_addr) + (u32)(reg_offset), (u32)(data))
	
#define ftnand_clearbit(base_addr, reg_offset, data) \
		ftclearbit32((base_addr) + (u32)(reg_offset), (u32)(data))


struct jedec_ecc_info {
	u8 ecc_bits;
	u8 codeword_size;
	u16 bb_per_lun;
	u16 block_endurance;
	u8 reserved[2];
} __attribute__((packed));
	
struct toggle_nand_geometry
{
	u8 sig[4];			/* Parameter page signature */
	u16 revision;		/* Revision number */
	u16 features;		/* Features supported */
	u8 opt_cmd[3];		/* Optional commands supported */
	u16 sec_cmd;
	u8 num_of_param_pages;
	u8 reserved0[18];

	/* manufacturer information block */
	char manufacturer[12];	/* Device manufacturer */
	char model[20]; 		/* Device model */
	u8 jedec_id[6]; 		/* JEDEC manufacturer ID */
	u8 reserved1[10];		

	/* memory organization block */
	u32 byte_per_page;		/* Number of data bytes per page */
	u16 spare_bytes_per_page; /* Number of spare bytes per page */
	u8 reserved2[6];		  /*  */
	u32 pages_per_block;	  /* Number of pages per block */
	u32 blocks_per_lun; 	  /* Number of blocks per logical unit */
	u8 lun_count;			  /* Number of logical unit */
	u8 addr_cycles;
	u8 bits_per_cell;
	u8 programs_per_page;
	u8 multi_plane_addr;
	u8 multi_plane_op_attr;
	u8 reserved3[38];

	/* electrical parameter block */
	u16 async_sdr_speed_grade;
	u16 toggle_ddr_speed_grade;
	u16 sync_ddr_speed_grade;
	u8 async_sdr_features;
	u8 toggle_ddr_features;
	u8 sync_ddr_features;
	u16 t_prog;
	u16 t_bers;
	u16 t_r;
	u16 t_r_multi_plane;
	u16 t_ccs;
	u16 io_pin_capacitance_typ;
	u16 input_pin_capacitance_typ;
	u16 clk_pin_capacitance_typ;
	u8 driver_strength_support;
	u16 t_adl;
	u8 reserved4[36];

	/* ECC and endurance block */
	u8 guaranteed_good_blocks;
	u16 guaranteed_block_endurance;
	struct jedec_ecc_info ecc_info[4];
	u8 reserved5[29];

	/* reserved */
	u8 reserved6[148];

	/* vendor */
	u16 vendor_rev_num;
	u8 reserved7[88];

	/* CRC for Parameter Page */
	u16 crc;

} __attribute__((__packed__));

struct cmd_ctrl
{
	u16 csel : 4;	  /* 每一位表示选择NAND FLASH 设备  */
	u16 dbc : 1;	  /* 表示是否有2级命令，1表示有，只有此位为1时，描述符表的CMD1才有效 */
	u16 addr_cyc : 3; /* 表示指令有几个周期，‘b000’:表示没有周期 ‘b001’:表示1一个地址周期，一次类推 */
	u16 nc : 1;		  /* 表示是否有连续的下一个指令，一般多页操作需要连续发送多个指令 */
	u16 cmd_type : 4; /* 表示命令类型 */
	u16 dc : 1;		  /* 表示命令发送是否包含有数据周期，有数据此位为1 */
	u16 auto_rs : 1;  /* 表示命令发送完成后是否检测闪存状态 */
	u16 ecc_en : 1;	  /* ECC 数据发送和读取使能位，位1 表示该命令仅发送或者读取ECC数据，当读命令该位使能位1后，控制器会对上一次数据进行ECC 校验，并返回结果 */
};

struct ftnand_dma_descriptor
{
	u8 cmd0; /* NAND FLASH 第一个命令编码 */
	u8 cmd1; /* NAND FLASH 第二个命令编码 */
	union
	{
		u16 ctrl;
		struct cmd_ctrl nfc_ctrl;
	} cmd_ctrl; /* 16位命令字 */
	u8 addr[FNAND_NFC_ADDR_MAX_LEN];
	u16 page_cnt;
	u8 mem_addr_first[FNAND_NFC_ADDR_MAX_LEN];

} __attribute__((packed)) __attribute__((aligned(128)));



typedef struct
   {
       u8 *addr_p; /* Address  */
       u32 addr_length;
       unsigned long phy_address;
       u32 phy_bytes_length;
       u32 chip_addr;
       u8 contiune_dma; /*  */
   } ftnand_dma_pack_data;


/* DMA format */
typedef struct
{
    int start_cmd;  /* Start command */
    int end_cmd;    /* End command */
    u8 addr_cycles; /* Number of address cycles */
    u8 cmd_type;    /* Presentation command type ,followed by FNAND_CMDCTRL_XXXX */
    u8 ecc_en;      /* Hardware ecc open */
    u8 auto_rs;     /* 表示命令发送完成后是否检测闪存状态 */
} ftnand_cmd_format;


typedef enum
{
    FNAND_ASYNC_TIM_INT_MODE0 = 0,
    FNAND_ASYNC_TIM_INT_MODE1,
    FNAND_ASYNC_TIM_INT_MODE2,
    FNAND_ASYNC_TIM_INT_MODE3,
    FNAND_ASYNC_TIM_INT_MODE4,
} ftnand_async_tim_int;

typedef enum
{
    FNAND_CMD_TYPE = 0 ,    /* 采用cmd 类型的操作类型 */
    FNAND_WRITE_PAGE_TYPE  ,   /* PAGE program 操作 */
    FNAND_READ_PAGE_TYPE  ,   /* PAGE read 操作 */
    FNAND_WAIT_ECC_TYPE, /* Waiting ECC FINISH 操作 */
    FNAND_TYPE_NUM
} ftnand_operation_type;

/* Irq Callback events */
typedef enum
{
    FNAND_IRQ_BUSY_EVENT = 0,/* nandflash控制器忙状态中断状态位 */
    FNAND_IRQ_DMA_BUSY_EVENT ,/* dma控制器忙状态中断状态位 */
    FNAND_IRQ_DMA_PGFINISH_EVENT ,/* dma页操作完成中断状态位 */
    FNAND_IRQ_DMA_FINISH_EVENT ,/* dma操作完成中断完成中断状态位 */
    FNAND_IRQ_FIFO_EMP_EVENT ,/* fifo为空中断状态位 */
    FNAND_IRQ_FIFO_FULL_EVENT ,/* fifo为满中断状态位 */
    FNAND_IRQ_FIFO_TIMEOUT_EVENT ,/* fifo超时中断状态位 */
    FNAND_IRQ_CMD_FINISH_EVENT ,/* nand接口命令完成中断状态位 */
    FNAND_IRQ_PGFINISH_EVENT ,/* nand接口页操作完成中断状态位 */
    FNAND_IRQ_RE_EVENT ,/* re_n门控打开中断状态位 */
    FNAND_IRQ_DQS_EVENT ,/* dqs门控打开中断状态位 */
    FNAND_IRQ_RB_EVENT ,/* rb_n信号busy中断状态位 */
    FNAND_IRQ_ECC_FINISH_EVENT ,/* ecc完成中断状态蔽位 */
    FNAND_IRQ_ECC_ERR_EVENT /* ecc正确中断状态蔽位 */
} ftnand_call_back_event;

typedef struct
{
    u32 bytes_per_page;       /* Bytes per page */
    u32 spare_bytes_per_page; /* Size of spare area in bytes */
    u32 pages_per_block;      /* Pages per block */
    u32 blocks_per_lun;       /* Bocks per LUN */
    u8 num_lun;               /* Total number of LUN */
    u8 flash_width;           /* Data width of flash device */
    u64 num_pages;            /* Total number of pages in device */
    u64 num_blocks;           /* Total number of blocks in device */
    u64 block_size;           /* Size of a block in bytes */
    u64 device_size;          /* Total device size in bytes */
    u8 rowaddr_cycles;        /* Row address cycles */
    u8 coladdr_cycles;        /* Column address cycles */
    u32 hw_ecc_steps;         /* number of ECC steps per page */
    u32 hw_ecc_length;        /* 产生硬件ecc校验参数的个数 */
    u32 ecc_offset;           /* spare_bytes_per_page - hw_ecc_length = obb存放硬件ecc校验参数页位置的偏移 */
    u32 ecc_step_size;        /* 进行读写操作时，单次ecc 的步骤的跨度 */
} ftnand_nand_geometry;

 typedef enum
 {
     FNAND_ASYN_SDR = 0,    /* ONFI & Toggle async */
     FNAND_ONFI_DDR = 1,    /* ONFI sync */
     FNAND_TOG_ASYN_DDR = 2 /* Toggle async */
 } ftnand_inter_mode;

 typedef enum
 {
     FNAND_TIMING_MODE0 = 0,
     FNAND_TIMING_MODE1 = 1,
     FNAND_TIMING_MODE2 = 2,
     FNAND_TIMING_MODE3 = 3,
     FNAND_TIMING_MODE4 = 4,
     FNAND_TIMING_MODE5 = 5,
 } ftnand_timing_mode;

 typedef struct
 {
     u32 instance_id; /* Id of device*/
     u32 irq_num;     /* Irq number */
     volatile unsigned long base_address;
     u32 ecc_strength; /* 每次ecc 步骤纠正的位数 */
     u32 ecc_step_size; /* 进行读写操作时，单次ecc 的步骤的跨度 */
 } ftnand_config;

 /**
  * Bad block pattern
  */
 typedef struct
 {
     u32 options;   /**< Options to search the bad block pattern */
     u32 offset;    /**< Offset to search for specified pattern */
     u32 length;    /**< Number of bytes to check the pattern */
     u8 pattern[2]; /**< Pattern format to search for */
 } ftnand_bad_block_pattern;


 typedef struct
 {
     u32 page_addr;
     u8* page_buf; /* page 数据缓存空间 */
     u32 page_offset; /* 从offset开始拷贝页数据 */
     u32 page_length; /* 从offset开始拷贝页数据的长度 */
     int obb_required; /* obb 是否读取的标志位,1 需要操作oob 区域 */
     u8* oob_buf; /* obb 数据缓存空间 */
     u32 oob_offset; /* 从offset开始拷贝页数据 */
     u32 oob_length; /* 从offset开始拷贝页数据的长度 */
     u32 chip_addr; /* 芯片地址 */
 }ftnand_op_data; 

/**
 * Bad block table descriptor
 */
typedef struct
{
    u32 page_offset;   /* Page offset where BBT resides */
    u32 sig_offset;    /* Signature offset in Spare area */
    u32 ver_offset;    /* Offset of BBT version */
    u32 sig_length;    /* Length of the signature */
    u32 max_blocks;    /* Max blocks to search for BBT */
    char signature[4]; /* BBT signature */
    u8 version;        /* BBT version */
    u32 valid;         /* BBT descriptor is valid or not */
} ftnand_bbt_desc;

typedef struct
{
    u8 bbt[FNAND_MAX_BLOCKS >> 2];
    ftnand_bbt_desc bbt_desc;           /* Bad block table descriptor */
    ftnand_bbt_desc bbt_mirror_desc;    /* Mirror BBT descriptor */
    ftnand_bad_block_pattern bb_pattern; /* Bad block pattern to search */
} ftnand_bad_block_manager;

  /* DMA */

  /* DMA buffer  */
struct ftnand_dma_buffer
{
    u8 data_buffer[FNAND_DMA_MAX_LENGTH];
} __attribute__((packed)) __attribute__((aligned(128)));

/* operation api */
typedef struct _ftnand ftnand;

typedef u32 (*FNandTransferP)(ftnand *instance_p, ftnand_op_data *op_data_p);
typedef u32 (*FNandEraseP)(ftnand *instance_p, u32 page_address,u32 chip_addr);

typedef void (*FnandIrqEventHandler)(void *args, ftnand_call_back_event event);
typedef u32 (*FNandOperationWaitIrqCallback)(void *args);


typedef struct
{
    u8 data[8];
    u32 len;

}ftnand_id;

struct ftnand_sdr_timings {
	u64 tBERS_max;
	u32 tCCS_min;
	u64 tPROG_max;
	u64 tR_max;
	u32 tALH_min;
	u32 tADL_min;
	u32 tALS_min;
	u32 tAR_min;
	u32 tCEA_max;
	u32 tCEH_min;
	u32 tCH_min;
	u32 tCHZ_max;
	u32 tCLH_min;
	u32 tCLR_min;
	u32 tCLS_min;
	u32 tCOH_min;
	u32 tCS_min;
	u32 tDH_min;
	u32 tDS_min;
	u32 tFEAT_max;
	u32 tIR_min;
	u32 tITC_max;
	u32 tRC_min;
	u32 tREA_max;
	u32 tREH_min;
	u32 tRHOH_min;
	u32 tRHW_min;
	u32 tRHZ_max;
	u32 tRLOH_min;
	u32 tRP_min;
	u32 tRR_min;
	u64 tRST_max;
	u32 tWB_max;
	u32 tWC_min;
	u32 tWH_min;
	u32 tWHR_min;
	u32 tWP_min;
	u32 tWW_min;
};

typedef struct _ftnand
{
    u32 is_ready; /* Device is ininitialized and ready*/
    ftnand_config config;
    u32 work_mode;              /* NAND controler work mode */

    /* nand flash info */
    ftnand_inter_mode inter_mode[FNAND_CONNECT_MAX_NUM]; /* NAND controler timing work mode */
    ftnand_timing_mode timing_mode[FNAND_CONNECT_MAX_NUM];
    u32 nand_flash_interface[FNAND_CONNECT_MAX_NUM] ; /* Nand Flash Interface , followed by FNAND_ONFI_MODE \ FNAND_TOGGLE_MODE*/

    struct ftnand_dma_buffer dma_data_buffer;          /* DMA data buffer */
    struct ftnand_dma_buffer descriptor_buffer;        /* DMA descriptor */
    struct ftnand_dma_descriptor descriptor[2];        /* DMA descriptor */
    struct ftnand_sdr_timings sdr_timing;              /* SDR NAND chip timings */

    /* bbm */
    ftnand_bad_block_manager bbt_manager[FNAND_CONNECT_MAX_NUM];    /* bad block manager handler */
    /* nand detect */
    ftnand_nand_geometry nand_geometry[FNAND_CONNECT_MAX_NUM];     /* nand flash infomation */
    /* dma 页操作 */
    FnandIrqEventHandler irq_event_fun_p;                       /* Interrupt event response function */
    void *irq_args;

    FNandOperationWaitIrqCallback wait_irq_fun_p;               /* The NAND controller operates the wait function */
    void *wait_args;        

    /* operations */
    FNandTransferP write_p ;                                    /* Write page function */
    FNandTransferP read_p ;                                     /* Read page function */
    FNandTransferP write_oob_p ;                                /* Write page spare space function */
    FNandTransferP read_oob_p ;                                 /* Read page spare space function */
    FNandTransferP write_hw_ecc_p ;                             /* Write page with hardware function */
    FNandTransferP read_hw_ecc_p ;                              /* Read page with hardware function */
    FNandEraseP erase_p;                                        /* Erase block  function */
} ftnand;




struct ftnand_manufacturer_ops {
	u32 (*detect)(ftnand *instance_p, ftnand_id *id_p,u32 chip_addr); /* detect chip */
	int (*init)(ftnand *instance_p,u32 chip_addr);
	void (*cleanup)(ftnand *instance_p,u32 chip_addr);
};

struct onfi_nand_geometry {
	/* rev info and features block */
	/* 'O' 'N' 'F' 'I'  */
	u8 sig[4];
	u16 revision;
	u16 features;
	u16 opt_cmd;
	u8 reserved0[2];
	u16 ext_param_page_length; /* since ONFI 2.1 */
	u8 num_of_param_pages;        /* since ONFI 2.1 */
	u8 reserved1[17];

	/* manufacturer information block */
	char manufacturer[12];
	char model[20];
	u8 jedec_id;
	u16 date_code;
	u8 reserved2[13];

	/* memory organization block */
	u32 byte_per_page;
	u16 spare_bytes_per_page;
	u32 data_bytes_per_ppage;
	u16 spare_bytes_per_ppage;
	u32 pages_per_block;
	u32 blocks_per_lun;
	u8 lun_count;
	u8 addr_cycles;
	u8 bits_per_cell;
	u16 bb_per_lun;
	u16 block_endurance;
	u8 guaranteed_good_blocks;
	u16 guaranteed_block_endurance;
	u8 programs_per_page;
	u8 ppage_attr;
	u8 ecc_bits;
	u8 interleaved_bits;
	u8 interleaved_ops;
	u8 reserved3[13];

	/* electrical parameter block */
	u8 io_pin_capacitance_max;
	u16 async_timing_mode;
	u16 program_cache_timing_mode;
	u16 t_prog;
	u16 t_bers;
	u16 t_r;
	u16 t_ccs;
	u16 src_sync_timing_mode;
	u8 src_ssync_features;
	u16 clk_pin_capacitance_typ;
	u16 io_pin_capacitance_typ;
	u16 input_pin_capacitance_typ;
	u8 input_pin_capacitance_max;
	u8 driver_strength_support;
	u16 t_int_r;
	u16 t_adl;
	u8 reserved4[8];

	/* vendor */
	u16 vendor_revision;
	u8 vendor[88];

	u16 crc;
} __attribute__((__packed__));

typedef struct  {
	int id;
	char *name;
	const struct ftnand_manufacturer_ops *ops;
}ftnand_manufacturer;

typedef struct
{
    u32 reg_off; /* 引脚配置寄存器偏移量 */
    u32 reg_bit; /* 引脚配置起始位 */
} fpin_index; /* 引脚索引 */

typedef enum
{
    FPIN_FUNC0 = 0b000,
    FPIN_FUNC1,
    FPIN_FUNC2,
    FPIN_FUNC3 = 0b011,
    FPIN_FUNC4,
    FPIN_FUNC5,
    FPIN_FUNC6,
    FPIN_FUNC7 = 0b111,
    FPIN_NUM_OF_FUNC
} fpin_func; /* 引脚复用功能配置, func0为默认功能 */

typedef enum
{
    FPIN_PULL_NONE = 0b00,
    FPIN_PULL_DOWN = 0b01,
    FPIN_PULL_UP = 0b10,

    FPIN_NUM_OF_PULL
} fpin_pull; /* 引脚上下拉配置 */

typedef enum
{
    FPIN_DRV0 = 0b0000,
    FPIN_DRV1,
    FPIN_DRV2,
    FPIN_DRV3,
    FPIN_DRV4,
    FPIN_DRV5,
    FPIN_DRV6,
    FPIN_DRV7,
    FPIN_DRV8,
    FPIN_DRV9,
    FPIN_DRV10,
    FPIN_DRV11,
    FPIN_DRV12,
    FPIN_DRV13,
    FPIN_DRV14,
    FPIN_DRV15 = 0b1111,

    FPIN_NUM_OF_DRIVE
} fpin_drive; /* 引脚驱动能力配置 */

enum commands_enum_new
{
    CMD_READ_OPTION_NEW = 0 ,
    CMD_RANDOM_DATA_OUT ,
    CMD_PAGE_PROGRAM ,
    CMD_PAGE_PROGRAM_WITH_OTHER ,
    CMD_COPY_BACK_PROGRAM ,
    CMD_BLOCK_ERASE,
    CMD_RESET,
    CMD_READ_ID ,
    CMD_READ_DEVICE_TABLE ,
    CMD_READ_PAGE,
    CMD_READ_STATUS,  
    CMD_INDEX_LENGTH_NEW,
};

static const struct ftnand_sdr_timings onfi_sdr_timings[] =
    {
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 20000,
            .tALS_min = 50000,
            .tAR_min = 25000,
            .tCEA_max = 100000,
            .tCEH_min = 20000,
            .tCH_min = 20000,
            .tCHZ_max = 100000,
            .tCLH_min = 20000,
            .tCLR_min = 20000,
            .tCLS_min = 50000,
            .tCOH_min = 0,
            .tCS_min = 70000,
            .tDH_min = 20000,
            .tDS_min = 40000,
            .tFEAT_max = 1000000,
            .tIR_min = 10000,
            .tITC_max = 1000000,
            .tRC_min = 100000,
            .tREA_max = 40000,
            .tREH_min = 30000,
            .tRHOH_min = 0,
            .tRHW_min = 200000,
            .tRHZ_max = 200000,
            .tRLOH_min = 0,
            .tRP_min = 50000,
            .tRR_min = 40000,
            .tRST_max = 250000000000ULL,
            .tWB_max = 200000,
            .tWC_min = 100000,
            .tWH_min = 30000,
            .tWHR_min = 120000,
            .tWP_min = 50000,
            .tWW_min = 100000,
        },

        /* Mode 1 */
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 10000,
            .tALS_min = 25000,
            .tAR_min = 10000,
            .tCEA_max = 45000,
            .tCEH_min = 20000,
            .tCH_min = 10000,
            .tCHZ_max = 50000,
            .tCLH_min = 10000,
            .tCLR_min = 10000,
            .tCLS_min = 25000,
            .tCOH_min = 15000,
            .tCS_min = 35000,
            .tDH_min = 10000,
            .tDS_min = 20000,
            .tFEAT_max = 1000000,
            .tIR_min = 0,
            .tITC_max = 1000000,
            .tRC_min = 50000,
            .tREA_max = 30000,
            .tREH_min = 15000,
            .tRHOH_min = 15000,
            .tRHW_min = 100000,
            .tRHZ_max = 100000,
            .tRLOH_min = 0,
            .tRP_min = 25000,
            .tRR_min = 20000,
            .tRST_max = 500000000,
            .tWB_max = 100000,
            .tWC_min = 45000,
            .tWH_min = 15000,
            .tWHR_min = 80000,
            .tWP_min = 25000,
            .tWW_min = 100000,
        },

        /* Mode 2 */
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 10000,
            .tALS_min = 15000,
            .tAR_min = 10000,
            .tCEA_max = 30000,
            .tCEH_min = 20000,
            .tCH_min = 10000,
            .tCHZ_max = 50000,
            .tCLH_min = 10000,
            .tCLR_min = 10000,
            .tCLS_min = 15000,
            .tCOH_min = 15000,
            .tCS_min = 25000,
            .tDH_min = 5000,
            .tDS_min = 15000,
            .tFEAT_max = 1000000,
            .tIR_min = 0,
            .tITC_max = 1000000,
            .tRC_min = 35000,
            .tREA_max = 25000,
            .tREH_min = 15000,
            .tRHOH_min = 15000,
            .tRHW_min = 100000,
            .tRHZ_max = 100000,
            .tRLOH_min = 0,
            .tRR_min = 20000,
            .tRST_max = 500000000,
            .tWB_max = 100000,
            .tRP_min = 17000,
            .tWC_min = 35000,
            .tWH_min = 15000,
            .tWHR_min = 80000,
            .tWP_min = 17000,
            .tWW_min = 100000,
        },

        /* Mode 3 */
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 5000,
            .tALS_min = 10000,
            .tAR_min = 10000,
            .tCEA_max = 25000,
            .tCEH_min = 20000,
            .tCH_min = 5000,
            .tCHZ_max = 50000,
            .tCLH_min = 5000,
            .tCLR_min = 10000,
            .tCLS_min = 10000,
            .tCOH_min = 15000,
            .tCS_min = 25000,
            .tDH_min = 5000,
            .tDS_min = 10000,
            .tFEAT_max = 1000000,
            .tIR_min = 0,
            .tITC_max = 1000000,
            .tRC_min = 30000,
            .tREA_max = 20000,
            .tREH_min = 10000,
            .tRHOH_min = 15000,
            .tRHW_min = 100000,
            .tRHZ_max = 100000,
            .tRLOH_min = 0,
            .tRP_min = 15000,
            .tRR_min = 20000,
            .tRST_max = 500000000,
            .tWB_max = 100000,
            .tWC_min = 30000,
            .tWH_min = 10000,
            .tWHR_min = 80000,
            .tWP_min = 15000,
            .tWW_min = 100000,
        },
        /* Mode 4 */
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 5000,
            .tALS_min = 10000,
            .tAR_min = 10000,
            .tCEA_max = 25000,
            .tCEH_min = 20000,
            .tCH_min = 5000,
            .tCHZ_max = 30000,
            .tCLH_min = 5000,
            .tCLR_min = 10000,
            .tCLS_min = 10000,
            .tCOH_min = 15000,
            .tCS_min = 20000,
            .tDH_min = 5000,
            .tDS_min = 10000,
            .tFEAT_max = 1000000,
            .tIR_min = 0,
            .tITC_max = 1000000,
            .tRC_min = 25000,
            .tREA_max = 20000,
            .tREH_min = 10000,
            .tRHOH_min = 15000,
            .tRHW_min = 100000,
            .tRHZ_max = 100000,
            .tRLOH_min = 5000,
            .tRP_min = 12000,
            .tRR_min = 20000,
            .tRST_max = 500000000,
            .tWB_max = 100000,
            .tWC_min = 25000,
            .tWH_min = 10000,
            .tWHR_min = 80000,
            .tWP_min = 12000,
            .tWW_min = 100000,
        },
        /* Mode 5 */
        {
            .tCCS_min = 500000,
            .tR_max = 200000000,
            .tADL_min = 400000,
            .tALH_min = 5000,
            .tALS_min = 10000,
            .tAR_min = 10000,
            .tCEA_max = 25000,
            .tCEH_min = 20000,
            .tCH_min = 5000,
            .tCHZ_max = 30000,
            .tCLH_min = 5000,
            .tCLR_min = 10000,
            .tCLS_min = 10000,
            .tCOH_min = 15000,
            .tCS_min = 15000,
            .tDH_min = 5000,
            .tDS_min = 7000,
            .tFEAT_max = 1000000,
            .tIR_min = 0,
            .tITC_max = 1000000,
            .tRC_min = 20000,
            .tREA_max = 16000,
            .tREH_min = 7000,
            .tRHOH_min = 15000,
            .tRHW_min = 100000,
            .tRHZ_max = 100000,
            .tRLOH_min = 5000,
            .tRP_min = 10000,
            .tRR_min = 20000,
            .tRST_max = 500000000,
            .tWB_max = 100000,
            .tWC_min = 20000,
            .tWH_min = 7000,
            .tWHR_min = 80000,
            .tWP_min = 10000,
            .tWW_min = 100000,
        },
};

u32 ftnand_flash_reset(ftnand *instance_p,u32 chip_addr) ;
u32 ftnand_flash_read_id(ftnand *instance_p, u8 address, u8 *id_buffer, u32 buffer_length, u32 chip_addr);
u32 ftnand_send_cmd(ftnand *instance_p, struct ftnand_dma_descriptor *descriptor_p,ftnand_operation_type isr_type);
u32 ftnand_erase_block(ftnand *instance_p,u32 block,u32 chip_addr);
u32 ftnand_write_page(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset ,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr);
u32 ftnand_read_page_oob(ftnand *instance_p,u32 page_addr,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr);
u32 ftnand_read_page(ftnand *instance_p,u32 page_addr,u8 *buffer,u32 page_copy_offset,u32 length,u8 *oob_buffer,u32 oob_copy_offset,u32 oob_length,u32 chip_addr);

#endif
