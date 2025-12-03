// SPDX-License-Identifier: GPL-2.0+
/*
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <dm/pinctrl.h>
#include <dt-bindings/pinctrl/pinctrl-phytium.h>

char ft_group_count = 0;
fdt_addr_t ft_pin_mux_base, ft_func;

void dump_mux_reg(void * addr, const MUX_field_desc_and_len_t *pdl)
{
	uchar i;
	unsigned int mask_in_place, regval;
	const pin_mux_filed_desc_t *pdesc;

	regval = readl(addr);

	printf("(0x%08x)@(0x%p)\n", regval, addr);
	printf("BitRange  FuncId   MuxName\n");
	printf("__________________________\n");

	for (i = 0; i < (pdl->len); i++) {
		pdesc = &pdl->pdesc[i];

		mask_in_place = pdesc->mask << pdesc->lo;

		if (pdesc->hi == pdesc->lo)
			printf("%2u   ", pdesc->lo);
		else
			printf("%2u-%2u", pdesc->hi, pdesc->lo);
		printf("	%d	%s",
			(regval & mask_in_place) >> pdesc->lo,
			pdesc->func[(regval & mask_in_place) >> pdesc->lo]);

		printf("\n");

	}
	printf("__________________________\n");
}

static int ft2400_pinctrl_probe(struct udevice *dev)
{
	char grp[] = {'b', 'a', 'n', 'k', '0' ,'0' ,'\0'};

	printf("PIN_MUX: %s : %d\n", __func__, __LINE__);

	fdt_size_t size;

	/*	get base addr*/
	ft_pin_mux_base = fdtdec_get_addr_size(gd->fdt_blob, dev_of_offset(dev), "reg",
				    &size);

	while(1)
		{
			while(1){

				//printf("grp = %s\n", grp);
				//printf("group_count= %d\n", group_count);

				ft_func = fdtdec_get_uint64(gd->fdt_blob, dev_of_offset(dev), grp, 0);

				if(ft_func == 0xf || ft_func == 0xff)
					break;

				setbits_32(ft_pin_mux_base + ft_group_count/8*4, ft_func << (7 - ft_group_count%8)*4);
				ft_group_count++;
				grp[5] += 1;

				//printf("grp[4] = %s\n", grp);
				//printf("group_count= %d\n", group_count);
			}

			grp[4] += 1;
			grp[5] = '0';
			//printf("grp[4] = %s\n", grp[4]);
		#ifdef	CONFIG_FT2004_PINCTRL_DEBUG
			dump_mux_reg((ft_pin_mux_base + (ft_group_count-1)/8*4), &desc_and_len_tbl[(ft_group_count-1)/8]);
		#endif
			if(ft_func == 0xff)
				break;
		}

	//unsigned int value;
	//value = readl(pin_mux_base);
	//dump_mux_reg(pin_mux_base, &desc_and_len_tbl[0]);
	//value = readl(pin_mux_base + (group_count-1)/8*4);
	//dump_mux_reg((pin_mux_base + (group_count-1)/8*4), &desc_and_len_tbl[1]);

	return 0;
}

static int ft2400_pinctrl_get_groups_count(struct udevice *dev)
{
	printf("PINCTRL: get_(functions/groups)_count\n");

	return 1;
}

static const char *ft2400_pinctrl_get_group_name(struct udevice *dev, unsigned selector)
{
	const pin_mux_filed_desc_t *pdesc = desc_and_len_tbl->pdesc;

	printf("PINCTRL: get_(function/group)_name %u\n", selector);

	return pdesc->func[selector];
}

static int ft2400_pinctrl_group_set(struct udevice *dev, unsigned selector, unsigned func_selector)
{
	printf("PINCTRL: get_(function/group)_name %u\n", selector);

	return 1;
}

static struct pinctrl_ops ft2400_pinctrl_ops = {
	.get_groups_count = ft2400_pinctrl_get_groups_count,
	.get_group_name = ft2400_pinctrl_get_group_name,
	.get_functions_count = ft2400_pinctrl_get_groups_count,
	.get_function_name = ft2400_pinctrl_get_group_name,
	.pinmux_group_set = ft2400_pinctrl_group_set,
};

static const struct udevice_id ft2400_pinctrl_ids[] = {
	{ .compatible = "phytium,ft2004-pinctrl" },
	{ .compatible = "phytium,e2000-pinctrl" },
	{ }
};

U_BOOT_DRIVER(pinctrl_ft2400) = {
	.name = "phytium_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ft2400_pinctrl_ids,
	.ops = &ft2400_pinctrl_ops,
	.probe = ft2400_pinctrl_probe,
};
