// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium E2000 MCI driver
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#include <common.h>
#include <clk.h>
#include <errno.h>
#include <malloc.h>
#include <mmc.h>
#include <linux/delay.h>
#include <memalign.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>

//#undef  debug
//#define debug printf

#ifdef CONFIG_DM_MMC
#include <dm.h>
#endif

#define MMC_CLOCK_MAX   52000000
#define MMC_CLOCK_MIN   400000
#define SDCI_CLOCK_DIV_DEFAELT  749
#define SDCI_TIMEOUT_CMD_VALUE  1000000
#define SDCI_TIMEOUT_DATA_VALUE 2000000

/* SDCI CMD_SETTING_REG */
#define SDCI_CMD_ADTC_MASK          (0x00008000)    /* ADTC 类型指令*/
#define SDCI_CMD_CICE_MASK          (0x00000010)    /* 命令响应执行索引检查 */
#define SDCI_CMD_CRCE_MASK          (0x00000008)    /* 命令响应执行crc检查 */
#define SDCI_CMD_RES_NONE_MASK      (0x00000000)    /* 无响应 */
#define SDCI_CMD_RES_LONG_MASK      (0x00000001)    /* 长响应 */
#define SDCI_CMD_RES_SHORT_MASK     (0x00000002)    /* 短响应 */

#define SDCI_CMD_INDEX_MASK     (0x00003F00)
#define SDCI_CMD_INDEX_SHIFT    (8)

struct phytium_sdci_host {
    void __iomem *base;
    char name[32];
    unsigned int b_max;
    unsigned int voltages;
    unsigned int caps;
    unsigned int clock_in;
    unsigned int clock_min;
    unsigned int clock_max;
    unsigned int clkdiv_init;
    int version2;
    struct mmc_config cfg;
    unsigned int *dma_buf;
    u64 bd_dma;
};

#define MCI_BUS_1BITS          0x0
#define MCI_BUS_4BITS          0x1
#define MCI_BUS_8BITS          (0x1 << 16)

/* FIFOTH register defines */
#define MCI_SET_FIFOTH(m, r, t) (((m) & 0x7) << 28 | \
                     ((r) & 0xFFF) << 16 | ((t) & 0xFFF))

/*-----------------------------E2000 DEBUG------------------------------*/
/*----------------------------------------------------------------------*/
/* Register Offset                          */
/*----------------------------------------------------------------------*/
#define MCI_CNTRL       0x00 /* the controller config reg */
#define MCI_PWREN       0x04 /* the power enable reg */
#define MCI_CLKDIV      0x08 /* the clock divider reg */
#define MCI_CLKENA      0x10 /* the clock enable reg */
#define MCI_TMOUT       0x14 /* the timeout reg */
#define MCI_CTYPE       0x18 /* the card type reg */
#define MCI_BLKSIZ      0x1C /* the block size reg */
#define MCI_BYTCNT      0x20 /* the byte count reg */
#define MCI_INT_MASK    0x24 /* the interrupt mask reg */
#define MCI_CMDARG      0x28 /* the command argument reg */
#define MCI_CMD         0x2C /* the command reg */
#define MCI_RESP0       0x30 /* the response reg0 */
#define MCI_RESP1       0x34 /* the response reg1 */
#define MCI_RESP2       0x38 /* the response reg2 */
#define MCI_RESP3       0X3C /* the response reg3 */
#define MCI_MASKED_INTS 0x40 /* the masked interrupt status reg */
#define MCI_RAW_INTS    0x44 /* the raw interrupt status reg */
#define MCI_STATUS      0x48 /* the status reg  */
#define MCI_FIFOTH      0x4C /* the FIFO threshold watermark reg */
#define MCI_CARD_DETECT 0x50 /* the card detect reg */
#define MCI_CARD_WRTPRT 0x54 /* the card write protect reg */
#define MCI_CCLK_RDY        0x58 /* first div is ready? 1:ready,0:not ready*/
#define MCI_TRAN_CARD_CNT   0x5C /* the transferred CIU card byte count reg */
#define MCI_TRAN_FIFO_CNT   0x60 /* the transferred host to FIFO byte count reg  */
#define MCI_DEBNCE      0x64 /* the debounce count reg */
#define MCI_UID         0x68 /* the user ID reg */
#define MCI_VID         0x6C /* the controller version ID reg */
#define MCI_HWCONF      0x70 /* the hardware configuration reg */
#define MCI_UHS_REG     0x74 /* the UHS-I reg */
#define MCI_CARD_RESET  0x78 /* the card reset reg */
#define MCI_BUS_MODE    0x80 /* the bus mode reg */
#define MCI_DESC_LIST_ADDRL 0x88 /* the descriptor list low base address reg */
#define MCI_DESC_LIST_ADDRH 0x8C /* the descriptor list high base address reg */
#define MCI_DMAC_STATUS 0x90 /* the internal DMAC status reg */
#define MCI_DMAC_INT_ENA    0x94 /* the internal DMAC interrupt enable reg */
#define MCI_CUR_DESC_ADDRL  0x98 /* the current host descriptor low address reg */
#define MCI_CUR_DESC_ADDRH  0x9C /* the current host descriptor high address reg */
#define MCI_CUR_BUF_ADDRL   0xA0 /* the current buffer low address reg */
#define MCI_CUR_BUF_ADDRH   0xA4 /* the current buffer high address reg */
#define MCI_CARD_THRCTL 0x100 /* the card threshold control reg */
#define MCI_UHS_REG_EXT 0x108 /* the UHS register extension */
#define MCI_EMMC_DDR_REG    0x10C /* the EMMC DDR reg */
#define MCI_ENABLE_SHIFT    0x110 /* the enable phase shift reg */
#define MCI_DATA        0x200 /* the data FIFO access */

/* Command register defines */
#define MCI_CMD_START               BIT(31)
#define MCI_CMD_USE_HOLD_REG        BIT(29)
#define MCI_CMD_VOLT_SWITCH         BIT(28)
#define MCI_CMD_CCS_EXP             BIT(23)
#define MCI_CMD_CEATA_RD            BIT(22)
#define MCI_CMD_UPD_CLK             BIT(21)
#define MCI_CMD_INIT                BIT(15)
#define MCI_CMD_STOP                BIT(14)
#define MCI_CMD_PRV_DAT_WAIT        BIT(13)
#define MCI_CMD_SEND_STOP           BIT(12)
#define MCI_CMD_STRM_MODE           BIT(11)
#define MCI_CMD_DAT_WR              BIT(10)
#define MCI_CMD_DAT_EXP             BIT(9)
#define MCI_CMD_RESP_CRC            BIT(8)
#define MCI_CMD_RESP_LONG           BIT(7)
#define MCI_CMD_RESP_EXP            BIT(6)
#define MCI_CMD_INDX(n)             ((n) & 0x1F)


/*------------------------------------------------------*/
/* Register Mask                    */
/*------------------------------------------------------*/
/* MCI_CNTRL mask */
#define MCI_CNTRL_CONTROLLER_RESET  (0x1 << 0) /* RW */
#define MCI_CNTRL_FIFO_RESET        (0x1 << 1) /* RW */
#define MCI_CNTRL_DMA_RESET         (0x1 << 2) /* RW */
#define MCI_CNTRL_RES               (0x1 << 3) /*  */
#define MCI_CNTRL_INT_ENABLE        (0x1 << 4) /* RW */
#define MCI_CNTRL_DMA_ENABLE        (0x1 << 5) /* RW */
#define MCI_CNTRL_READ_WAIT         (0x1 << 6) /* RW */
#define MCI_CNTRL_SEND_IRQ_RESPONSE (0x1 << 7) /* RW */
#define MCI_CNTRL_ABORT_READ_DATA   (0x1 << 8) /* RW */
#define MCI_CNTRL_ENDIAN            (0x1 << 11) /* RW */
//#define MCI_CNTRL_CARD_VOLTAGE_A  (0xF << 16) /* RW */
//#define MCI_CNTRL_CARD_VOLTAGE_B  (0xF << 20) /* RW */
#define MCI_CNTRL_ENABLE_OD_PULLUP  (0x1 << 24) /* RW */
#define MCI_CNTRL_USE_INTERNAL_DMAC (0x1 << 25) /* RW */

/* MCI_PWREN mask */
#define MCI_PWREN_ENABLE            (0x1 << 0)  /* RW */

/* MCI_CLKENA mask */
#define MCI_CLKENA_CCLK_ENABLE      (0x1 << 0) /* RW */
#define MCI_CLKENA_CCLK_LOW_POWER   (0x1 << 16) /* RW */
#define MCI_EXT_CLK_ENABLE          (0x1 << 1)

/* MCI_INT_MASK mask */
#define MCI_INT_MASK_CD             (0x1 << 0) /* RW */
#define MCI_INT_MASK_RE             (0x1 << 1) /* RW */
#define MCI_INT_MASK_CMD            (0x1 << 2) /* RW */
#define MCI_INT_MASK_DTO            (0x1 << 3) /* RW */
#define MCI_INT_MASK_TXDR           (0x1 << 4) /* RW */
#define MCI_INT_MASK_RXDR           (0x1 << 5) /* RW */
#define MCI_INT_MASK_RCRC           (0x1 << 6) /* RW */
#define MCI_INT_MASK_DCRC           (0x1 << 7) /* RW */
#define MCI_INT_MASK_RTO            (0x1 << 8) /* RW */
#define MCI_INT_MASK_DRTO           (0x1 << 9) /* RW */
#define MCI_INT_MASK_HTO            (0x1 << 10) /* RW */
#define MCI_INT_MASK_FRUN           (0x1 << 11) /* RW */
#define MCI_INT_MASK_HLE            (0x1 << 12) /* RW */
#define MCI_INT_MASK_SBE_BCI        (0x1 << 13) /* RW */
#define MCI_INT_MASK_ACD            (0x1 << 14) /* RW */
#define MCI_INT_MASK_EBE            (0x1 << 15) /* RW */
#define MCI_INT_MASK_SDIO           (0x1 << 16) /* RW */


/* MCI_MASKED_INTS mask */
#define MCI_MASKED_INTS_CD          (0x1 << 0) /* RO */
#define MCI_MASKED_INTS_RE          (0x1 << 1) /* RO */
#define MCI_MASKED_INTS_CMD         (0x1 << 2) /* RO */
#define MCI_MASKED_INTS_DTO         (0x1 << 3) /* RO */
#define MCI_MASKED_INTS_TXDR        (0x1 << 4) /* RO */
#define MCI_MASKED_INTS_RXDR        (0x1 << 5) /* RO */
#define MCI_MASKED_INTS_RCRC        (0x1 << 6) /* RO */
#define MCI_MASKED_INTS_DCRC        (0x1 << 7) /* RO */
#define MCI_MASKED_INTS_RTO         (0x1 << 8) /* RO */
#define MCI_MASKED_INTS_DRTO        (0x1 << 9) /* RO */
#define MCI_MASKED_INTS_HTO         (0x1 << 10) /* RO */
#define MCI_MASKED_INTS_FRUN        (0x1 << 11) /* RO */
#define MCI_MASKED_INTS_HLE         (0x1 << 12) /* RO */
#define MCI_MASKED_INTS_SBE_BCI     (0x1 << 13) /* RO */
#define MCI_MASKED_INTS_ACD         (0x1 << 14) /* RO */
#define MCI_MASKED_INTS_EBE         (0x1 << 15) /* RO */
#define MCI_MASKED_INTS_SDIO        (0x1 << 16) /* RO */

/* MCI_RAW_INTS mask */
#define MCI_RAW_INTS_CD             (0x1 << 0) /* W1C */
#define MCI_RAW_INTS_RE             (0x1 << 1) /* W1C */
#define MCI_RAW_INTS_CMD            (0x1 << 2) /* W1C */
#define MCI_RAW_INTS_DTO            (0x1 << 3) /* W1C */
#define MCI_RAW_INTS_TXDR           (0x1 << 4) /* W1C */
#define MCI_RAW_INTS_RXDR           (0x1 << 5) /* W1C */
#define MCI_RAW_INTS_RCRC           (0x1 << 6) /* W1C */
#define MCI_RAW_INTS_DCRC           (0x1 << 7) /* W1C */
#define MCI_RAW_INTS_RTO            (0x1 << 8) /* W1C */
#define MCI_RAW_INTS_DRTO           (0x1 << 9) /* W1C */
#define MCI_RAW_INTS_HTO            (0x1 << 10) /* W1C */
#define MCI_RAW_INTS_FRUN           (0x1 << 11) /* W1C */
#define MCI_RAW_INTS_HLE            (0x1 << 12) /* W1C */
#define MCI_RAW_INTS_SBE_BCI        (0x1 << 13) /* W1C */
#define MCI_RAW_INTS_ACD            (0x1 << 14) /* W1C */
#define MCI_RAW_INTS_EBE            (0x1 << 15) /* W1C */
#define MCI_RAW_INTS_SDIO           (0x1 << 16)  /* W1C */

/* MCI_STATUS mask */
#define MCI_STATUS_FIFO_RX          (0x1 << 0) /* RO */
#define MCI_STATUS_FIFO_TX          (0x1 << 1) /* RO */
#define MCI_STATUS_FIFO_EMPTY       (0x1 << 2) /* RO */
#define MCI_STATUS_FIFO_FULL        (0x1 << 3) /* RO */
#define MCI_STATUS_CARD_STATUS      (0x1 << 8) /* RO */
#define MCI_STATUS_CARD_BUSY        (0x1 << 9) /* RO */
#define MCI_STATUS_DATA_BUSY        (0x1 << 10) /* RO */
#define MCI_STATUS_DMA_ACK          (0x1 << 31) /* RO */
#define MCI_STATUS_DMA_REQ          (0x1 << 32) /* RO */


/* MCI_UHS_REG mask */
#define MCI_UHS_REG_VOLT            (0x1 << 0) /* RW */
#define MCI_UHS_REG_DDR             (0x1 << 16) /* RW */

/* MCI_CARD_RESET mask */
#define MCI_CARD_RESET_ENABLE       (0x1 << 0) /* RW */

/* MCI_BUS_MODE mask */
#define MCI_BUS_MODE_SWR            (0x1 << 0) /* RW */
#define MCI_BUS_MODE_FB             (0x1 << 1) /* RW */
#define MCI_BUS_MODE_DE             (0x1 << 7) /* RW */

/* MCI_DMAC_STATUS mask */
#define MCI_DMAC_STATUS_TI          (0x1 << 0) /* RW */
#define MCI_DMAC_STATUS_RI          (0x1 << 1) /* RW */
#define MCI_DMAC_STATUS_FBE         (0x1 << 2) /* RW */
#define MCI_DMAC_STATUS_DU          (0x1 << 4) /* RW */
#define MCI_DMAC_STATUS_NIS         (0x1 << 8) /* RW */
#define MCI_DMAC_STATUS_AIS         (0x1 << 9) /* RW */

/* MCI_DMAC_INT_ENA mask */
#define MCI_DMAC_INT_ENA_TI         (0x1 << 0) /* RW */
#define MCI_DMAC_INT_ENA_RI         (0x1 << 1) /* RW */
#define MCI_DMAC_INT_ENA_FBE        (0x1 << 2) /* RW */
#define MCI_DMAC_INT_ENA_DU         (0x1 << 4) /* RW */
#define MCI_DMAC_INT_ENA_CES        (0x1 << 5) /* RW */
#define MCI_DMAC_INT_ENA_NIS        (0x1 << 8) /* RW */
#define MCI_DMAC_INT_ENA_AIS        (0x1 << 9) /* RW */

/* MCI_CARD_THRCTL mask */
#define MCI_CARD_THRCTL_CARDRD      (0x1 << 0) /* RW */
#define MCI_CARD_THRCTL_BUSY_CLR    (0x1 << 1) /* RW */
#define MCI_CARD_THRCTL_CARDWR      (0x1 << 2) /* RW */

/* MCI_UHS_REG_EXT mask */
#define MCI_UHS_REG_EXT_MMC_VOLT    (0x1 << 0) /* RW */
#define MCI_UHS_REG_EXT_CLK_ENA     (0x1 << 1) /* RW */

/* MCI_EMMC_DDR_REG mask */
#define MCI_EMMC_DDR_CYCLE          (0x1 << 0) /* RW */


static const u32 cmd_ints_mask = MCI_INT_MASK_RE | MCI_INT_MASK_CMD | MCI_INT_MASK_RCRC |
                 MCI_INT_MASK_RTO | MCI_INT_MASK_HTO | MCI_RAW_INTS_HLE;
static const u32 data_ints_mask = MCI_INT_MASK_DTO | MCI_INT_MASK_DCRC | MCI_INT_MASK_DRTO |
                  MCI_INT_MASK_SBE_BCI;
//static const u32 cmd_err_ints_mask = MCI_INT_MASK_RTO | MCI_INT_MASK_RCRC | MCI_INT_MASK_RE |
//                     MCI_INT_MASK_DCRC | MCI_INT_MASK_DRTO |
//                     MCI_MASKED_INTS_SBE_BCI;

static const u32 dmac_ints_mask = MCI_DMAC_INT_ENA_FBE | MCI_DMAC_INT_ENA_DU | MCI_DMAC_INT_ENA_RI |
                  MCI_DMAC_INT_ENA_NIS | MCI_DMAC_INT_ENA_AIS;

struct phytium_sdci_plat {
    struct mmc_config cfg;
    struct mmc mmc;
};

struct phytium_mci_idmac {
    u32 attribute;
#define IDMAC_DES0_DIC  BIT(1)
#define IDMAC_DES0_LD   BIT(2)
#define IDMAC_DES0_FD   BIT(3)
#define IDMAC_DES0_CH   BIT(4)
#define IDMAC_DES0_ER   BIT(5)
#define IDMAC_DES0_CES  BIT(30)
#define IDMAC_DES0_OWN  BIT(31)
    u32 NON1;
    u32 len;
    u32 NON2;
    u32 addr_lo; /* Lower 32-bits of Buffer Address Pointer 1*/
    u32 addr_hi; /* Upper 32-bits of Buffer Address Pointer 1*/
    u32 desc_lo; /* Lower 32-bits of Next Descriptor Address */
    u32 desc_hi; /* Upper 32-bits of Next Descriptor Address */
};

static void sdr_set_bits(void __iomem *reg, u32 bs)
{
    u32 val = readl(reg);

    val |= bs;
    writel(val, reg);
}

static void sdr_clr_bits(void __iomem *reg, u32 bs)
{
    u32 val = readl(reg);

    val &= ~bs;
    writel(val, reg);
}

static void mci_set_idma_desc(struct phytium_mci_idmac *idmac,
        u32 desc0, u32 desc2, u32 desc4, u32 desc5, u32 desc6, u32 desc7)
{
    struct phytium_mci_idmac *desc = idmac;

    desc->attribute = desc0;
    desc->NON1 = 0;
    desc->len = desc2;
    desc->NON2 = 0;
    desc->addr_lo = desc4;
    desc->addr_hi = desc5;
    desc->desc_lo = desc6;
    desc->desc_hi = desc7;
    //desc->next_addr = (ulong)desc + sizeof(struct phytium_mci_idmac);
}

static void phytium_mci_idmac_reset(struct phytium_sdci_host *host)
{
    u32 bmod = readl(host->base + MCI_BUS_MODE);

    bmod |= MCI_BUS_MODE_SWR;
    writel(bmod, host->base + MCI_BUS_MODE);
}
/*
static void
phytium_mci_data_sg_write_2_fifo(struct phytium_mci_host *host, struct mmc_data *data)
{
    struct scatterlist *sg;
    u32 dma_len, i, j;
    u32 *virt_addr;

    if (mmc_get_dma_dir(data) == DMA_TO_DEVICE) {
        writel(0x1<<10, host->base + MCI_CMD);
        for_each_sg(data->sg, sg, data->sg_count, i) {
            dma_len = sg_dma_len(sg);
            virt_addr = sg_virt(data->sg);
            for (j = 0; j < (dma_len / 4); j++) {
                writel(*virt_addr, host->base + MCI_DATA);
                virt_addr++;
            }
        }
    }
}

static void phytium_mci_send_cmd(struct phytium_sdci_host *host, u32 cmd, u32 arg)
{
    writel(arg, host->base + MCI_CMDARG);
    while (readl(host->base + MCI_STATUS) & MCI_STATUS_CARD_BUSY);

    writel(MCI_CMD_START | cmd, host->base + MCI_CMD);
    while (readl(host->base + MCI_CMD) & MCI_CMD_START);
}
*/
static void phytium_mci_set_buswidth(struct phytium_sdci_host *host, u32 width)
{
    u32 val;

    switch (width) {
    case 1:
        val = MCI_BUS_1BITS;
        break;
    case 4:
        val = MCI_BUS_4BITS;
        break;
    case 8:
        val = MCI_BUS_8BITS;
        break;
    default:
        val = MCI_BUS_1BITS;
        break;
    }
    writel(val, host->base + MCI_CTYPE);
    debug("Bus Width = %d, set value:0x%x\n", width, val);
}

static void phytium_mci_update_external_clk(struct phytium_sdci_host *host, u32 uhs_reg_value)
{
    writel(0, host->base + MCI_UHS_REG_EXT);
    writel(uhs_reg_value, host->base + MCI_UHS_REG_EXT);
    while (!(readl(host->base + MCI_CCLK_RDY) & 0x1))
    {
        udelay(5);
        debug("%s -- %d\n", __func__, __LINE__);
    }
}

static void phytium_sdci_reset_hw(struct phytium_sdci_host *host)
{
    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET);
    debug("%s -- %d -- %p\n", __func__, __LINE__, host->base);
    while (readl(host->base + MCI_CNTRL) & (MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET))
    {
        udelay(5);
        debug("0x%x\n", readl(host->base + MCI_CNTRL));
    }

    if (host->caps & MMC_CAP_NONREMOVABLE)
        sdr_set_bits(host->base + MCI_CARD_RESET, MCI_CARD_RESET_ENABLE);
    else
        sdr_clr_bits(host->base + MCI_CARD_RESET, MCI_CARD_RESET_ENABLE);

    udelay(200);
    sdr_set_bits(host->base + MCI_CARD_RESET, MCI_CARD_RESET_ENABLE);
}

static int wait_for_command_end(struct mmc *dev, struct mmc_cmd *cmd)
{
    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_CMD_VALUE;
    struct phytium_sdci_host *host = dev->priv;
    u32 *rsp = cmd->response;

    sdr_clr_bits(host->base + MCI_INT_MASK, cmd_ints_mask | data_ints_mask);

    statusmask = MCI_RAW_INTS_CMD;
    do
    {
        status = readl(host->base + MCI_RAW_INTS) & statusmask;
        debug("CMD cmd->status:[0x%08x]\n",readl(host->base + MCI_RAW_INTS));
        udelay(20);
        timeout--;
    } while ((!status) && timeout);

    status = readl(host->base + MCI_RAW_INTS);
	if (status & MCI_RAW_INTS_RTO) {
		/*
		 * Timeout here is not necessarily fatal. (e)MMC cards
		 * will splat here when they receive CMD55 as they do
		 * not support this command and that is exactly the way
		 * to tell them apart from SD cards. Thus, this output
		 * below shall be debug(). eMMC cards also do not favor
		 * CMD8, please keep that in mind.
		 */
		debug("%s: Response Timeout.\n", __func__);
		return -ETIMEDOUT;
	} else if (status & MCI_RAW_INTS_RE) {
		debug("%s: Response Error.\n", __func__);
		return -EIO;
	} else if ((cmd->resp_type & MMC_RSP_CRC) &&
		   (status & MCI_RAW_INTS_RCRC)) {
		debug("%s: Response CRC Error.\n", __func__);
		return -EIO;
	}

    if (cmd->resp_type & MMC_RSP_PRESENT) {
        if (cmd->resp_type & MMC_RSP_136) {
            rsp[3] = readl(host->base + MCI_RESP0);
            rsp[2] = readl(host->base + MCI_RESP1);
            rsp[1] = readl(host->base + MCI_RESP2);
            rsp[0] = readl(host->base + MCI_RESP3);
        } else {
            rsp[0] = readl(host->base + MCI_RESP0);
        }

//        if (cmd->opcode == SD_SEND_RELATIVE_ADDR)
//            host->current_rca = rsp[0] & 0xFFFF0000;
    }

    debug("CMD%d response[0]:0x%08X, response[1]:0x%08X, "
        "response[2]:0x%08X, response[3]:0x%08X\n",
        cmd->cmdidx, cmd->response[0], cmd->response[1],
        cmd->response[2], cmd->response[3]);

    return 0;
}

static inline u32 phytium_mci_cmd_find_resp(struct mmc_cmd *cmd)
{
    u32 resp;

    switch (cmd->resp_type) {
    case MMC_RSP_R1:
    case MMC_RSP_R1b:
        resp = 0x5;
        break;

    case MMC_RSP_R2:
        resp = 0x7;
        break;

    case MMC_RSP_R3:
        resp = 0x1;
        break;

    case MMC_RSP_NONE:
    default:
        resp = 0x0;
        break;
    }

    return resp;
}

static inline u32 phytium_mci_cmd_prepare_raw_cmd(
                     struct mmc_cmd *cmd)
{
    u32 opcode = cmd->cmdidx;
    u32 resp = phytium_mci_cmd_find_resp(cmd);
    u32 rawcmd = ((opcode & 0x3f) | ((resp & 0x7) << 6));

    debug("\nCMD%d prepare...\n", opcode);
    if (cmd->cmdidx == 0)
    {
        rawcmd |= (0x1 << 15);                                //在发送命令之前，等待80cyc 初始时钟序列完成。
    }

//    if (opcode == MMC_GO_INACTIVE_STATE ||
//        (opcode == SD_IO_RW_DIRECT && ((cmd->arg >> 9) & 0x1FFFF) == SDIO_CCCR_ABORT))
//        rawcmd |= (0x1 << 14);                                // 1-stop/abort 操作
//    else if (opcode == SD_SWITCH_VOLTAGE)
//        rawcmd |= (0x1 << 28);                                // 0 - 无电压切换  1 - 使能电压切换，

//    if (test_and_clear_bit(MCI_CARD_NEED_INIT, &host->flags))
//        rawcmd |= (0x1 << 15);                                //在发送命令之前，等待80cyc 初始时钟序列完成。

//    if (cmd->data) {                          //数据判断
//        struct mmc_data *data = cmd->data;
//
//        rawcmd |= (0x1 << 9);
//
//        if (data->flags & MMC_DATA_WRITE)     //读写卡判断
//            rawcmd |= (0x1 << 10);
//    }

    return (rawcmd | (0x1 << 29) | (0x1 << 31));
}

/* send command to the mmc card and wait for results */
static int do_command(struct mmc *dev, struct mmc_cmd *cmd)
{
    int result = 0;
    u32 rawcmd = 0;
    struct phytium_sdci_host * host = dev->priv;

    debug("%s -- %d -- command\n", __func__, __LINE__);
    /* 清除命令状态寄存器 */
    while ((readl(host->base + MCI_STATUS) & (MCI_STATUS_CARD_BUSY)))
    {
        debug("busy status\n");
    }
    writel(0xffffe, host->base + MCI_RAW_INTS);

    /* 设置命令和参数寄存器，触发发送命令 */
    rawcmd = phytium_mci_cmd_prepare_raw_cmd(cmd);

    sdr_set_bits(host->base + MCI_INT_MASK, cmd_ints_mask);

    writel(cmd->cmdarg, host->base + MCI_CMDARG);
    while (readl(host->base + MCI_STATUS) & MCI_STATUS_CARD_BUSY);

    writel(MCI_CMD_START | rawcmd, host->base + MCI_CMD);
    while (readl(host->base + MCI_CMD) & MCI_CMD_START);

    debug("cmd->cmdarg     :0x%08x\n", cmd->cmdarg);
    debug("rawcmd          :0x%08x\n", rawcmd);
    debug("MCI_CMDARG  addr:%p reg:0x%08x\n", host->base + MCI_CMDARG, readl(host->base + MCI_CMDARG)); //28
    debug("MCI_CMD     addr:%p reg:0x%08x\n", host->base + MCI_CMD, readl(host->base + MCI_CMD));       //2c

    result = wait_for_command_end(dev, cmd);

    /* After CMD2 set RCA to a none zero value. */
    //if ((result == 0) && (cmd->cmdidx == MMC_CMD_ALL_SEND_CID))
    //    dev->rca = 10;

    //sdr_set_bits(host->base + MCI_INT_MASK, cmd_ints_mask);
    //writel(cmd->cmdarg, host->base + MCI_CMDARG);
    //writel(rawcmd, host->base + MCI_CMD);

    return result;
}

static int read_bytes(struct mmc *dev, struct mmc_cmd *cmd, u32 *dest, u32 blkcount, u32 blksize)
{
    struct phytium_sdci_host * host = dev->priv;

    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_DATA_VALUE;
    u32 rawcmd;

    u32 *buf = host->dma_buf;
    u32 addrl = (u32)((u64)buf & 0xFFFFFFFF);
    u32 addrh = (u32)(((u64)buf >> 32) & 0xFFFFFFFF);

    debug("%s -- %d -- blkcount: %d -- blksize: %d\n", __func__, __LINE__, blkcount, blksize);

    invalidate_dcache_range((u64)buf, (u64)((u64)buf + (blkcount * blksize)) );

    /* clear interrupts */
    writel(0xffffe,    host->base + MCI_RAW_INTS);
    writel(0xffffffff, host->base + MCI_DMAC_STATUS);

    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET);
    while (readl(host->base + MCI_CNTRL) & (MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET)){;}

    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_USE_INTERNAL_DMAC);

    sdr_clr_bits(host->base + MCI_CNTRL, MCI_CNTRL_INT_ENABLE);

    rawcmd = phytium_mci_cmd_prepare_raw_cmd(cmd);
    rawcmd |= (0x1 << 9);

//    ulong data_start, data_end;

    ALLOC_CACHE_ALIGN_BUFFER(struct phytium_mci_idmac, cur_idmac, 1);   //描述符个数1

//    struct phytium_mci_idmac *cur_idmac;
//    cur_idmac = malloc(128);
//    invalidate_dcache_range((u64)cur_idmac, (u64)((u64)cur_idmac + (blkcount * 128)) );

    mci_set_idma_desc(cur_idmac,
                        0x8000003c,
                        512*blkcount,
                        addrl,
                        addrh,
                        0x0,
                        0x0);
    phytium_mci_idmac_reset(host);

    debug("cur_idmac       :%p\n", cur_idmac);
    writel((u32)((u64)cur_idmac),  host->base + MCI_DESC_LIST_ADDRL);
    writel(0,               host->base + MCI_DESC_LIST_ADDRH);

    debug("CUR addrl       :0x%08x\n", addrl);

    sdr_set_bits(host->base + MCI_INT_MASK, cmd_ints_mask | data_ints_mask);
    if (blksize >0){//(host->is_use_dma && host->adtc_type == BLOCK_RW_ADTC) {
        sdr_set_bits(host->base + MCI_DMAC_INT_ENA, dmac_ints_mask);
        /* Enable the IDMAC */
        sdr_set_bits(host->base + MCI_BUS_MODE, MCI_BUS_MODE_DE);
        //debug("DESC addrl       :0x%08x\n", addrl);
        //debug("DESC addrh       :0x%08x\n", addrh);
    }

    writel(512, host->base + MCI_BLKSIZ);                   //块大小
    writel(blkcount* 512, host->base + MCI_BYTCNT);         //数据大小
    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_INT_ENABLE); //使能中断

    debug ("[0x48: 0x%08x]\n",readl(host->base + MCI_STATUS));
    debug ("[0x90: 0x%08x]\n",readl(host->base + MCI_DMAC_STATUS));

    writel(cmd->cmdarg, host->base + MCI_CMDARG);               //写cmdarg
    writel(rawcmd, host->base + MCI_CMD);                       //写cmd

    debug("cmd->cmdarg      :0x%08x\n", cmd->cmdarg);
    debug("rawcmd           :0x%08x\n", rawcmd);
    debug("MCI_CMDARG   addr:%p reg:0x%08x\n", host->base + MCI_CMDARG, readl(host->base + MCI_CMDARG)); //28
    debug("MCI_CMD      addr:%p reg:0x%08x\n", host->base + MCI_CMD, readl(host->base + MCI_CMD));       //2c

    wait_for_command_end(dev, cmd);

    if (blksize>0)
    {
        statusmask = data_ints_mask;
        do
        {
            status = readl(host->base + MCI_RAW_INTS) & statusmask;
            udelay(20);
            timeout--;
        } while ((!status) && timeout);
        if (!timeout)
        {
            printf("sd date read timeout:[0x44: 0x%08x]\n",readl(host->base + MCI_RAW_INTS));
            return -ETIMEDOUT;
        }
        debug("[0x44: 0x%08x]\n",readl(host->base + MCI_RAW_INTS));
        debug("[0x48: 0x%08x]\n",readl(host->base + MCI_STATUS));

        timeout = SDCI_TIMEOUT_DATA_VALUE;
        statusmask = dmac_ints_mask;
        do
        {
            status = readl(host->base + MCI_DMAC_STATUS) & statusmask;
            timeout--;
        } while ((!status) && timeout);
        if(!timeout)
        {
            printf("sd date read error:[0x90: 0x%08x]\n",readl(host->base + MCI_DMAC_STATUS));
            return -ETIMEDOUT;
        }
        debug ("[0x90: 0x%08x]\n",readl(host->base + MCI_DMAC_STATUS));

        memcpy(dest, buf, blkcount * blksize);
    }

    //for (int i=0; i<blksize*blkcount/4; i++)
    //{
    //    if (i%8 == 0) debug("\n");
    //    debug("%08x ", buf[i]);
    //}
    //debug("\n");

    return 0;
}

static int write_bytes(struct mmc *dev, struct mmc_cmd *cmd, u32 *src, u32 blkcount, u32 blksize)
{
    struct phytium_sdci_host * host = dev->priv;
    u32 status, statusmask;
    int timeout = SDCI_TIMEOUT_DATA_VALUE;
    u32 rawcmd;
    u32 *buf = host->dma_buf;
    u32 addrl = (u32)((u64)buf & 0xFFFFFFFF);
    u32 addrh = (u32)(((u64)buf >> 32) & 0xFFFFFFFF);

    debug("%s -- %d -- blkcount: %d -- blksize: %d\n", __func__, __LINE__, blkcount, blksize);
    memcpy(buf, src, blkcount * blksize);
    flush_dcache_range((u64)buf, (u64)((u64)buf + blkcount * blksize));
    //for (int i=0; i<blksize*blkcount/4; i++)
    //{
    //    if (i%8 == 0) debug("\n");
    //    debug("%08x ", buf[i]);
    //}
    //debug("\n");

    /* clear interrupts */
    writel(0xffffe,    host->base + MCI_RAW_INTS);
    writel(0xffffffff, host->base + MCI_DMAC_STATUS);

    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET);
    while (readl(host->base + MCI_CNTRL) & (MCI_CNTRL_FIFO_RESET | MCI_CNTRL_DMA_RESET));

    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_USE_INTERNAL_DMAC);

    sdr_clr_bits(host->base + MCI_CNTRL, MCI_CNTRL_INT_ENABLE);

    rawcmd = phytium_mci_cmd_prepare_raw_cmd(cmd);
    rawcmd |= (0x1 << 9);
    rawcmd |= (0x1 << 10);

    ALLOC_CACHE_ALIGN_BUFFER(struct phytium_mci_idmac, cur_idmac, 1);   //描述符个数1

    mci_set_idma_desc(cur_idmac,
                        0x8000003c,
                        blksize*blkcount,
                        addrl,
                        addrh,
                        0x0,
                        0x0);
    phytium_mci_idmac_reset(host);

    debug("cur_idmac       :%p\n", cur_idmac);
    writel((u32)((u64)cur_idmac),  host->base + MCI_DESC_LIST_ADDRL);
    writel(0,               host->base + MCI_DESC_LIST_ADDRH);

    debug("CUR addrl       :0x%08x\n", addrl);

    sdr_set_bits(host->base + MCI_INT_MASK, cmd_ints_mask | data_ints_mask);
    if (blksize >= 512){
        sdr_set_bits(host->base + MCI_DMAC_INT_ENA, dmac_ints_mask);
        /* Enable the IDMAC */
        sdr_set_bits(host->base + MCI_BUS_MODE, MCI_BUS_MODE_DE);
    }

    writel(blksize, host->base + MCI_BLKSIZ);                   //块大小
    writel(blkcount* blksize, host->base + MCI_BYTCNT);         //数据大小
    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_INT_ENABLE); //使能中断

    debug ("[0x48: 0x%08x]\n",readl(host->base + MCI_STATUS));
    debug ("[0x90: 0x%08x]\n",readl(host->base + MCI_DMAC_STATUS));

    writel(cmd->cmdarg, host->base + MCI_CMDARG);               //写cmdarg
    writel(rawcmd, host->base + MCI_CMD);                       //写cmd

    wait_for_command_end(dev, cmd);

    statusmask = data_ints_mask;
    do
    {
        status = readl(host->base + MCI_RAW_INTS) & statusmask;

        udelay(20);
        timeout--;
    } while ((!status) && timeout);
    if (!timeout)
    {
        printf("sd date read timeout:[0x44: 0x%08x]\n",readl(host->base + MCI_RAW_INTS));
        return -ETIMEDOUT;
    }
    debug("[0x44: 0x%08x]\n",readl(host->base + MCI_RAW_INTS));
    debug("[0x48: 0x%08x]\n",readl(host->base + MCI_STATUS));

    return 0;
}

static int do_data_transfer(struct mmc *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int error = -ETIMEDOUT;

    if (data->flags & MMC_DATA_READ) {
        error = read_bytes(dev, cmd, (u32 *)data->dest, (u32)data->blocks,
                (u32)data->blocksize);
    } else if (data->flags & MMC_DATA_WRITE) {
        error = write_bytes(dev, cmd, (u32 *)data->src, (u32)data->blocks,
                        (u32)data->blocksize);
    }

    return error;
}

static int host_request(struct mmc *dev, struct mmc_cmd *cmd, struct mmc_data *data)
{
    int result;


    if (data){
        result = do_data_transfer(dev, cmd, data);
    }else{
        result = do_command(dev, cmd);
    }

    return result;
}

#define FSDIO_CLK_RATE_HZ (1200000000UL) /* 1.2GHz */

#define FSDIO_SD_400KHZ                 400000U
#define FSDIO_SD_25_MHZ                 25000000U
#define FSDIO_SD_50_MHZ                 50000000U

/* set 32-bit register [a:b] as x, where a is high bit, b is low bit, x is setting/getting value */
#define GET_REG32_BITS(x, a, b)                  (u32)((((u32)(x)) & GENMASK(a, b)) >> b)
#define SET_REG32_BITS(x, a, b)                  (u32)((((u32)(x)) << b) & GENMASK(a, b))

/** @name FSDIO_UHS_REG_EXT_OFFSET Register
 */
#define FSDIO_UHS_EXT_MMC_VOLT BIT(0)         /* RW 1.2V供电选择 */
#define FSDIO_UHS_EXT_CLK_ENA BIT(1)          /* RW 外部时钟，CIU时钟使能 */
#define FSDIO_UHS_CLK_DIV_MASK GENMASK(14, 8) /* RW 分频系数，ciu_f = clk_div_ctrl + 1, min=1*/
#define FSDIO_UHS_CLK_DIV(x) (FSDIO_UHS_CLK_DIV_MASK & ((x) << 8))
#define FSDIO_UHS_CLK_SAMP_MASK GENMASK(22, 16) /* RW 采样相位参数，相对于ciu时钟相位点 */
#define FSDIO_UHS_CLK_SAMP(x) (FSDIO_UHS_CLK_SAMP_MASK & ((x) << 16))
#define FSDIO_UHS_CLK_DRV_MASK GENMASK(30, 24) /* RW 输出相位参数，相对于ciu时钟相位点 */
#define FSDIO_UHS_CLK_DRV(x) (FSDIO_UHS_CLK_DRV_MASK & ((x) << 24))
#define FSDIO_UHS_EXT_CLK_MUX BIT(31)

/* FSDIO_UHS_REG_EXT_OFFSET 和 FSDIO_CLKDIV_OFFSET 两个寄存器配合完成卡时钟和驱动采样相位调整
    UHS_REG_EXT 配置一级分频，CLK_DIV 决定CARD工作时钟频率, DRV 和 SAMP 分别控制驱动相位和采样相位粗调
        分配系数 = bit [14 : 8] + 1
    CLKDIV 配置二级分频, DIVIDER , DRV 和 SAMP 分别控制驱动相位和采样相位精调
        分配系数 = bit [7: 0] * 2
*/
#define FSDIO_UHS_REG(drv_phase, samp_phase, clk_div) \
    (FSDIO_UHS_CLK_DRV(drv_phase) |                   \
     FSDIO_UHS_CLK_SAMP(samp_phase) |                 \
     FSDIO_UHS_CLK_DIV(clk_div))

#define FSDIO_UHS_CLK_DIV_SET(x)        SET_REG32_BITS((x), 14, 8)
#define FSDIO_UHS_CLK_DIV_GET(reg_val)  GET_REG32_BITS((reg_val), 14, 8)
#define FSDIO_UHS_CLK_SAMP_SET(x)       SET_REG32_BITS((x), 22, 16)
#define FSDIO_UHS_CLK_DRV_SET(x)        SET_REG32_BITS((x), 30, 24)

static int  host_set_ios(struct mmc *dev)
{
    struct phytium_sdci_host *host = dev->priv;
    u32 div = 0xff, drv = 0, sample = 0;
    u32 first_uhs_div, tmp_ext_reg;
    u32 input_clk_hz = dev->clock;
    u32 regs;
    debug("%s -- %d -- host->clock_in: %dHz -- dev->clock: %dHz\n", __func__, __LINE__, host->clock_in, dev->clock);

    phytium_mci_set_buswidth(host, dev->bus_width);

	regs = readl(host->base + MCI_UHS_REG);
	if (dev->ddr_mode)
		regs |= MCI_UHS_REG_DDR;
	else
		regs &= ~MCI_UHS_REG_DDR;
	writel(regs, host->base + MCI_UHS_REG);


    if(dev->clock)
    {
        writel(0x1,         host->base + MCI_PWREN);
    }else{
        writel(0x0,         host->base + MCI_PWREN);

        sdr_clr_bits(host->base + MCI_CLKENA, MCI_CLKENA_CCLK_ENABLE);
        sdr_clr_bits(host->base + MCI_UHS_REG_EXT, MCI_EXT_CLK_ENABLE);
    }

    /* must set 2nd stage clcok first then set 1st stage clock */
    /* experimental uhs setting  --> 2nd stage clock */
    if (input_clk_hz >= FSDIO_SD_25_MHZ) /* e.g. 25MHz or 50MHz */
    {
        tmp_ext_reg = FSDIO_UHS_REG(0U, 0U, 0x2U) | FSDIO_UHS_EXT_CLK_ENA;
    }
    else if (input_clk_hz == FSDIO_SD_400KHZ) /* 400kHz */
    {
        tmp_ext_reg = FSDIO_UHS_REG(0U, 0U, 0x5U) | FSDIO_UHS_EXT_CLK_ENA;
    }
    else /* e.g. 20MHz */
    {
        tmp_ext_reg = FSDIO_UHS_REG(0U, 0U, 0x3U) | FSDIO_UHS_EXT_CLK_ENA;
    }

    /* update uhs setting */
    writel(0x0,host->base + MCI_UHS_REG_EXT);
    writel(0x00000502,host->base + MCI_UHS_REG_EXT);
    while((readl(host->base + MCI_CCLK_RDY)&0x1)!=0x1);             //直到data[0]=1,表明时钟分频完成

    writel(0x0,host->base+0x10);                                    //：关断时钟

    /* send private cmd to update clock */
    while(readl(host->base + MCI_STATUS) & MCI_STATUS_CARD_BUSY);
    writel(0, host->base+0x28);
    writel(0x1<<21|0x1<<31, host->base+0x2c);

    first_uhs_div = 1 + ((tmp_ext_reg >> 8) & 0xFF);
    div = FSDIO_CLK_RATE_HZ / (2 * first_uhs_div * input_clk_hz);
    if (div > 2) {
        sample = div / 2 + 1;
        drv = sample - 1;
        writel((sample << 16) | (drv << 8) | (div & 0xff), host->base+0x08);                //：卡时钟分频，卡时钟分频参数、采样和驱动相位
    } else if (div == 2) {
        drv = 0;
        sample = 1;
        writel((sample << 16) | (drv << 8) | (div & 0xff), host->base+0x08);                //：卡时钟分频，卡时钟分频参数、采样和驱动相位
    }

    writel(0x1,host->base+0x10);                                    //：开时钟

    /* update clock for 1 stage clock */
    while(readl(host->base + MCI_STATUS) & MCI_STATUS_CARD_BUSY);   //card busy
    writel(0, host->base+0x28);
    writel(0x1<<21|0x1<<31, host->base+0x2c);

    return 0;
}

#ifdef CONFIG_DM_MMC
static void phytium_sdci_init(struct phytium_sdci_host *host)
{
    u32 val;

    writel(0, 0x2807e0c0);          //creg_nand_mmcsd_hddr
    writel(0, 0x2807e1a0);          //时钟域qchannel寄存器
    writel(0, 0x2807e1b0);
    writel(0, 0x2807e1c0);
    writel(0, 0x2807e1d0);
    writel(0, 0x2807e1e0);
    
    debug("%s -- %d\n", __func__, __LINE__);

    sdr_set_bits(host->base + MCI_PWREN, MCI_PWREN_ENABLE);
    sdr_set_bits(host->base + MCI_CLKENA, MCI_CLKENA_CCLK_ENABLE);
    sdr_set_bits(host->base + MCI_UHS_REG_EXT, MCI_EXT_CLK_ENABLE);
    sdr_clr_bits(host->base + MCI_UHS_REG, MCI_UHS_REG_VOLT);
    debug("%s -- %d -- %p\n", __func__, __LINE__, host->base);
    debug("MCI_PWREN        : 0x%x\n", readl(host->base + MCI_PWREN));
    debug("MCI_CLKENA       : 0x%x\n", readl(host->base + MCI_CLKENA));
    debug("MCI_UHS_REG_EXT  : 0x%x\n", readl(host->base + MCI_UHS_REG_EXT));
    debug("MCI_UHS_REG      : 0x%x\n", readl(host->base + MCI_UHS_REG));

    phytium_sdci_reset_hw(host);

    writel(0, host->base + MCI_INT_MASK);
    val = readl(host->base + MCI_RAW_INTS);
    writel(val, host->base + MCI_RAW_INTS);
    writel(0, host->base + MCI_DMAC_INT_ENA);
    val = readl(host->base + MCI_DMAC_STATUS);
    writel(val, host->base + MCI_DMAC_STATUS);
    if (!(host->caps & MMC_CAP_NONREMOVABLE))
        writel(MCI_INT_MASK_CD, host->base + MCI_INT_MASK);

    sdr_set_bits(host->base + MCI_CNTRL, MCI_CNTRL_INT_ENABLE |
             MCI_CNTRL_USE_INTERNAL_DMAC);

    writel(0xFFFFFFFF, host->base + MCI_TMOUT);

    debug("init phytium mci hardware done!\n");
}

static int phytium_sdci_probe(struct udevice *dev)
{
    struct phytium_sdci_plat *pdata = dev_get_plat(dev);
    struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
    struct mmc *mmc = &pdata->mmc;
    struct phytium_sdci_host *host = dev->priv_;
    struct mmc_config *cfg = &pdata->cfg;
    int ret;
    u32 clk;

    host->clkdiv_init = SDCI_CLOCK_DIV_DEFAELT;

    clk = dev_read_u32_default(dev, "clock", 600000000);
    host->clock_in = clk;
    printf("clk = %dHz\n", clk);

    cfg->name = "PHYTIUM MCI";
    cfg->voltages = MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_165_195;
    cfg->host_caps |= MMC_MODE_4BIT | MMC_MODE_HS | MMC_MODE_HS_52MHz;
    cfg->f_min = MMC_CLOCK_MIN;
    cfg->f_max = MMC_CLOCK_MAX;
    cfg->b_max = 15;

    ret = mmc_of_parse(dev, cfg);
    if (ret)
        return ret;

    host->dma_buf = memalign(4096, 16*512);
    if(!host->dma_buf)
    {
        debug("alloc buffer failed!\n");
        return -ENOMEM;
    }
    writel(MCI_SET_FIFOTH(0x2, 7, 0x100), host->base + MCI_FIFOTH);
    writel(0,    host->base + MCI_CNTRL);
    sdr_clr_bits(host->base + MCI_CLKENA, MCI_CLKENA_CCLK_ENABLE);
    phytium_mci_update_external_clk(host, 0x502);

    phytium_sdci_init(host);

    mmc->priv = host;
    mmc->dev = dev;
    upriv->mmc = mmc;

    return 0;
}

int phytium_sdci_bind(struct udevice *dev)
{
    struct phytium_sdci_plat *plat = dev_get_plat(dev);

    return mmc_bind(dev, &plat->mmc, &plat->cfg);
}

static int dm_host_request(struct udevice *dev, struct mmc_cmd *cmd,
               struct mmc_data *data)
{
    struct mmc *mmc = mmc_get_mmc_dev(dev);

    return host_request(mmc, cmd, data);
}

static int dm_host_set_ios(struct udevice *dev)
{
    struct mmc *mmc = mmc_get_mmc_dev(dev);

    return host_set_ios(mmc);
}

static int dm_mmc_getcd(struct udevice *dev)
{
    debug("%s -- %d\n", __func__, __LINE__);
    struct mmc *mmc = mmc_get_mmc_dev(dev);
    struct phytium_sdci_host *host = dev->priv_;

    if (mmc->cfg->host_caps & MMC_CAP_NONREMOVABLE)
        return 1;

    return !(readl(host->base + MCI_CARD_DETECT) & 1);
}

//#include "sdhci.h"
#define MAX_SDCD_DEBOUNCE_TIME 2000
static int phytium_mci_deferred_probe(struct udevice *dev)
{
	unsigned long start;
	int val;
	/*
	 * The controller takes about 1 second to debounce the card detect line
	 * and doesn't let us power on until that time is up. Instead of waiting
	 * for 1 second at every stage, poll on the CARD_PRESENT bit upto a
	 * maximum of 2 seconds to be safe..
	 */
	start = get_timer(0);
	do {
		if (get_timer(start) > MAX_SDCD_DEBOUNCE_TIME)
			return -ENOMEDIUM;

        val = dm_mmc_getcd(dev);
	} while (!val);

    return 0;
}

static const struct dm_mmc_ops phytium_dm_sdc_ops = {
    .send_cmd = dm_host_request,
    .set_ios = dm_host_set_ios,
    .get_cd = dm_mmc_getcd,
    .deferred_probe = phytium_mci_deferred_probe,
};

static int phytium_sdci_ofdata_to_platdata(struct udevice *dev)
{
    struct phytium_sdci_host *host = dev->priv_;
    fdt_addr_t addr;

    addr = dev_read_addr(dev);
    if (addr == FDT_ADDR_T_NONE)
        return -EINVAL;

    host->base = (void *)addr;

    return 0;
}

static const struct udevice_id phytium_sdci_match[] = {
    { .compatible = "phytium,phytium-mci" },
    { /* sentinel */ }
};

U_BOOT_DRIVER(phytium_sdci) = {
    .name = "phytium_mci",
    .id = UCLASS_MMC,
    .of_match = phytium_sdci_match,
    .ops = &phytium_dm_sdc_ops,
    .probe = phytium_sdci_probe,
    .of_to_plat = phytium_sdci_ofdata_to_platdata,
    .bind = phytium_sdci_bind,
    .priv_auto = sizeof(struct phytium_sdci_host),
    .plat_auto = sizeof(struct phytium_sdci_plat),
};
#endif
