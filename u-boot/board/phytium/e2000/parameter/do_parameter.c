// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * lixinde <lixinde@phytium.com.cn>
 */

#include <common.h>
#include <env.h>
#include <asm/io.h>
#include <cpu_func.h>
#include <linux/arm-smccc.h>
#include "parameter.h"

static uint32_t get_info_for_atf(uint32_t oem_number, uint32_t fun_number, void *base, uint32_t size)
{
	struct arm_smccc_res res;

	memset(base, 0, size);
	flush_dcache_range((unsigned long)base, (unsigned long)base + size);
	arm_smccc_smc((uint64_t)oem_number, (uint64_t)fun_number,(uint64_t)base, (uint64_t)size, 0, 0, 0, 0, &res);
	dsb();
	isb();
	if(res.a0){
		printf("smc : error  oem : 0x%x, fun: 0x%x\n ", oem_number, fun_number);
		return -1;
	}else
		return 0;
}

/*获取cpu info*/
uint32_t get_cpu_info(core_info_t *ptr)
{
	return get_info_for_atf(OEM_COMMON_INFO, COMMON_CPU_INFO, ptr, sizeof(core_info_t));
}

uint32_t get_peu_info(peu_info_t *ptr)
{
	return get_info_for_atf(OEM_PEU_INFO, PEU_BASE_INFO, ptr, sizeof(peu_info_t) * 2);
}

uint32_t get_dram_info(mem_region_t *ptr)
{
	struct arm_smccc_res res;
	memset(ptr, 0, sizeof(mem_region_t));
	flush_dcache_range((unsigned long)ptr, (unsigned long)ptr + sizeof(mem_region_t));
	arm_smccc_smc(MEM_REGIONS, (uint64_t)ptr, sizeof(mem_region_t), 0, 0, 0, 0, 0, &res);
	dsb();
	isb();
	if(res.a0){
		printf("smc : error , id: 0x%x\n", MEM_REGIONS);
		return -1;
	}else{
		return 0;
	}

}

/*获取initrd, 修改环境变量*/
void board_fix_env(void)
{
	char tep[32];
	char tep1[16];

	if(get_info_for_atf(OEM_OS_INFO, OS_INITRD_SIZE, tep, sizeof(tep)))
		printf("error : board_fix_initrd_size \n ");
	else{
		printf("get initrd_size : %s \n",tep);
		printf("get initrd_size_bd : %s \n",&tep[16]);
		env_set("initrd_size", tep);
		env_set("initrd_size_bd",&tep[16]);
	}

	if(get_info_for_atf(OEM_OS_INFO, OS_INITRD_START, tep1, sizeof(tep1)))
		printf("error : board_fix_initrd_addr \n ");
	else{
		printf("get initrd_addr : %s \n",tep1);
		env_set("initrd_addr", tep1);
	}
}
