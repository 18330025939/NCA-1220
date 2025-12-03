/*
 * =====================================================================================
 *
 *       Filename:  ft_fdt_update.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2018年05月21日 16时35分13秒
 *       Revision:  none
 *
 *         Author:  lixinde, lixindei@phytium.com.cn
 *        Company:  Phytium
 *        License:  Dual BSD/GPL
 *
 * =====================================================================================
 */

#include <common.h>
#include <malloc.h>
#include <init.h>
#include "../cpu.h"
#include <linux/arm-smccc.h>
#include "../parameter/parameter.h"
#include "ft_fdt_lib.h"

DECLARE_GLOBAL_DATA_PTR;

static uint32_t ft_fdt_fixup_dram(void)
{
	int nodeoffset, err;

	//获取内存大小
	mem_region_t dram_info;
	if(get_dram_info(&dram_info)){
        printf("fdt :can not get dram info, skip memory node update\n");
        return 0;
    }

    printf("mb_count = 0x%llx\n", dram_info.mb_count);
    printf("mb_blocks[0].mb_size = 0x%llx\n", dram_info.mb_blocks[0].mb_size);
    printf("mb_blocks[1].mb_size = 0x%llx\n", dram_info.mb_blocks[1].mb_size);

	//dram_info.mb_count = 1;
	//dram_info.mb_blocks[1].mb_size = 0;
	//dram_info.mb_blocks[1].mb_start = 0x2000001000;

	if(dram_info.mb_count == 1){
		//只有一段,删除01节点,修正00节点
		printf("fdt : remove memory@1");
		fdt_remove_node("/memory@01");
		nodeoffset = fdt_get_offset("/memory@00");
		err = ft_fdt_set_mem_prop(nodeoffset, "reg",dram_info.mb_blocks[0].mb_start, dram_info.mb_blocks[0].mb_size);
		if(err){
			printf("fdt : failed ft_fdt_fixup_dram\n");
			return err;
		}
	}else{
		//有两段,都修正
		nodeoffset = fdt_get_offset("/memory@00");
		err = ft_fdt_set_mem_prop(nodeoffset, "reg",dram_info.mb_blocks[0].mb_start, dram_info.mb_blocks[0].mb_size);
		if(err){
			printf("fdt : failed ft_fdt_fixup_dram\n");
			return err;
		}

		nodeoffset = fdt_get_offset("/memory@01");
		err = ft_fdt_set_mem_prop(nodeoffset, "reg",dram_info.mb_blocks[1].mb_start, dram_info.mb_blocks[1].mb_size);
		if(err){
			printf("fdt : failed ft_fdt_fixup_dram\n");
			return err;
		}
	}
    printf("fdt : dram size 0x%llx update successfully\n", (0x80000000 + dram_info.mb_blocks[1].mb_size));

	return 0;
}

#if 0
static uint32_t ft_fdt_fixup_timer(u32 frq)
{
	int nodeoffset, err, len;
	u8 tmp[8] = {0};

	nodeoffset = fdt_get_offset("/timer");
	if(nodeoffset < 0)
		return	0;
	len = ft_fdt_pack_word(tmp, frq);
	err = fdt_add_prop(nodeoffset, "clock-frequency", tmp, len);

	return err;
}

static uint32_t ft_fdt_fixup_cpu(void)
{
	core_info_t cpu_info;
	uint32_t i, err, cpu_id;
	char *node_name = malloc(40);
	uint32_t clu_num = 2 * 2 ;	/*e2000有2cluster, 4个核*/

	if(get_cpu_info(&cpu_info)){
		printf("fdt :can not get cpu info, skip cpu node update\n");
		return 0;
	}

	printf("cpu_map = 0x%llx\n", cpu_info.cpu_map);
	//该节点不存在删除对应cpu节点
	for( i = 0; i < clu_num; i++){
		if(!(cpu_info.cpu_map >> i)&1){
			switch (i) {
			case 0:
				cpu_id = 0;
				break;
			case 1:
				cpu_id = 100;
				break;
			case 2:
				cpu_id = 200;
				break;
			case 3:
				cpu_id = 201;
				break;
			}

			printf("fdt : remove cpu@%d\n", cpu_id);
			sprintf(node_name,"/cpus/cpu@%d", cpu_id);
			fdt_remove_node(node_name);
		}
	}
	free(node_name);
	printf("fdt : cpu node update has been successfully!\n");

	err = ft_fdt_fixup_timer(cpu_info.tick_frq);
	if(err){
		printf("fdt : failed ft_fdt_fixup_timer\n");
		return err;
	}

	printf("fdt : time frq : %d HZ update successfully\n",cpu_info.tick_frq);

	return 0;
}

static uint32_t ft_fdt_fixup_peu(void)
{
	peu_info_t peu_info[2];

	if(get_peu_info(peu_info)){
		printf("fdt :can not get peu info, skip peu node update\n");
		return 0;
	}

	if(!(peu_info[0].peu_init_stat)){
		fdt_remove_node("/interrupt-controller/gic-its");
		fdt_remove_node("/smb/motherboard/peu");
	}

	printf("fdt : pcie node has been successfully  update\n");
	return 0;
}

static uint32_t ft_fdt_fixup_other(void)
{
	ft_fixup_mem(gd->bd->bi_dram[0].size, gd->bd->bi_dram[1].size);

	printf("fdt : memory node has been successfully  update\n");

	return	0;
}
#endif
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int err;
	printf("run in ft_board_setup\n");

	err = set_fdt_addr(blob);
	if(err)
		return err;
#if 0
	err = ft_fdt_fixup_cpu();
	if(err)
		return err;

	err = ft_fdt_fixup_peu();
	if(err)
		return err;

#endif
	err = ft_fdt_fixup_dram();
	if(err)
		return err;
#if 0
	err = ft_fdt_fixup_other();
	if(err)
		return err;
#endif
	return 0;
}

