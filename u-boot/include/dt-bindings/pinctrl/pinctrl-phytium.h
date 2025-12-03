#ifndef __FT_PINC_H
#define __FT_PINC_H

#include <common.h>
/*******************当前还是FT2004的pin信息，还未改成e2000*******************************/
typedef struct _pin_mux_field_desc_t {
	ushort hi;
	ushort lo;
	ushort mask;
	char * func[3];
} pin_mux_filed_desc_t;

static const pin_mux_filed_desc_t reg_0_desc_tbl[] = {
	{ 29, 28, 0x3, {"all_pll_lock", "null", "i2c_1_scl"}	},			//all_pll_lock_pad
	{ 25, 24, 0x3, {"cru_clk_ob)" , "gpio0_porta_0", "i2c_1_sda"}	},	//cru_clk_obv_pad
	{ 21, 20, 0x3, {"sjtag_tdi_pad", "gpio0_porta_1", "uart_0_cts_n"}    }, 
	{ 17, 16, 0x3, {"sjtag_tms_pad", "gpio0_porta_2", "uart_0_dcd_n"}    }, 
	{ 13, 12, 0x3, {"sjtag_ntrst_pad", "gpio0_porta_3", "uart_0_dsr_n"}    }, 
	{  9,  8, 0x3, {"sjtag_tdo_pad", "gpio0_porta_4", "uart_0_ri_n"}    }, 
	{  5,  4, 0x3, {"tjtag_tdo_pad", "gpio0_porta_5", "uart_0_rts_n"}    },
	{  1,  0, 0x3, {"tjtag_ntrst_pad", "gpio0_porta_6", "uart_0_dtr_n"}    }
};

static const pin_mux_filed_desc_t reg_1_desc_tbl[] = {
 	{ 29, 28, 0x3, {"tjtag_tms_pad", 	"gpio0_porta_7", "peu1_linkup_0"}	},	//tjtag_tms_pad
 	{ 25, 24, 0x3, {"tjtag_tdi_pad", 	"can_txd_0", "peu1_linkup_1"}	},	//tjtag_tdi_pad
 	{ 21, 20, 0x3, {"ntrst_swj_pad",	"can_txd_1", "peu1_linkup_2"}   },	//ntrst_swj_pad 
 	{ 17, 16, 0x3, {"tdi_swj_pad",   	"can_txd_2", "null"}           }, 
	{ 13, 12, 0x3, {"swditms_swj_pad",  "can_rxd_0", "null"}       }, 
	{  9,  8, 0x3, {"swdo_swj_pad",     "can_rxd_1", "i2c_2_scl"}     }, 
	{  5,  4, 0x3, {"tdo_swj_pad",      "can_rxd_2", "i2c_2_sda"}      },
	{  1,  0, 0x3, {"hdt_mb_done_state_pad", 	"lpc_ext_irq_outen", "i2c_3_scl"}	}
};

static const pin_mux_filed_desc_t reg_2_desc_tbl[] = {
 	{ 29, 28, 0x3, {"hdt_mb_fail_state_pad", 	"lpc_ext_lad_outen", "i2c_3_sda"}	},	//0x0208
 	{ 25, 24, 0x3, {"i2c_0_scl_pad",	"null", "null"}             },	
 	{ 21, 20, 0x3, {"i2c_0_sda_pad",   	"null", "null"}           }, 
 	{ 17, 16, 0x3, {"spi0_csn0_pad",    "gpio1_porta_5", "null"}       }, 
	{ 13, 12, 0x3, {"spi0_sck_pad",     "gpio1_porta_6", "null"}     }, 
	{  9,  8, 0x3, {"spi0_so_pad",   	"gpio1_porta_7", "null"}             },
	{  5,  4, 0x3, {"spi0_si_pad",      "gpio1_portb_0", "null"}      },
	{  1,  0, 0x3, {"sd_cmd_pad",      "gpio1_portb_1", "null"}      }
};

static const pin_mux_filed_desc_t reg_3_desc_tbl[] = {
 	{ 29, 28, 0x3, {"sd_clk_pad ", 	"gpio1_portb_2", "null"}	},			//0x020c
 	{ 25, 24, 0x3, {"sd_dat0_pad", 	"gpio1_portb_3", "null"}	},	
 	{ 21, 20, 0x3, {"sd_dat1_pad",	"gpio1_portb_4", "null"}    },	
 	{ 17, 16, 0x3, {"sd_dat2_pad",  "gpio1_portb_5", "null"}    }, 
	{ 13, 12, 0x3, {"sd_dat3_pad",  "gpio1_portb_6", "null"}    }, 
	{  9,  8, 0x3, {"sd_detect_pad",     "status_jtagnsw", "null"}     }, 
	{  5,  4, 0x3, {"hda_bclk_pad ",   	"null", "null"}             },
	{  1,  0, 0x3, {"hda_rst_pad",      "null", "null"}      }
};	

static const pin_mux_filed_desc_t reg_4_desc_tbl[] = {
 	{ 29, 28, 0x3, {"hda_sync_pad ", 	"null", "null"}	},		//0x0210
 	{ 25, 24, 0x3, {"hda_sdo_pad", 	"null", "null"}	},	
 	{ 21, 20, 0x3, {"hda_sdi0_pad",	"null", "null"}             },	
 	{ 17, 16, 0x3, {"uart_0_rxd_pad",  "null", "null"}           }, 
	{ 13, 12, 0x3, {"uart_0_txd_pad",  "null", "null"}       }, 
	{  9,  8, 0x3, {"uart_1_rxd_pad ",     "null", "null"}     }, 
	{  5,  4, 0x3, {"uart_1_txd_pad  ",   	"null", "null"}             },
	{  1,  0, 0x3, {"uart_2_rxd_pad ",      "spi1_csn0", "gpio0_portb_5"}      }
};	

static const pin_mux_filed_desc_t reg_5_desc_tbl[] = {		//0x0214
 	{ 29, 28, 0x3, {"uart_2_txd_pad ", 	"spi1_sck", "hda_sdi1"}	},		
 	{ 25, 24, 0x3, {"uart_3_rxd_pad ", 	"spi1_so", "hda_sdi2"}	},	
 	{ 21, 20, 0x3, {"uart_3_txd_pad",	"spi1_si", "hda_sdi3"}             },	
 	{ 17, 16, 0x3, {"qspi_csn0_pad",  "null", "null"}           }, 
	{ 13, 12, 0x3, {"qspi_csn1_pad",  "gpio1_portb_7", "null"}       }, 
	{  9,  8, 0x3, {"qspi_csn2_pad ", "spi1_csn1", "gpio0_portb_6"}     }, 
	{  5,  4, 0x3, {"qspi_csn3_pad ",  "spi1_csn2", "gpio0_portb_7"}       },
	{  1,  0, 0x3, {"qspi_sck_pad",      "null", "null"}      }
};	

static const pin_mux_filed_desc_t reg_6_desc_tbl[] = {
 	{ 29, 28, 0x3, {"qspi_so_io0_pad ", 	"null", "null"}	},		//0x0218
 	{ 25, 24, 0x3, {"qspi_si_io1_pad", 	"null", "null"}	},	
 	{ 21, 20, 0x3, {"qspi_wp_io2_pad",	"null", "null"}             },	
 	{ 17, 16, 0x3, {"qspi_hold_io3_pad",  "null", "null"}           }, 
	{ 13, 12, 0x3, {"ext_lpc_lad_0_pad",  "gpio1_porta_3", "null"}      },
	{  9,  8, 0x3, {"ext_lpc_lad_1_pad ", "gpio1_porta_4", "null"}	},		
	{  5,  4, 0x3, {"ext_lpc_lad_2_pad",  "spi1_csn3", "null"}	},	
	{  1,  0, 0x3, {"ext_lpc_lad_3_pad",  "spi0_csn3", "null"}    }
};	

/*
static const pin_mux_filed_desc_t reg_4_desc_tbl[] = {
 	{ 29, 28, 0x3, {"null ", 	"null", "null"}	},		
 	{ 25, 24, 0x3, {"null", 	"null", "null"}	},	
 	{ 21, 20, 0x3, {"null",	"null", "null"}             },	
 	{ 17, 16, 0x3, {"null",  "null", "null"}           }, 
	{ 13, 12, 0x3, {"null",  "null", "null"}       }, 
	{  9,  8, 0x3, {"null",     "null", "null"}     }, 
	{  5,  4, 0x3, {"null ",   	"null", "null"}             },
	{  1,  0, 0x3, {"null",      "null", "null"}      }
};	
*/


typedef struct _MUX_field_desc_and_len_t {
	const pin_mux_filed_desc_t *pdesc;
	ushort len;
} MUX_field_desc_and_len_t;

static const MUX_field_desc_and_len_t desc_and_len_tbl[] = {
	{ reg_0_desc_tbl, ARRAY_SIZE(reg_0_desc_tbl)   },
	{ reg_1_desc_tbl, ARRAY_SIZE(reg_1_desc_tbl)   },
	{ reg_2_desc_tbl, ARRAY_SIZE(reg_2_desc_tbl)   },
	{ reg_3_desc_tbl, ARRAY_SIZE(reg_3_desc_tbl)   },
	{ reg_4_desc_tbl, ARRAY_SIZE(reg_4_desc_tbl)   },
	{ reg_5_desc_tbl, ARRAY_SIZE(reg_5_desc_tbl)   },
	{ reg_6_desc_tbl, ARRAY_SIZE(reg_6_desc_tbl)   },
};

void dump_mux_reg(void * addr, const MUX_field_desc_and_len_t *pdl);

#endif /* __FT_PINC_H */
