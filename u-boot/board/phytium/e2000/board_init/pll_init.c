// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/gic.h>
#include <asm/armv8/mmu.h>
#include "cpu.h"
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include "../parameter/parameter.h"
#include "board.h"
#include <e_uart.h>

DECLARE_GLOBAL_DATA_PTR;

pll_config_t const pll_base_info = {
	.magic = PARAMETER_PLL_MAGIC,
	.version = 0x1,
	.size = 0x100,
	.clust0_pll = 2000,
	.clust1_pll = 2000,
	.clust2_pll = 2000,
	.noc_pll = 1800,
	.dmu_pll = 600,
};
#if 0
void fix_pll_info(uint8_t *data)
{
	pll_config_t *pm_data = (pll_config_t *)data;
	if(pm_enable_spd_freq()){
        //读取spd频率
        p_printf("read spd freq...\n");
        uint32_t spd_freq = i2c_read_spd_lmu_freq(0x3);
        if(spd_freq)
            pm_data->lmu_pll = pm_data->lmu_pll > spd_freq ? spd_freq : pm_data->lmu_pll;
	}
}
#endif
void pll_init(void)
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;

	if((get_parameter_info(PM_PLL, buff, sizeof(buff)))){ //从参数表中获取参数
		p_printf("use default parameter\n");
		memcpy(buff, &pll_base_info, sizeof(pll_base_info));
	}

	//fix_pll_info(buff);

	arm_smccc_smc(CPU_INIT_PLL , 0, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("error ret  %lx\n", res.a0);
		while(1);
	}
}
