#include <common.h>
#include <asm/io.h>
#include "parameter.h"
#include <e_uart.h>
#include "../cpu.h"

#define  FREQ_FROM_SPD_EN_BIT           0

static const board_config_t board_info = {
     .head.magic = PARAMETER_BOARD_MAGIC,
     .head.version = BOARD_VERSION,				/*代码版本*/
     .head.size = BOARD_SIZE,
     .misc_config = (1 << FREQ_FROM_SPD_EN_BIT),
	 .power_manger = 0,
	 .qspi_freq_rank = 0,
};

const board_config_t * get_board_pm_ptr(uint32_t ver)
{
	board_config_t const *ret;
	ret = (board_config_t *)get_parameter_base(PM_BOARD);

	if(!ret)
		ret = &board_info;
	/*参数版本低于代码版本则 返回默认值*/
	if(ver > ret->head.version)
		ret = NULL;

	return (ret);
}

/*用于区分代码版本*/
#define DECLARE_PTR(MAJOR, SEC)     board_config_t const * const info_ptr = get_board_pm_ptr(MAJOR << 16 | SEC)

/* 0 : disable    1: enable*/
uint32_t pm_enable_spd_freq(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

	return ((info_ptr->misc_config >> FREQ_FROM_SPD_EN_BIT) & 0x1);
}

/*
 *  0 - CPLD
 *  1 - EC
 *  2 - SE
 */
uint32_t pm_get_power_manager(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

	return (info_ptr->power_manger);
}

/*
 *   0 - 37.5MHZ
 *   1 - 18.5MHZ
 *   2 - 9.37MHZ
 *   3 - 4.6MHZ
 */
uint32_t pm_get_qspi_freq_rank(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

	return (info_ptr->qspi_freq_rank);
}

uint32_t pm_get_psu_phy0_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>0)&0x3;
}

uint32_t pm_get_psu_phy1_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>2)&0x3;
}

uint32_t pm_get_gsd_phy0_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>4)&0x3;
}

uint32_t pm_get_gsd_phy1_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>6)&0x3;
}

uint32_t pm_get_gsd_phy2_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>8)&0x3;
}

uint32_t pm_get_gsd_phy3_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->phy_sel_mode)>>10)&0x3;
}

uint32_t pm_get_mac0_mode_sel(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

    return ((info_ptr->gmac_speed_mode)>>0)&0xf;
}

uint32_t pm_get_mac1_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>4)&0xf;
}

uint32_t pm_get_mac2_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>8)&0xf;
}

uint32_t pm_get_mac3_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>12)&0xf;
}

uint32_t pm_get_speed0_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>16)&0xf;
}

uint32_t pm_get_speed1_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>20)&0xf;
}

uint32_t pm_get_speed2_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>24)&0xf;
}

uint32_t pm_get_speed3_mode_sel(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

    return ((info_ptr->gmac_speed_mode)>>28)&0xf;
}

uint32_t pm_get_phy_sel_all(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

	return (info_ptr->phy_sel_mode);
}

uint32_t pm_get_mac_mode_all(void)
{
	 DECLARE_PTR(0, 1);

	 if(!info_ptr)
		 return 0;

	return (info_ptr->gmac_speed_mode);
}

uint32_t pm_get_speed_mode_all(void)
{
	  DECLARE_PTR(0, 1);

	  if(!info_ptr)
		  return 0;

	return ((info_ptr->gmac_speed_mode)>>16);
}

uint32_t pm_get_param_auto_sel(void)
{
	  DECLARE_PTR(0, 1);

	  if(!info_ptr)
		  return 0;

	return (info_ptr->param_auto_sel);
}

uint32_t pm_get_param_mio_sel(void)
{
	  DECLARE_PTR(0, 1);

	  if(!info_ptr)
		  return 0;

	return (info_ptr->mio_sel);
}

uint32_t pm_get_sd0_hw_cfg(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

	return (info_ptr->sd0_hw_cfg);
}

uint32_t pm_get_sd1_hw_cfg(void)
{
	DECLARE_PTR(0, 1);

	if(!info_ptr)
		return 0;

	return (info_ptr->sd1_hw_cfg);
}
