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
#include <netdev.h>
#include <phy.h>
#include <linux/arm-smccc.h>
#include "../parameter/parameter.h"
#include "board.h"
#include <e_uart.h>
#include "../cpu.h"

DECLARE_GLOBAL_DATA_PTR;

peu_config_t const peu_base_info  = {
	.magic = PARAMETER_PCIE_MAGIC,
	.version = 0x2,
	.size = 0x100,
	.independent_tree = CONFIG_INDEPENDENT_TREE,
	.base_cfg = ((CONFIG_PCI_PEU1 | (X1X1X1X1<< 1)) << PEU1_OFFSET | \
   				(CONFIG_PCI_PEU0 | (X1X1 << 1))),
   	.ctr_cfg[0].base_config[0] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[0].base_config[1] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
   	.ctr_cfg[1].base_config[0] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[1] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[2] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[1].base_config[3] = (RC_MODE << PEU_C_OFFSET_MODE) | (GEN3 << PEU_C_OFFSET_SPEED),
	.ctr_cfg[0].equalization[0] = 0x7,
	.ctr_cfg[0].equalization[1] = 0x7,
	.ctr_cfg[1].equalization[0] = 0x7,
	.ctr_cfg[1].equalization[1] = 0x7,
	.ctr_cfg[1].equalization[2] = 0x7,
	.ctr_cfg[1].equalization[3] = 0x7,
};

void pcie_init(void)
{
	uint8_t buff[0x100];
	struct arm_smccc_res res;

	if((get_parameter_info(PM_PCIE, buff, sizeof(buff)))){ //从参数表中获取参数
		p_printf("use default parameter\n");
		memcpy(buff, &peu_base_info, sizeof(peu_base_info));
	}

    peu_config_t *pcie_cfg = (peu_config_t *)buff;
    if (gd->m2_flag){
        pcie_cfg->ctr_cfg[0].base_config[1] &= ~(1<<PEU_C_OFFSET_PORT_DISABLE);
        pcie_cfg->base_cfg = ((pcie_cfg->base_cfg & 0xffff000) | (CONFIG_PCI_PEU0 | (X1X0 << 1)));
    }

	arm_smccc_smc(CPU_INIT_PCIE, 0, (u64)buff, 0, 0, 0, 0, 0, &res);
	if(0 != res.a0){
		p_printf("error ret  %lx\n", res.a0);
		while(1);
	}
}

#ifdef CONFIG_PCI_ARID
#define EFI_PCIE_CAPABILITY_BASE_OFFSET                             0x100
#define EFI_PCIE_CAPABILITY_ID_SRIOV_CONTROL_ARI_HIERARCHY          0x10
#define EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_OFFSET            0x24
#define EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_ARI_FORWARDING    0x20
#define EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET                 0x28
#define EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING         0x20

void board_pci_fixup_dev(struct udevice *bus, struct udevice *dev)
{
    u32 data;
    int dev_exp_cap, bus_exp_cap, dev_ari_cap;//用来获取返回的offset

	/* step 1 Detect if PCI Express Device */
	dev_exp_cap = dm_pci_find_capability(dev, PCI_CAP_ID_EXP);
    debug("dev_exp_cap offset is %d\n", dev_exp_cap);

	/* step 2 Check if the device is an ARI device.*/
	if(dev_exp_cap > 0) {
		dev_ari_cap = dm_pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ARI);
        debug("dev_ari_cap offset is %d\n", dev_ari_cap);

		/* step 3 Check if its parent supports ARI forwarding. */
		if(dev_ari_cap > 0) {		//获取到了ARI功能寄存器的offset
			bus_exp_cap = dm_pci_find_capability(bus, PCI_CAP_ID_EXP);
            debug("bus_exp_cap offset is %d\n", bus_exp_cap);
			if(bus_exp_cap > 0) {
				dm_pci_read_config32(bus, bus_exp_cap + EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_OFFSET, &data);
				if ((data & EFI_PCIE_CAPABILITY_DEVICE_CAPABILITIES_2_ARI_FORWARDING) != 0) {
					/*step 4 ARI forward support in bridge, so enable it. */
					dm_pci_read_config32(bus, bus_exp_cap + EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET, &data);
					if ((data & EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING) == 0) {
						data |= EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_ARI_FORWARDING;
						dm_pci_write_config32(bus, bus_exp_cap + EFI_PCIE_CAPABILITY_DEVICE_CONTROL_2_OFFSET, data);
					}
				}
			}
		}
	}
}
#endif
