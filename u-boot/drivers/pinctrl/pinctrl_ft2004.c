// SPDX-License-Identifier: GPL-2.0+
/*
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <dm/pinctrl.h>

typedef struct _pin_mux_field_desc_t {
	ushort hi;
	ushort lo;
	ushort mask;
	char * func[3];
} pin_mux_filed_desc_t;

static const pin_mux_filed_desc_t reg_0_desc_tbl[] = {
	{ 29, 28, 0x3, {"all_pll_lock", "(null)", "i2c_1_scl"}	},			//all_pll_lock_pad
	{ 25, 24, 0x3, {"cru_clk_obv" , "gpio0_porta_0", "i2c_1_sda"}	},	//cru_clk_obv_pad
	{ 21, 20, 0x3, {"(null)", "(null)", "(null)"}             },
	{ 17, 16, 0x3, {"(null)", "(null)", "(null)"}             },
	{ 13, 12, 0x3, {"(null)", "(null)", "(null)"}             },
	{  9,  8, 0x3, {"(null)", "(null)", "(null)"}             },
	{  5,  4, 0x3, {"(null)", "(null)", "(null)"}             },
	{  1,  0, 0x3, {"(null)", "(null)", "(null)"}             }
};

static const pin_mux_filed_desc_t reg_1_desc_tbl[] = {
	{ 29, 28, 0x3, {"(null)", "gpio0_porta_7", "peu1_linkup_0"}	},	//tjtag_tms_pad
	{ 25, 24, 0x3, {"(null)", "gpio0_porta_0", "peu1_linkup_1"}	},	//tjtag_tdi_pad
	{ 21, 20, 0x3, {"(null)", "(null)", "(null)"}             },
	{ 17, 16, 0x3, {"(null)", "(null)", "(null)"}             },
	{ 13, 12, 0x3, {"(null)", "(null)", "(null)"}             },
	{  9,  8, 0x3, {"(null)", "(null)", "(null)"}             },
	{  5,  4, 0x3, {"(null)", "(null)", "(null)"}             },
	{  1,  0, 0x3, {"(null)", "(null)", "(null)"}             }
};


typedef struct _MUX_field_desc_and_len_t {
	const pin_mux_filed_desc_t *pdesc;
	ushort len;
} MUX_field_desc_and_len_t;

static const MUX_field_desc_and_len_t desc_and_len_tbl[] = {
	{ reg_0_desc_tbl, ARRAY_SIZE(reg_0_desc_tbl)   },
	{ reg_1_desc_tbl, ARRAY_SIZE(reg_1_desc_tbl)   },

};

static void dump_mux_reg(unsigned         regval,
	const MUX_field_desc_and_len_t *pdl)
{
	uchar i;
	unsigned mask_in_place;
	const pin_mux_filed_desc_t *pdesc;

	printf("(%04x)\n", regval);
	printf("BitRange  FuncId   MuxName\n");
	printf("__________________________\n");

	for (i = 0; i < pdl->len; i++) {
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
	printf("\n");
}


static int ft2400_pinctrl_probe(struct udevice *dev)
{

	unsigned int value;
	char grp[] = {'b', 'a', 'n', 'k', '0' ,'\0'};
	char group_count = 0;

	printf("PIN_MUX : %s : %d\n", __func__, __LINE__);

	fdt_addr_t pin_mux_base, func;
	fdt_size_t size;

	/*	get base addr*/
	pin_mux_base = fdtdec_get_addr_size(gd->fdt_blob, dev_of_offset(dev), "reg",
				    &size);

	while(1){

		grp[4] = '0' + group_count;

		func = fdtdec_get_uint64(gd->fdt_blob, dev_of_offset(dev), grp, 0);

		if(func == 0xff)
			break;

		setbits_32(pin_mux_base + group_count/8*4, func << (7 - group_count)*4);
		group_count++;
	}

	value = readl(pin_mux_base);
	dump_mux_reg(value, desc_and_len_tbl);

	return 0;
}

static int ft2400_pinctrl_get_groups_count(struct udevice *dev)
{
	printf("PINCTRL: get_(functions/groups)_count\n");

	return 1;
}

static const char *ft2400_pinctrl_get_group_name(struct udevice *dev,
						  unsigned selector)
{
	const pin_mux_filed_desc_t *pdesc = desc_and_len_tbl->pdesc;

	printf("PINCTRL: get_(function/group)_name %u\n", selector);

	return pdesc->func[selector];
}

static int ft2400_pinctrl_group_set(struct udevice *dev, unsigned selector,
				     unsigned func_selector)
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
	{ }
};

U_BOOT_DRIVER(pinctrl_ft2400) = {
	.name = "ft2004_pinctrl",
	.id = UCLASS_PINCTRL,
	.of_match = ft2400_pinctrl_ids,
	.ops = &ft2400_pinctrl_ops,
	.probe = ft2400_pinctrl_probe,
};
