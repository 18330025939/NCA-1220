#ifndef _PARAMETER_H
#define _PARAMETER_H

#define OEM_COMMON_INFO				0xC300FF10
#define COMMON_CPU_INFO				0x0

#define OEM_PEU_INFO				0xC300FF11
#define PEU_BASE_INFO				0x0

#define OEM_OS_INFO					0xC300FF12
#define OS_INITRD_SIZE				0x0
#define OS_INITRD_START				0x1

#define	MEM_REGIONS					0xC2000005			//phytium_sip 有,就不用在OEM中重新注册一个了

#define USE_PARAMETER				//采用读取参数表
#define IACC_BASE				    (0x38000000)
#define O_PARAMETER_BASE			(IACC_BASE + 0xF5000)	//1004k

#define PM_PLL_OFFSET				(0x0)
#define PM_PCIE_OFFSET				(0x100)
#define PM_COMMON_OFFSET			(0x200)
#define PM_MCU_OFFSET				(0x300)
#define PM_BOARD_OFFSET				(0x400)
#define PM_SECURE_OFFSET			(0x500)	//存放tzc aes的misc结构体地址偏移

#define PARAMETER_CPU_MAGIC         0x54460020
#define PARAMETER_PLL_MAGIC			0x54460020	
#define PARAMETER_PCIE_MAGIC        0x54460021
#define PARAMETER_COMMON_MAGIC		0x54460013
#define PARAMETER_MCU_MAGIC         0x54460024
#define PARAMETER_BOARD_MAGIC       0x54460015
#define PARAMETER_PAD_MAGIC			0x54460016

#define PM_PLL						0
#define PM_PCIE						1
#define PM_COMMON					2
#define PM_MCU						3
#define PM_BOARD					4

#define PLAT_PAD_BASE				(IACC_BASE + 0xFA000)//1023k

/*-------------------HEAD------------------*/
typedef struct head_config {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint8_t rev1[4];
}__attribute__((aligned(sizeof(unsigned long)))) head_config_t;

/*-------------------PLL------------------*/
typedef struct pll_config {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint8_t rev1[4];
	uint32_t clust0_pll;	
	uint32_t clust1_pll;	
	uint32_t clust2_pll;	
	uint32_t noc_pll;	
	uint32_t dmu_pll;	
}__attribute__((aligned(sizeof(unsigned long)))) pll_config_t;

/*-------------------PEU------------------*/
typedef struct pcu_ctr{
	uint32_t base_config[4];
	uint32_t equalization[4];
	uint8_t rev[72];
}__attribute__((aligned(sizeof(unsigned long)))) peu_ctr_t;

typedef struct pcu_config {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint8_t rev1[4];
	uint32_t independent_tree;
	uint32_t base_cfg;
	uint8_t rev2[16];
	peu_ctr_t ctr_cfg[2];
}__attribute__((aligned(sizeof(unsigned long)))) peu_config_t;

/*-------------------COMMON------------------*/
typedef struct common_config {
	uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint8_t rev1[4];
	uint64_t core_bit_map;
}__attribute__((aligned(sizeof(unsigned long)))) common_config_t;

/*-------------------TZC AES------------------*/
typedef struct secure_config {
	uint64_t misc_enable;	//0: tzc en; 1: aes en

	uint64_t tzc_region_low_1;  //tzc enable时,开放4个安全域供用户配置
    uint64_t tzc_region_top_1;
    uint64_t tzc_region_low_2;
    uint64_t tzc_region_top_2;
    uint64_t tzc_region_low_3;
    uint64_t tzc_region_top_3;
    uint64_t tzc_region_low_4;
    uint64_t tzc_region_top_4;

}__attribute__((aligned(sizeof(unsigned long)))) secure_config_t;

/*-------------------MCU------------------*/
typedef struct ddr_spd {
    /******************* read from spd *****************/
    uint8_t  dimm_type;			/* 1: RDIMM;    2: UDIMM;  3: SODIMM;    4: LRDIMM */
    uint8_t  data_width;		/* 0: x4;       1: x8;     2: x16        3: X32*/
    uint8_t  mirror_type;		/* 0: stardard; 1: mirror  DDR5 无*/
    uint8_t  ecc_type;			/* 0: no-ecc;   1:ecc */
    uint8_t  dram_type;			/* 0xB: DDR3;   0xC: DDR4  0x10: LPDDR4, 0x12: DDR5*/
    uint8_t  rank_num;
    uint8_t  row_num;
    uint8_t  col_num;

    uint8_t  bg_num;			/*DDR4 / DDR5*/
    uint8_t  bank_num;
    uint16_t module_manufacturer_id;
    uint16_t tAAmin;
    uint16_t tRCDmin;

    uint16_t tRPmin;
    uint16_t tRASmin;
    uint16_t tRCmin;
    uint16_t tFAWmin;		/*only DDR3/DDR4*/

    uint16_t tRRD_Smin;		/*only DDR4*/
    uint16_t tRRD_Lmin;		/*only DDR4*/
    uint16_t tCCD_Lmin;		/*only DDR4*/
    uint16_t tWRmin;

    uint16_t tWTR_Smin;		/*only DDR4*/
    uint16_t tWTR_Lmin;		/*only DDR4*/
    uint32_t tRFC1min;

    uint32_t tRFC2min;
	uint32_t tRFC4_RFCsbmin;		/*DDR4: tRFC4min;  DDR5: tRFCsbmin*/

	uint8_t  resv[8];

    /******************* RCD control words *****************/
    uint8_t  F0RC03;	/*bit[3:2]:CS         bit[1:0]:CA  */
    uint8_t  F0RC04; 	/*bit[3:2]:ODT        bit[1:0]:CKE */
    uint8_t  F0RC05; 	/*bit[3:2]:CLK-A side bit[1:0]:CLK-B side */
    uint8_t  rcd_num;			/* Registers used on RDIMM */

	uint8_t  lrdimm_resv[4];
	uint8_t  lrdimm_resv1[8];
	uint8_t  lrdimm_resv2[8];

}__attribute__((aligned(sizeof(unsigned long)))) ddr_spd_info_t;

typedef struct mcu_config { 
    uint32_t magic;
	uint32_t version;
	uint32_t size;
	uint8_t rev1[4];

	uint8_t  ch_enable;
	uint8_t  resv1[7];

	uint64_t misc_enable;

	uint8_t  train_debug;
	uint8_t  train_recover;		/*为1时跳过training*/
	uint8_t  train_param_type;
	uint8_t  train_param_1;		/* DDR4: cpu_odt     */
	uint8_t  train_param_2;		/* DDR4: cpu_drv     */
	uint8_t  train_param_3;		/* DDR4: mr_drv      */
	uint8_t  train_param_4;		/* DDR4: rtt_nom     */
	uint8_t  train_param_5;		/* DDR4: rtt_park    */

	uint8_t  train_param_6;		/* DDR4: rtt_wr      */
	uint8_t  resv2[7];

/*******for LPDDR4 dq swap************/	
	uint32_t data_byte_swap;
	uint32_t slice0_dq_swizzle;

	uint32_t slice1_dq_swizzle;
	uint32_t slice2_dq_swizzle;

	uint32_t slice3_dq_swizzle;
	uint32_t slice4_dq_swizzle;

	uint32_t slice5_dq_swizzle;
	uint32_t slice6_dq_swizzle;

	uint32_t slice7_dq_swizzle;
	uint8_t  resv3[4];

	uint8_t  resv4[8];

    ddr_spd_info_t ddr_spd_info;
}__attribute__((aligned(sizeof(unsigned long)))) mcu_config_t;


/*-------------------Board------------------*/
typedef struct board_config {
	head_config_t head;

	uint8_t misc_config;
	uint8_t power_manger;
	uint8_t qspi_freq_rank; 
	uint8_t resev;
	uint32_t phy_sel_mode; 
	uint32_t gmac_speed_mode;//后面可能删去
	uint32_t mio_sel;
	uint32_t param_auto_sel;
	uint32_t sd0_hw_cfg;
	uint32_t sd1_hw_cfg;
}__attribute__((aligned(sizeof(unsigned long)))) board_config_t;

#define BOARD_VERSION		((0<<16) | 1)
#define BOARD_SIZE			(0x100)

/*-------------------FDT------------------*/
typedef struct core_info {
	uint64_t cpu_map;
	uint32_t tick_frq;
}__attribute__((aligned(sizeof(unsigned long)))) core_info_t;

typedef struct peu_info {
	uint32_t peu_init_stat;
}__attribute__((aligned(sizeof(unsigned long)))) peu_info_t;

typedef struct mem_block_pbf {
    uint64_t mb_start;
    uint64_t mb_size;
    uint64_t mb_node_id;
} mem_block_t;

typedef struct mem_region_pbf {
    uint64_t mb_count;
    mem_block_t mb_blocks[2];
} mem_region_t;

uint32_t get_dram_info(mem_region_t *ptr);

/*****************************************************************************/


void board_fix_env(void);
void get_pm_pll_info(pll_config_t *pm_data);
void get_pm_peu_info(peu_config_t *pm_data);
void get_pm_common_info(common_config_t *pm_data);
void get_pm_mcu_info(mcu_config_t *pm_data);
void *get_parameter_base(uint32_t id);
int get_parameter_info(uint32_t id, uint8_t *pm_data, const uint32_t data_size);

uint32_t get_cpu_info(core_info_t *ptr);
uint32_t get_peu_info(peu_info_t *ptr);
uint32_t pm_enable_spd_freq(void);
uint32_t pm_get_power_manager(void);
uint32_t pm_get_qspi_freq_rank(void);
uint32_t pm_get_psu_phy0_sel(void);
uint32_t pm_get_psu_phy1_sel(void);
uint32_t pm_get_gsd_phy0_sel(void);
uint32_t pm_get_gsd_phy1_sel(void);
uint32_t pm_get_gsd_phy2_sel(void);
uint32_t pm_get_gsd_phy3_sel(void);
uint32_t pm_get_mac0_mode_sel(void);
uint32_t pm_get_mac1_mode_sel(void);
uint32_t pm_get_mac2_mode_sel(void);
uint32_t pm_get_mac3_mode_sel(void);
uint32_t pm_get_speed0_mode_sel(void);
uint32_t pm_get_speed1_mode_sel(void);
uint32_t pm_get_speed2_mode_sel(void);
uint32_t pm_get_speed3_mode_sel(void);
uint32_t pm_get_phy_sel_all(void);
uint32_t pm_get_mac_mode_all(void);
uint32_t pm_get_speed_mode_all(void);
uint32_t pm_get_param_auto_sel(void);
uint32_t pm_get_param_mio_sel(void);
uint32_t pm_get_sd0_hw_cfg(void);
uint32_t pm_get_sd1_hw_cfg(void);
#ifdef PHYTIUM_PINCTRL_TOOLS
struct gpio_pad *pm_get_gpio_pad_info(void);
#endif
#endif
