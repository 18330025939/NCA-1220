/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Phytium ethernet driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */

#ifndef __DRIVERS_FT_H__
#define __DRIVERS_FT_H__

#define FT_GREGS_NBR 16
#define FT_GREGS_VERSION 2
#define FT_MAX_QUEUES 8

/* FT register offsets */
#define FT_NCR		0x0000 /* Network Control */
#define FT_NCFGR	0x0004 /* Network Config */
#define FT_NSR		0x0008 /* Network Status */
#define FT_TAR		0x000c /* AT91RM9200 only */
#define FT_TCR		0x0010 /* AT91RM9200 only */
#define FT_TSR		0x0014 /* Transmit Status */
#define FT_RBQP		0x0018 /* RX Q Base Address */
#define FT_TBQP		0x001c /* TX Q Base Address */
#define FT_RSR		0x0020 /* Receive Status */
#define FT_ISR		0x0024 /* Interrupt Status */
#define FT_IER		0x0028 /* Interrupt Enable */
#define FT_IDR		0x002c /* Interrupt Disable */
#define FT_IMR		0x0030 /* Interrupt Mask */
#define FT_MAN		0x0034 /* PHY Maintenance */
#define FT_PTR		0x0038
#define FT_PFR		0x003c
#define FT_FTO		0x0040
#define FT_SCF		0x0044
#define FT_MCF		0x0048
#define FT_FRO		0x004c
#define FT_FCSE		0x0050
#define FT_ALE		0x0054
#define FT_DTF		0x0058
#define FT_LCOL		0x005c
#define FT_EXCOL	0x0060
#define FT_TUND		0x0064
#define FT_CSE		0x0068
#define FT_RRE		0x006c
#define FT_ROVR		0x0070
#define FT_RSE		0x0074
#define FT_ELE		0x0078
#define FT_RJA		0x007c
#define FT_USF		0x0080
#define FT_STE		0x0084
#define FT_RLE		0x0088
#define FT_TPF		0x008c
#define FT_HRB		0x0090
#define FT_HRT		0x0094
#define FT_SA1B		0x0098
#define FT_SA1T		0x009c
#define FT_SA2B		0x00a0
#define FT_SA2T		0x00a4
#define FT_SA3B		0x00a8
#define FT_SA3T		0x00ac
#define FT_SA4B		0x00b0
#define FT_SA4T		0x00b4
#define FT_TID		0x00b8
#define FT_TPQ		0x00bc
#define FT_USRIO	0x00c0
#define FT_WOL		0x00c4
#define FT_MID		0x00fc
#define FT_TBQPH	0x04C8
#define FT_RBQPH	0x04D4

/* FT_ETH register offsets. */
#define FT_ETH_NCFGR			0x0004 /* Network Config */
#define FT_ETH_USRIO			0x000c /* User IO */
#define FT_ETH_DMACFG			0x0010 /* DMA Configuration */
#define FT_ETH_JML				0x0048 /* Jumbo Max Length */
#define FT_ETH_HRB				0x0080 /* Hash Bottom */
#define FT_ETH_HRT				0x0084 /* Hash Top */
#define FT_ETH_SA1B				0x0088 /* Specific1 Bottom */
#define FT_ETH_SA1T				0x008C /* Specific1 Top */
#define FT_ETH_SA2B				0x0090 /* Specific2 Bottom */
#define FT_ETH_SA2T				0x0094 /* Specific2 Top */
#define FT_ETH_SA3B				0x0098 /* Specific3 Bottom */
#define FT_ETH_SA3T				0x009C /* Specific3 Top */
#define FT_ETH_SA4B				0x00A0 /* Specific4 Bottom */
#define FT_ETH_SA4T				0x00A4 /* Specific4 Top */
#define FT_ETH_EFTSH			0x00e8 /* PTP Event Frame Transmitted Seconds Register 47:32 */
#define FT_ETH_EFRSH			0x00ec /* PTP Event Frame Received Seconds Register 47:32 */
#define FT_ETH_PEFTSH			0x00f0 /* PTP Peer Event Frame Transmitted Seconds Register 47:32 */
#define FT_ETH_PEFRSH			0x00f4 /* PTP Peer Event Frame Received Seconds Register 47:32 */
#define FT_ETH_OTX				0x0100 /* Octets transmitted */
#define FT_ETH_OCTTXL			0x0100 /* Octets transmitted [31:0] */
#define FT_ETH_OCTTXH			0x0104 /* Octets transmitted [47:32] */
#define FT_ETH_TXCNT			0x0108 /* Frames Transmitted counter */
#define FT_ETH_TXBCCNT			0x010c /* Broadcast Frames counter */
#define FT_ETH_TXMCCNT			0x0110 /* Multicast Frames counter */
#define FT_ETH_TXPAUSECNT		0x0114 /* Pause Frames Transmitted Counter */
#define FT_ETH_TX64CNT			0x0118 /* 64 byte Frames TX counter */
#define FT_ETH_TX65CNT			0x011c /* 65-127 byte Frames TX counter */
#define FT_ETH_TX128CNT			0x0120 /* 128-255 byte Frames TX counter */
#define FT_ETH_TX256CNT			0x0124 /* 256-511 byte Frames TX counter */
#define FT_ETH_TX512CNT			0x0128 /* 512-1023 byte Frames TX counter */
#define FT_ETH_TX1024CNT		0x012c /* 1024-1518 byte Frames TX counter */
#define FT_ETH_TX1519CNT		0x0130 /* 1519+ byte Frames TX counter */
#define FT_ETH_TXURUNCNT		0x0134 /* TX under run error counter */
#define FT_ETH_SNGLCOLLCNT		0x0138 /* Single Collision Frame Counter */
#define FT_ETH_MULTICOLLCNT		0x013c /* Multiple Collision Frame Counter */
#define FT_ETH_EXCESSCOLLCNT	0x0140 /* Excessive Collision Frame Counter */
#define FT_ETH_LATECOLLCNT		0x0144 /* Late Collision Frame Counter */
#define FT_ETH_TXDEFERCNT		0x0148 /* Deferred Transmission Frame Counter */
#define FT_ETH_TXCSENSECNT		0x014c /* Carrier Sense Error Counter */
#define FT_ETH_ORX				0x0150 /* Octets received */
#define FT_ETH_OCTRXL			0x0150 /* Octets received [31:0] */
#define FT_ETH_OCTRXH			0x0154 /* Octets received [47:32] */
#define FT_ETH_RXCNT			0x0158 /* Frames Received Counter */
#define FT_ETH_RXBROADCNT		0x015c /* Broadcast Frames Received Counter */
#define FT_ETH_RXMULTICNT		0x0160 /* Multicast Frames Received Counter */
#define FT_ETH_RXPAUSECNT		0x0164 /* Pause Frames Received Counter */
#define FT_ETH_RX64CNT			0x0168 /* 64 byte Frames RX Counter */
#define FT_ETH_RX65CNT			0x016c /* 65-127 byte Frames RX Counter */
#define FT_ETH_RX128CNT			0x0170 /* 128-255 byte Frames RX Counter */
#define FT_ETH_RX256CNT			0x0174 /* 256-511 byte Frames RX Counter */
#define FT_ETH_RX512CNT			0x0178 /* 512-1023 byte Frames RX Counter */
#define FT_ETH_RX1024CNT		0x017c /* 1024-1518 byte Frames RX Counter */
#define FT_ETH_RX1519CNT		0x0180 /* 1519+ byte Frames RX Counter */
#define FT_ETH_RXUNDRCNT		0x0184 /* Undersize Frames Received Counter */
#define FT_ETH_RXOVRCNT			0x0188 /* Oversize Frames Received Counter */
#define FT_ETH_RXJABCNT			0x018c /* Jabbers Received Counter */
#define FT_ETH_RXFCSCNT			0x0190 /* Frame Check Sequence Error Counter */
#define FT_ETH_RXLENGTHCNT		0x0194 /* Length Field Error Counter */
#define FT_ETH_RXSYMBCNT		0x0198 /* Symbol Error Counter */
#define FT_ETH_RXALIGNCNT		0x019c /* Alignment Error Counter */
#define FT_ETH_RXRESERRCNT		0x01a0 /* Receive Resource Error Counter */
#define FT_ETH_RXORCNT			0x01a4 /* Receive Overrun Counter */
#define FT_ETH_RXIPCCNT			0x01a8 /* IP header Checksum Error Counter */
#define FT_ETH_RXTCPCCNT		0x01ac /* TCP Checksum Error Counter */
#define FT_ETH_RXUDPCCNT		0x01b0 /* UDP Checksum Error Counter */
#define FT_ETH_TISUBN			0x01bc /* 1588 Timer Increment Sub-ns */
#define FT_ETH_TSH				0x01c0 /* 1588 Timer Seconds High */
#define FT_ETH_TSL				0x01d0 /* 1588 Timer Seconds Low */
#define FT_ETH_TN				0x01d4 /* 1588 Timer Nanoseconds */
#define FT_ETH_TA				0x01d8 /* 1588 Timer Adjust */
#define FT_ETH_TI				0x01dc /* 1588 Timer Increment */
#define FT_ETH_EFTSL			0x01e0 /* PTP Event Frame Tx Seconds Low */
#define FT_ETH_EFTN				0x01e4 /* PTP Event Frame Tx Nanoseconds */
#define FT_ETH_EFRSL			0x01e8 /* PTP Event Frame Rx Seconds Low */
#define FT_ETH_EFRN				0x01ec /* PTP Event Frame Rx Nanoseconds */
#define FT_ETH_PEFTSL			0x01f0 /* PTP Peer Event Frame Tx Secs Low */
#define FT_ETH_PEFTN			0x01f4 /* PTP Peer Event Frame Tx Ns */
#define FT_ETH_PEFRSL			0x01f8 /* PTP Peer Event Frame Rx Sec Low */
#define FT_ETH_PEFRN			0x01fc /* PTP Peer Event Frame Rx Ns */
#define FT_ETH_DCFG1			0x0280 /* Design Config 1 */
#define FT_ETH_DCFG2			0x0284 /* Design Config 2 */
#define FT_ETH_DCFG3			0x0288 /* Design Config 3 */
#define FT_ETH_DCFG4			0x028c /* Design Config 4 */
#define FT_ETH_DCFG5			0x0290 /* Design Config 5 */
#define FT_ETH_DCFG6			0x0294 /* Design Config 6 */
#define FT_ETH_DCFG7			0x0298 /* Design Config 7 */
#define FT_ETH_DCFG8			0x029C /* Design Config 8 */
#define FT_ETH_DCFG10			0x02A4 /* Design Config 10 */

#define FT_ETH_TXBDCTRL			0x04cc /* TX Buffer Descriptor control register */
#define FT_ETH_RXBDCTRL			0x04d0 /* RX Buffer Descriptor control register */

/* Screener Type 2 match registers */
#define FT_ETH_SCRT2			0x540

/* EtherType registers */
#define FT_ETH_ETHT				0x06E0

/* Type 2 compare registers */
#define FT_ETH_T2CMPW0			0x0700
#define FT_ETH_T2CMPW1			0x0704
#define T2CMP_OFST(t2idx)		(t2idx * 2)

/* type 2 compare registers
 * each location requires 3 compare regs
 */
#define FT_ETH_IP4SRC_CMP(idx)	(idx * 3)
#define FT_ETH_IP4DST_CMP(idx)	(idx * 3 + 1)
#define FT_ETH_PORT_CMP(idx)	(idx * 3 + 2)

/* Which screening type 2 EtherType register will be used (0 - 7) */
#define SCRT2_ETHT				0

#define FT_ETH_ISR(hw_q)		(0x0400 + ((hw_q) << 2))
#define FT_ETH_TBQP(hw_q)		(0x0440 + ((hw_q) << 2))
#define FT_ETH_TBQPH(hw_q)		(0x04C8)
#define FT_ETH_RBQP(hw_q)		(0x0480 + ((hw_q) << 2))
#define FT_ETH_RBQS(hw_q)		(0x04A0 + ((hw_q) << 2))
#define FT_ETH_RBQPH(hw_q)		(0x04D4)
#define FT_ETH_IER(hw_q)		(0x0600 + ((hw_q) << 2))
#define FT_ETH_IDR(hw_q)		(0x0620 + ((hw_q) << 2))
#define FT_ETH_IMR(hw_q)		(0x0640 + ((hw_q) << 2))

/* Bitfields in NCR */
#define FT_LB_OFFSET		0 /* reserved */
#define FT_LB_SIZE			1
#define FT_LLB_OFFSET		1 /* Loop back local */
#define FT_LLB_SIZE			1
#define FT_RE_OFFSET		2 /* Receive enable */
#define FT_RE_SIZE			1
#define FT_TE_OFFSET		3 /* Transmit enable */
#define FT_TE_SIZE			1
#define FT_MPE_OFFSET		4 /* Management port enable */
#define FT_MPE_SIZE			1
#define FT_CLRSTAT_OFFSET	5 /* Clear stats regs */
#define FT_CLRSTAT_SIZE		1
#define FT_INCSTAT_OFFSET	6 /* Incremental stats regs */
#define FT_INCSTAT_SIZE		1
#define FT_WESTAT_OFFSET	7 /* Write enable stats regs */
#define FT_WESTAT_SIZE		1
#define FT_BP_OFFSET		8 /* Back pressure */
#define FT_BP_SIZE			1
#define FT_TSTART_OFFSET	9 /* Start transmission */
#define FT_TSTART_SIZE		1
#define FT_THALT_OFFSET		10 /* Transmit halt */
#define FT_THALT_SIZE		1
#define FT_NCR_TPF_OFFSET	11 /* Transmit pause frame */
#define FT_NCR_TPF_SIZE		1
#define FT_TZQ_OFFSET		12 /* Transmit zero quantum pause frame */
#define FT_TZQ_SIZE			1
#define FT_SRTSM_OFFSET		15
#define FT_OSSMODE_OFFSET 	24 /* Enable One Step Synchro Mode */
#define FT_OSSMODE_SIZE		1

/* Bitfields in NCFGR */
#define FT_SPD_OFFSET		0 /* Speed */
#define FT_SPD_SIZE			1
#define FT_FD_OFFSET		1 /* Full duplex */
#define FT_FD_SIZE			1
#define FT_BIT_RATE_OFFSET	2 /* Discard non-VLAN frames */
#define FT_BIT_RATE_SIZE	1
#define FT_JFRAME_OFFSET	3 /* reserved */
#define FT_JFRAME_SIZE		1
#define FT_CAF_OFFSET		4 /* Copy all frames */
#define FT_CAF_SIZE			1
#define FT_NBC_OFFSET		5 /* No broadcast */
#define FT_NBC_SIZE			1
#define FT_NCFGR_MTI_OFFSET	6 /* Multicast hash enable */
#define FT_NCFGR_MTI_SIZE	1
#define FT_UNI_OFFSET		7 /* Unicast hash enable */
#define FT_UNI_SIZE			1
#define FT_BIG_OFFSET		8 /* Receive 1536 byte frames */
#define FT_BIG_SIZE			1
#define FT_EAE_OFFSET		9 /* External address match enable */
#define FT_EAE_SIZE			1
#define FT_CLK_OFFSET		10
#define FT_CLK_SIZE			2
#define FT_RTY_OFFSET		12 /* Retry test */
#define FT_RTY_SIZE			1
#define FT_PAE_OFFSET		13 /* Pause enable */
#define FT_PAE_SIZE			1
#define FT_RM9200_RMII_OFFSET	13 /* AT91RM9200 only */
#define FT_RM9200_RMII_SIZE	1  /* AT91RM9200 only */
#define FT_RBOF_OFFSET		14 /* Receive buffer offset */
#define FT_RBOF_SIZE		2
#define FT_RLCE_OFFSET		16 /* Length field error frame discard */
#define FT_RLCE_SIZE		1
#define FT_DRFCS_OFFSET		17 /* FCS remove */
#define FT_DRFCS_SIZE		1
#define FT_EFRHD_OFFSET		18
#define FT_EFRHD_SIZE		1
#define FT_IRXFCS_OFFSET	19
#define FT_IRXFCS_SIZE		1

/* FT_ETH specific NCFGR bitfields. */
#define FT_ETH_GBE_OFFSET		10 /* Gigabit mode enable */
#define FT_ETH_GBE_SIZE			1
#define FT_ETH_PCSSEL_OFFSET	11
#define FT_ETH_PCSSEL_SIZE		1
#define FT_ETH_CLK_OFFSET		18 /* MDC clock division */
#define FT_ETH_CLK_SIZE			3
#define FT_ETH_DBW_OFFSET		21 /* Data bus width */
#define FT_ETH_DBW_SIZE			2
#define FT_ETH_RXCOEN_OFFSET	24
#define FT_ETH_RXCOEN_SIZE		1
#define FT_ETH_SGMIIEN_OFFSET	27
#define FT_ETH_SGMIIEN_SIZE		1


/* Constants for data bus width. */
#define FT_ETH_DBW32			0 /* 32 bit AMBA AHB data bus width */
#define FT_ETH_DBW64			1 /* 64 bit AMBA AHB data bus width */
#define FT_ETH_DBW128			2 /* 128 bit AMBA AHB data bus width */

/* Bitfields in DMACFG. */
#define FT_ETH_FBLDO_OFFSET		0 /* fixed burst length for DMA */
#define FT_ETH_FBLDO_SIZE		5
#define FT_ETH_ENDIA_DESC_OFFSET	6 /* endian swap mode for management descriptor access */
#define FT_ETH_ENDIA_DESC_SIZE	1
#define FT_ETH_ENDIA_PKT_OFFSET	7 /* endian swap mode for packet data access */
#define FT_ETH_ENDIA_PKT_SIZE	1
#define FT_ETH_RXBMS_OFFSET		8 /* RX packet buffer memory size select */
#define FT_ETH_RXBMS_SIZE		2
#define FT_ETH_TXPBMS_OFFSET	10 /* TX packet buffer memory size select */
#define FT_ETH_TXPBMS_SIZE		1
#define FT_ETH_TXCOEN_OFFSET	11 /* TX IP/TCP/UDP checksum gen offload */
#define FT_ETH_TXCOEN_SIZE		1
#define FT_ETH_RXBS_OFFSET		16 /* DMA receive buffer size */
#define FT_ETH_RXBS_SIZE		8
#define FT_ETH_DDRP_OFFSET		24 /* disc_when_no_ahb */
#define FT_ETH_DDRP_SIZE		1
#define FT_ETH_RXEXT_OFFSET		28 /* RX extended Buffer Descriptor mode */
#define FT_ETH_RXEXT_SIZE		1
#define FT_ETH_TXEXT_OFFSET		29 /* TX extended Buffer Descriptor mode */
#define FT_ETH_TXEXT_SIZE		1
#define FT_ETH_ADDR64_OFFSET	30 /* Address bus width - 64b or 32b */
#define FT_ETH_ADDR64_SIZE		1


/* Bitfields in NSR */
#define FT_NSR_LINK_OFFSET	0 /* pcs_link_state */
#define FT_NSR_LINK_SIZE	1
#define FT_MDIO_OFFSET		1 /* status of the mdio_in pin */
#define FT_MDIO_SIZE		1
#define FT_IDLE_OFFSET		2 /* The PHY management logic is idle */
#define FT_IDLE_SIZE		1

/* Bitfields in TSR */
#define FT_UBR_OFFSET		0 /* Used bit read */
#define FT_UBR_SIZE			1
#define FT_COL_OFFSET		1 /* Collision occurred */
#define FT_COL_SIZE			1
#define FT_TSR_RLE_OFFSET	2 /* Retry limit exceeded */
#define FT_TSR_RLE_SIZE		1
#define FT_TGO_OFFSET		3 /* Transmit go */
#define FT_TGO_SIZE			1
#define FT_BEX_OFFSET		4 /* TX frame corruption due to AHB error */
#define FT_BEX_SIZE			1
#define FT_RM9200_BNQ_OFFSET	4 /* AT91RM9200 only */
#define FT_RM9200_BNQ_SIZE	1 /* AT91RM9200 only */
#define FT_COMP_OFFSET		5 /* Trnasmit complete */
#define FT_COMP_SIZE		1
#define FT_UND_OFFSET		6 /* Trnasmit under run */
#define FT_UND_SIZE			1

/* Bitfields in RSR */
#define FT_BNA_OFFSET		0 /* Buffer not available */
#define FT_BNA_SIZE			1
#define FT_REC_OFFSET		1 /* Frame received */
#define FT_REC_SIZE			1
#define FT_OVR_OFFSET		2 /* Receive overrun */
#define FT_OVR_SIZE			1

/* Bitfields in ISR/IER/IDR/IMR */
#define FT_MFD_OFFSET		0 /* Management frame sent */
#define FT_MFD_SIZE			1
#define FT_RCOMP_OFFSET		1 /* Receive complete */
#define FT_RCOMP_SIZE		1
#define FT_RXUBR_OFFSET		2 /* RX used bit read */
#define FT_RXUBR_SIZE		1
#define FT_TXUBR_OFFSET		3 /* TX used bit read */
#define FT_TXUBR_SIZE		1
#define FT_ISR_TUND_OFFSET	4 /* Enable TX buffer under run interrupt */
#define FT_ISR_TUND_SIZE	1
#define FT_ISR_RLE_OFFSET	5 /* EN retry exceeded/late coll interrupt */
#define FT_ISR_RLE_SIZE		1
#define FT_TXERR_OFFSET		6 /* EN TX frame corrupt from error interrupt */
#define FT_TXERR_SIZE		1
#define FT_TCOMP_OFFSET		7 /* Enable transmit complete interrupt */
#define FT_TCOMP_SIZE		1
#define FT_ISR_LINK_OFFSET	9 /* Enable link change interrupt */
#define FT_ISR_LINK_SIZE	1
#define FT_ISR_ROVR_OFFSET	10 /* Enable receive overrun interrupt */
#define FT_ISR_ROVR_SIZE	1
#define FT_HRESP_OFFSET		11 /* Enable hrsep not OK interrupt */
#define FT_HRESP_SIZE		1
#define FT_PFR_OFFSET		12 /* Enable pause frame w/ quantum interrupt */
#define FT_PFR_SIZE			1
#define FT_PTZ_OFFSET		13 /* Enable pause time zero interrupt */
#define FT_PTZ_SIZE			1
#define FT_WOL_OFFSET		14 /* Enable wake-on-lan interrupt */
#define FT_WOL_SIZE			1
#define FT_DRQFR_OFFSET		18 /* PTP Delay Request Frame Received */
#define FT_DRQFR_SIZE		1
#define FT_SFR_OFFSET		19 /* PTP Sync Frame Received */
#define FT_SFR_SIZE			1
#define FT_DRQFT_OFFSET		20 /* PTP Delay Request Frame Transmitted */
#define FT_DRQFT_SIZE		1
#define FT_SFT_OFFSET		21 /* PTP Sync Frame Transmitted */
#define FT_SFT_SIZE			1
#define FT_PDRQFR_OFFSET	22 /* PDelay Request Frame Received */
#define FT_PDRQFR_SIZE		1
#define FT_PDRSFR_OFFSET	23 /* PDelay Response Frame Received */
#define FT_PDRSFR_SIZE		1
#define FT_PDRQFT_OFFSET	24 /* PDelay Request Frame Transmitted */
#define FT_PDRQFT_SIZE		1
#define FT_PDRSFT_OFFSET	25 /* PDelay Response Frame Transmitted */
#define FT_PDRSFT_SIZE		1
#define FT_SRI_OFFSET		26 /* TSU Seconds Register Increment */
#define FT_SRI_SIZE			1

/* Timer increment fields */
#define FT_TI_CNS_OFFSET	0
#define FT_TI_CNS_SIZE		8
#define FT_TI_ACNS_OFFSET	8
#define FT_TI_ACNS_SIZE		8
#define FT_TI_NIT_OFFSET	16
#define FT_TI_NIT_SIZE		8

/* Bitfields in MAN */
#define FT_DATA_OFFSET		0 /* data */
#define FT_DATA_SIZE		16
#define FT_CODE_OFFSET		16 /* Must be written to 10 */
#define FT_CODE_SIZE		2
#define FT_REGA_OFFSET		18 /* Register address */
#define FT_REGA_SIZE		5
#define FT_PHYA_OFFSET		23 /* PHY address */
#define FT_PHYA_SIZE		5
#define FT_RW_OFFSET		28 /* Operation. 10 is read. 01 is write. */
#define FT_RW_SIZE			2
#define FT_SOF_OFFSET		30 /* Must be written to 1 for Clause 22 */
#define FT_SOF_SIZE			2

/* Bitfields in USRIO (AVR32) */
#define FT_MII_OFFSET				0
#define FT_MII_SIZE					1
#define FT_EAM_OFFSET				1
#define FT_EAM_SIZE					1
#define FT_TX_PAUSE_OFFSET			2
#define FT_TX_PAUSE_SIZE			1
#define FT_TX_PAUSE_ZERO_OFFSET		3
#define FT_TX_PAUSE_ZERO_SIZE		1

/* Bitfields in USRIO (AT91) */
#define FT_RMII_OFFSET				0
#define FT_RMII_SIZE				1
#define FT_ETH_RGMII_OFFSET			0 /* FT_ETH gigabit mode */
#define FT_ETH_RGMII_SIZE			1
#define FT_CLKEN_OFFSET				1
#define FT_CLKEN_SIZE				1

/* Bitfields in WOL */
#define FT_IP_OFFSET				0
#define FT_IP_SIZE					16
#define FT_MAG_OFFSET				16
#define FT_MAG_SIZE					1
#define FT_ARP_OFFSET				17
#define FT_ARP_SIZE					1
#define FT_SA1_OFFSET				18
#define FT_SA1_SIZE					1
#define FT_WOL_MTI_OFFSET			19
#define FT_WOL_MTI_SIZE				1

/* Bitfields in MID */
#define FT_IDNUM_OFFSET				16
#define FT_IDNUM_SIZE				12
#define FT_REV_OFFSET				0
#define FT_REV_SIZE					16

/* Bitfields in DCFG1. */
#define FT_ETH_IRQCOR_OFFSET		23
#define FT_ETH_IRQCOR_SIZE			1
#define FT_ETH_DBWDEF_OFFSET		25
#define FT_ETH_DBWDEF_SIZE			3

/* Bitfields in DCFG2. */
#define FT_ETH_RX_PKT_BUFF_OFFSET	20
#define FT_ETH_RX_PKT_BUFF_SIZE		1
#define FT_ETH_TX_PKT_BUFF_OFFSET	21
#define FT_ETH_TX_PKT_BUFF_SIZE		1


/* Bitfields in DCFG5. */
#define FT_ETH_TSU_OFFSET			8
#define FT_ETH_TSU_SIZE				1

/* Bitfields in DCFG6. */
#define FT_ETH_PBUF_LSO_OFFSET		27
#define FT_ETH_PBUF_LSO_SIZE		1
#define FT_ETH_DAW64_OFFSET			23
#define FT_ETH_DAW64_SIZE			1

/* Bitfields in DCFG8. */
#define FT_ETH_T1SCR_OFFSET			24
#define FT_ETH_T1SCR_SIZE			8
#define FT_ETH_T2SCR_OFFSET			16
#define FT_ETH_T2SCR_SIZE			8
#define FT_ETH_SCR2ETH_OFFSET		8
#define FT_ETH_SCR2ETH_SIZE			8
#define FT_ETH_SCR2CMP_OFFSET		0
#define FT_ETH_SCR2CMP_SIZE			8

/* Bitfields in DCFG10 */
#define FT_ETH_TXBD_RDBUFF_OFFSET	12
#define FT_ETH_TXBD_RDBUFF_SIZE		4
#define FT_ETH_RXBD_RDBUFF_OFFSET	8
#define FT_ETH_RXBD_RDBUFF_SIZE		4

/* Bitfields in TISUBN */
#define FT_ETH_SUBNSINCR_OFFSET		0
#define FT_ETH_SUBNSINCR_SIZE		16

/* Bitfields in TI */
#define FT_ETH_NSINCR_OFFSET		0
#define FT_ETH_NSINCR_SIZE			8

/* Bitfields in TSH */
#define FT_ETH_TSH_OFFSET			0 /* TSU timer value (s). MSB [47:32] of seconds timer count */
#define FT_ETH_TSH_SIZE				16

/* Bitfields in TSL */
#define FT_ETH_TSL_OFFSET			0 /* TSU timer value (s). LSB [31:0] of seconds timer count */
#define FT_ETH_TSL_SIZE				32

/* Bitfields in TN */
#define FT_ETH_TN_OFFSET			0 /* TSU timer value (ns) */
#define FT_ETH_TN_SIZE				30

/* Bitfields in TXBDCTRL */
#define FT_ETH_TXTSMODE_OFFSET		4 /* TX Descriptor Timestamp Insertion mode */
#define FT_ETH_TXTSMODE_SIZE		2

/* Bitfields in RXBDCTRL */
#define FT_ETH_RXTSMODE_OFFSET		4 /* RX Descriptor Timestamp Insertion mode */
#define FT_ETH_RXTSMODE_SIZE		2

/* Bitfields in SCRT2 */
#define FT_ETH_QUEUE_OFFSET			0 /* Queue Number */
#define FT_ETH_QUEUE_SIZE			4
#define FT_ETH_VLANPR_OFFSET		4 /* VLAN Priority */
#define FT_ETH_VLANPR_SIZE			3
#define FT_ETH_VLANEN_OFFSET		8 /* VLAN Enable */
#define FT_ETH_VLANEN_SIZE			1
#define FT_ETH_ETHT2IDX_OFFSET		9 /* Index to screener type 2 EtherType register */
#define FT_ETH_ETHT2IDX_SIZE		3
#define FT_ETH_ETHTEN_OFFSET		12 /* EtherType Enable */
#define FT_ETH_ETHTEN_SIZE			1
#define FT_ETH_CMPA_OFFSET			13 /* Compare A - Index to screener type 2 Compare register */
#define FT_ETH_CMPA_SIZE			5
#define FT_ETH_CMPAEN_OFFSET		18 /* Compare A Enable */
#define FT_ETH_CMPAEN_SIZE			1
#define FT_ETH_CMPB_OFFSET			19 /* Compare B - Index to screener type 2 Compare register */
#define FT_ETH_CMPB_SIZE			5
#define FT_ETH_CMPBEN_OFFSET		24 /* Compare B Enable */
#define FT_ETH_CMPBEN_SIZE			1
#define FT_ETH_CMPC_OFFSET			25 /* Compare C - Index to screener type 2 Compare register */
#define FT_ETH_CMPC_SIZE			5
#define FT_ETH_CMPCEN_OFFSET		30 /* Compare C Enable */
#define FT_ETH_CMPCEN_SIZE			1

/* Bitfields in ETHT */
#define FT_ETH_ETHTCMP_OFFSET		0 /* EtherType compare value */
#define FT_ETH_ETHTCMP_SIZE			16

/* Bitfields in T2CMPW0 */
#define FT_ETH_T2CMP_OFFSET			16 /* 0xFFFF0000 compare value */
#define FT_ETH_T2CMP_SIZE			16
#define FT_ETH_T2MASK_OFFSET		0 /* 0x0000FFFF compare value or mask */
#define FT_ETH_T2MASK_SIZE			16

/* Bitfields in T2CMPW1 */
#define FT_ETH_T2DISMSK_OFFSET		9 /* disable mask */
#define FT_ETH_T2DISMSK_SIZE		1
#define FT_ETH_T2CMPOFST_OFFSET		7 /* compare offset */
#define FT_ETH_T2CMPOFST_SIZE		2
#define FT_ETH_T2OFST_OFFSET		0 /* offset value */
#define FT_ETH_T2OFST_SIZE			7

/* Offset for screener type 2 compare values (T2CMPOFST).
 * Note the offset is applied after the specified point,
 * e.g. FT_ETH_T2COMPOFST_ETYPE denotes the EtherType field, so an offset
 * of 12 bytes from this would be the source IP address in an IP header
 */
#define FT_ETH_T2COMPOFST_SOF		0
#define FT_ETH_T2COMPOFST_ETYPE		1
#define FT_ETH_T2COMPOFST_IPHDR		2
#define FT_ETH_T2COMPOFST_TCPUDP	3

/* offset from EtherType to IP address */
#define ETYPE_SRCIP_OFFSET			12
#define ETYPE_DSTIP_OFFSET			16

/* offset from IP header to port */
#define IPHDR_SRCPORT_OFFSET		0
#define IPHDR_DSTPORT_OFFSET		2

/* Transmit DMA buffer descriptor Word 1 */
#define FT_ETH_DMA_TXVALID_OFFSET	23 /* timestamp has been captured in the Buffer Descriptor */
#define FT_ETH_DMA_TXVALID_SIZE		1

/* Receive DMA buffer descriptor Word 0 */
#define FT_ETH_DMA_RXVALID_OFFSET	2 /* indicates a valid timestamp in the Buffer Descriptor */
#define FT_ETH_DMA_RXVALID_SIZE		1

/* DMA buffer descriptor Word 2 (32 bit addressing) or Word 4 (64 bit addressing) */
#define FT_ETH_DMA_SECL_OFFSET		30 /* Timestamp seconds[1:0]  */
#define FT_ETH_DMA_SECL_SIZE		2
#define FT_ETH_DMA_NSEC_OFFSET		0 /* Timestamp nanosecs [29:0] */
#define FT_ETH_DMA_NSEC_SIZE		30

/* DMA buffer descriptor Word 3 (32 bit addressing) or Word 5 (64 bit addressing) */

/* New hardware supports 12 bit precision of timestamp in DMA buffer descriptor.
 * Old hardware supports only 6 bit precision but it is enough for PTP.
 * Less accuracy is used always instead of checking hardware version.
 */
#define FT_ETH_DMA_SECH_OFFSET		0 /* Timestamp seconds[5:2] */
#define FT_ETH_DMA_SECH_SIZE		4
#define FT_ETH_DMA_SEC_WIDTH		(FT_ETH_DMA_SECH_SIZE + FT_ETH_DMA_SECL_SIZE)
#define FT_ETH_DMA_SEC_TOP			(1 << FT_ETH_DMA_SEC_WIDTH)
#define FT_ETH_DMA_SEC_MASK			(FT_ETH_DMA_SEC_TOP - 1)

/* Bitfields in ADJ */
#define FT_ETH_ADDSUB_OFFSET		31
#define FT_ETH_ADDSUB_SIZE			1
/* Constants for CLK */
#define FT_CLK_DIV8					0
#define FT_CLK_DIV16				1
#define FT_CLK_DIV32				2
#define FT_CLK_DIV64				3

/* FT_ETH specific constants for CLK */
#define FT_ETH_CLK_DIV8				0
#define FT_ETH_CLK_DIV16			1
#define FT_ETH_CLK_DIV32			2
#define FT_ETH_CLK_DIV48			3
#define FT_ETH_CLK_DIV64			4
#define FT_ETH_CLK_DIV96			5
#define FT_ETH_CLK_DIV128			6
#define FT_ETH_CLK_DIV224			7

/* Constants for MAN register */
#define FT_MAN_SOF				1
#define FT_MAN_WRITE			1
#define FT_MAN_READ				2
#define FT_MAN_CODE				2

/* Capability mask bits */
#define FT_CAPS_ISR_CLEAR_ON_WRITE			0x00000001
#define FT_CAPS_USRIO_HAS_CLKEN				0x00000002
#define FT_CAPS_USRIO_DEFAULT_IS_MII_GMII	0x00000004
#define FT_CAPS_NO_GIGABIT_HALF				0x00000008
#define FT_CAPS_USRIO_DISABLED				0x00000010
#define FT_CAPS_JUMBO						0x00000020
#define FT_CAPS_FT_ETH_HAS_PTP				0x00000040
#define FT_CAPS_BD_RD_PREFETCH				0x00000080
#define FT_CAPS_NEEDS_RSTONUBR				0x00000100
#define FT_CAPS_FIFO_MODE					0x10000000
#define FT_CAPS_GIGABIT_MODE_AVAILABLE		0x20000000
#define FT_CAPS_SG_DISABLED					0x40000000
#define FT_CAPS_FT_IS_FT_ETH				0x80000000

/* LSO settings */
#define FT_LSO_UFO_ENABLE			0x01
#define FT_LSO_TSO_ENABLE			0x02

/* Bit manipulation macros */
#define FT_BIT(name)					\
	(1 << FT_##name##_OFFSET)
#define FT_BF(name,value)				\
	(((value) & ((1 << FT_##name##_SIZE) - 1))	\
	 << FT_##name##_OFFSET)
#define FT_BFEXT(name,value)\
	(((value) >> FT_##name##_OFFSET)		\
	 & ((1 << FT_##name##_SIZE) - 1))
#define FT_BFINS(name,value,old)			\
	(((old) & ~(((1 << FT_##name##_SIZE) - 1)	\
		    << FT_##name##_OFFSET))		\
	 | FT_BF(name,value))

#define FT_ETH_BIT(name)					\
	(1 << FT_ETH_##name##_OFFSET)
#define FT_ETH_BF(name, value)				\
	(((value) & ((1 << FT_ETH_##name##_SIZE) - 1))	\
	 << FT_ETH_##name##_OFFSET)
#define FT_ETH_BFEXT(name, value)\
	(((value) >> FT_ETH_##name##_OFFSET)		\
	 & ((1 << FT_ETH_##name##_SIZE) - 1))
#define FT_ETH_BFINS(name, value, old)			\
	(((old) & ~(((1 << FT_ETH_##name##_SIZE) - 1)	\
		    << FT_ETH_##name##_OFFSET))		\
	 | FT_ETH_BF(name, value))

/* Register access macros */
#define ft_readl(port, reg)				\
	readl((port)->regs + FT_##reg)
#define ft_writel(port, reg, value)			\
	writel((value), (port)->regs + FT_##reg)
#define ft_eth_readl(port, reg)				\
	readl((port)->regs + FT_ETH_##reg)
#define ft_eth_writel(port, reg, value)			\
	writel((value), (port)->regs + FT_ETH_##reg)

/* DMA descriptor bitfields */
#define FT_RX_USED_OFFSET			0
#define FT_RX_USED_SIZE				1
#define FT_RX_WRAP_OFFSET			1
#define FT_RX_WRAP_SIZE				1
#define FT_RX_WADDR_OFFSET			2
#define FT_RX_WADDR_SIZE			30

#define FT_RX_FRMLEN_OFFSET			0
#define FT_RX_FRMLEN_SIZE			12
#define FT_RX_OFFSET_OFFSET			12
#define FT_RX_OFFSET_SIZE			2
#define FT_RX_SOF_OFFSET			14
#define FT_RX_SOF_SIZE				1
#define FT_RX_EOF_OFFSET			15
#define FT_RX_EOF_SIZE				1
#define FT_RX_CFI_OFFSET			16
#define FT_RX_CFI_SIZE				1
#define FT_RX_VLAN_PRI_OFFSET		17
#define FT_RX_VLAN_PRI_SIZE			3
#define FT_RX_PRI_TAG_OFFSET		20
#define FT_RX_PRI_TAG_SIZE			1
#define FT_RX_VLAN_TAG_OFFSET		21
#define FT_RX_VLAN_TAG_SIZE			1
#define FT_RX_TYPEID_MATCH_OFFSET	22
#define FT_RX_TYPEID_MATCH_SIZE		1
#define FT_RX_SA4_MATCH_OFFSET		23
#define FT_RX_SA4_MATCH_SIZE		1
#define FT_RX_SA3_MATCH_OFFSET		24
#define FT_RX_SA3_MATCH_SIZE		1
#define FT_RX_SA2_MATCH_OFFSET		25
#define FT_RX_SA2_MATCH_SIZE		1
#define FT_RX_SA1_MATCH_OFFSET		26
#define FT_RX_SA1_MATCH_SIZE		1
#define FT_RX_EXT_MATCH_OFFSET		28
#define FT_RX_EXT_MATCH_SIZE		1
#define FT_RX_UHASH_MATCH_OFFSET	29
#define FT_RX_UHASH_MATCH_SIZE		1
#define FT_RX_MHASH_MATCH_OFFSET	30
#define FT_RX_MHASH_MATCH_SIZE		1
#define FT_RX_BROADCAST_OFFSET		31
#define FT_RX_BROADCAST_SIZE		1

#define FT_RX_FRMLEN_MASK			0xFFF
#define FT_RX_JFRMLEN_MASK			0x3FFF

/* RX checksum offload disabled: bit 24 clear in NCFGR */
#define FT_ETH_RX_TYPEID_MATCH_OFFSET	22
#define FT_ETH_RX_TYPEID_MATCH_SIZE		2

/* RX checksum offload enabled: bit 24 set in NCFGR */
#define FT_ETH_RX_CSUM_OFFSET			22
#define FT_ETH_RX_CSUM_SIZE				2

#define FT_TX_FRMLEN_OFFSET				0
#define FT_TX_FRMLEN_SIZE				11
#define FT_TX_LAST_OFFSET				15
#define FT_TX_LAST_SIZE					1
#define FT_TX_NOCRC_OFFSET				16
#define FT_TX_NOCRC_SIZE				1
#define FT_MSS_MFS_OFFSET				16
#define FT_MSS_MFS_SIZE					14
#define FT_TX_LSO_OFFSET				17
#define FT_TX_LSO_SIZE					2
#define FT_TX_TCP_SEQ_SRC_OFFSET		19
#define FT_TX_TCP_SEQ_SRC_SIZE			1
#define FT_TX_BUF_EXHAUSTED_OFFSET		27
#define FT_TX_BUF_EXHAUSTED_SIZE		1
#define FT_TX_UNDERRUN_OFFSET			28
#define FT_TX_UNDERRUN_SIZE				1
#define FT_TX_ERROR_OFFSET				29
#define FT_TX_ERROR_SIZE				1
#define FT_TX_WRAP_OFFSET				30
#define FT_TX_WRAP_SIZE					1
#define FT_TX_USED_OFFSET				31
#define FT_TX_USED_SIZE					1

#define FT_ETH_TX_FRMLEN_OFFSET			0
#define FT_ETH_TX_FRMLEN_SIZE			14

/* Buffer descriptor constants */
#define FT_ETH_RX_CSUM_NONE				0
#define FT_ETH_RX_CSUM_IP_ONLY			1
#define FT_ETH_RX_CSUM_IP_TCP			2
#define FT_ETH_RX_CSUM_IP_UDP			3

/*Phytium E2000 platform*/
#define FT_ETH_SRC_SEL_LN					0x1c04
#define FT_ETH_DIV_SEL0_LN					0x1c08
#define FT_ETH_DIV_SEL1_LN					0x1c0c
#define FT_ETH_PMA_XCVR_POWER_STATE			0x1c10
#define FT_ETH_MII_SELECT					0x1c18
#define FT_ETH_SEL_MII_ON_RGMII				0x1c1c
#define FT_ETH_TX_CLK_SEL0					0x1c20
#define FT_ETH_TX_CLK_SEL1					0x1c24
#define FT_ETH_TX_CLK_SEL2					0x1c28
#define FT_ETH_TX_CLK_SEL3					0x1c2c
#define FT_ETH_RX_CLK_SEL0					0x1c30
#define FT_ETH_RX_CLK_SEL1					0x1c34
#define FT_ETH_CLK_250M_DIV10_DIV100_SEL	0x1c38
#define FT_ETH_RX_CLK_SEL5					0x1c48
#define FT_ETH_TX_CLK_SEL3_0				0x1c70
#define FT_ETH_TX_CLK_SEL4_0				0x1c74
#define FT_ETH_RX_CLK_SEL3_0				0x1c78
#define FT_ETH_RX_CLK_SEL4_0				0x1c7c
#define FT_ETH_RGMII_TX_CLK_SEL0			0x1c80
#define FT_ETH_RGMII_TX_CLK_SEL1			0x1c84

#define FT_ETH_SPEED_100					0
#define FT_ETH_SPEED_1000					1
#define FT_ETH_SPEED_2500					2
#define FT_ETH_SPEED_5000					3
#define FT_ETH_SPEED_10000					4
#define FT_ETH_SPEED_25000					5

/*FT_ETH pcs control register bitfields*/
#define FT_ETH_PCSCTRL     					0x0200 /* PCS control register*/
#define FT_ETH_PCSSTATUS   					0x0204 /* PCS Status*/
#define FT_ETH_PCSANLPBASE 					0x0214 /* PCS an lp base */
#define FT_ETH_AUTONEG_OFFSET    			12
#define FT_ETH_AUTONEG_SIZE      			1

#define FT_ETH_HSMAC						0x0050 /* Hs mac config register*/
/*FT_ETH hs mac config register bitfields*/
#define FT_ETH_HSMACSPEED_OFFSET    		0
#define FT_ETH_HSMACSPEED_SIZE      		3
#define FT_ETH_USX_CONTROL         			0x0A80  /* High speed PCS control register */

/* Bitfields in USX_CONTROL. */
#define FT_ETH_SIGNAL_OK_OFFSET            0
#define FT_ETH_SIGNAL_OK_SIZE              1
#define FT_ETH_TX_EN_OFFSET                1
#define FT_ETH_TX_EN_SIZE                  1
#define FT_ETH_RX_SYNC_RESET_OFFSET		   2
#define FT_ETH_RX_SYNC_RESET_SIZE          1
#define FT_ETH_FEC_ENABLE_OFFSET           4
#define FT_ETH_FEC_ENABLE_SIZE             1
#define FT_ETH_FEC_ENA_ERR_IND_OFFSET      5
#define FT_ETH_FEC_ENA_ERR_IND_SIZE        1
#define FT_ETH_TX_SCR_BYPASS_OFFSET        8
#define FT_ETH_TX_SCR_BYPASS_SIZE          1
#define FT_ETH_RX_SCR_BYPASS_OFFSET        9
#define FT_ETH_RX_SCR_BYPASS_SIZE          1
#define FT_ETH_SERDES_RATE_OFFSET          12
#define FT_ETH_SERDES_RATE_SIZE            2
#define FT_ETH_USX_CTRL_SPEED_OFFSET       14
#define FT_ETH_USX_CTRL_SPEED_SIZE         3

#define FT_2PT5G_OFFSET  				29
#define FT_2PT5G_SIZE    				1
#define FT_HSMAC_OFFSET   				31 /* Use high speed MAC */
#define FT_HSMAC_SIZE     				1

/* limit RX checksum offload to TCP and UDP packets */
#define FT_ETH_RX_CSUM_CHECKED_MASK		2
#define ft_eth_writel_queue_TBQP(port, value, queue_num)	\
	writel((value), (port)->regs + FT_ETH_TBQP(queue_num))
#define ft_eth_writel_queue_TBQPH(port, value, queue_num)	\
	writel((value), (port)->regs + FT_ETH_TBQPH(queue_num))
#define ft_eth_writel_queue_RBQP(port, value, queue_num)	\
	writel((value), (port)->regs + FT_ETH_RBQP(queue_num))
#define ft_eth_writel_queue_RBQPH(port, value, queue_num)	\
	writel((value), (port)->regs + FT_ETH_RBQPH(queue_num))

#endif /* __DRIVERS_FT_H__ */
