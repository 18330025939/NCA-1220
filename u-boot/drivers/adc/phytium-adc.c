// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium adc controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */
#include <dm.h>
#include <adc.h>
#include <linux/io.h>
#include <linux/bitmap.h>


#define   ADC_CTRL_REG					0x00
#define   ADC_CTRL_REG_PD_EN			BIT(31)
#define   ADC_CTRL_REG_CH_ONLY_EN		BIT(3)
#define   ADC_CTRL_REG_SINGLE_EN		BIT(2)
#define   ADC_CTRL_REG_SINGLE_SEL		BIT(1)
#define   ADC_CTRL_REG_SOC_EN			BIT(0)
#define   ADC_CTRL_REG_CH_ONLY_S(x)		((x & 0x7) << 16)
#define   ADC_CTRL_REG_CLK_DIV(x)		((x) << 12)
#define   ADC_CTRL_REG_CHANNEL_EN(x)		BIT((x) + 4)
#define   ADC_INTRMASK_REG			        0x30
#define   ADC_INTRMASK_REG_ERR_INTR_MASK	BIT(24)

#define   ADC_INTR_REG				    0x34
#define   ADC_COV_RESULT_REG(x)			(0x38 + ((x) << 2))
#define   ADC_COV_RESULT_REG_MASK		GENMASK(9, 0)
#define   FT_MAX_CHANNELS          8
#define   FT_ADC_TIMEOUT	        20 

struct ft_adc_priv{
	int active_channel;
	void __iomem *regs;

};

static void ft_adc_intrmask_setup(struct ft_adc_priv *priv, unsigned long chan_mask, bool on)
{
	u32 reg;
	u16 limit_mask = 0;
	int ch;

	for_each_set_bit(ch, &chan_mask, FT_MAX_CHANNELS)
		limit_mask |= BIT(ch << 1) | BIT((ch << 1) + 1);

	reg = readl(priv->regs + ADC_INTRMASK_REG);
	if (on)
		reg &= ~(ADC_INTRMASK_REG_ERR_INTR_MASK |
			 (limit_mask << 8) | chan_mask);
	else
		reg |= (ADC_INTRMASK_REG_ERR_INTR_MASK |
			(limit_mask << 8) | chan_mask);
	writel(reg, priv->regs + ADC_INTRMASK_REG);
}

static void ft_adc_single_conv_setup(struct ft_adc_priv *priv, u8 ch)
{
	u32 reg;

	reg = readl(priv->regs + ADC_CTRL_REG);

	/* Clean ch_only_s bits */
	reg &= ~ADC_CTRL_REG_CH_ONLY_S(7);

	/* Clean channel_en bit */
	reg &= 0xFFF00F;

	reg |=  ADC_CTRL_REG_SINGLE_SEL | ADC_CTRL_REG_SINGLE_EN |
		ADC_CTRL_REG_CH_ONLY_EN | ADC_CTRL_REG_CH_ONLY_S(ch) | ADC_CTRL_REG_CHANNEL_EN(ch);
	writel(reg, priv->regs + ADC_CTRL_REG);
}

static void ft_adc_start_stop(struct ft_adc_priv *priv, bool start)
{
	u32 ctrl;

	ctrl = readl(priv->regs + ADC_CTRL_REG);
	if (start)
		ctrl |= ADC_CTRL_REG_SOC_EN | ADC_CTRL_REG_SINGLE_EN;
	else
		ctrl &= ~ADC_CTRL_REG_SOC_EN;
	/* Start conversion */
	writel(ctrl, priv->regs + ADC_CTRL_REG);
}

static void ft_adc_power_setup(struct ft_adc_priv *priv, bool on)
{
	u32 reg;

	reg = readl(priv->regs + ADC_CTRL_REG);
	if (on)
		reg &= ~ADC_CTRL_REG_PD_EN;
	else
		reg |= ADC_CTRL_REG_PD_EN;
	writel(reg, priv->regs + ADC_CTRL_REG);
}


int ft_adc_channel_data(struct udevice *dev, int channel,
			    unsigned int *data)
{
	u32 intr;
	struct ft_adc_priv *priv = dev_get_priv(dev);

	if (channel != priv->active_channel) {
		pr_err("Requested channel is not active!");
		return -EINVAL;
	}

	intr = readl(priv->regs + ADC_INTR_REG);
	if(!(intr&BIT(channel)))
		return -EBUSY;

	*data = readl(priv->regs + ADC_COV_RESULT_REG(channel));

    writel(intr,priv->regs + ADC_INTR_REG);
	ft_adc_start_stop(priv, false);
	ft_adc_intrmask_setup(priv, BIT(channel), false);

	return 0;
}

int ft_adc_start_channel(struct udevice *dev, int channel)
{
	struct ft_adc_priv *priv = dev_get_priv(dev);

	ft_adc_intrmask_setup(priv, BIT(channel), true);
	ft_adc_single_conv_setup(priv, channel);
	ft_adc_start_stop(priv, true);

	priv->active_channel = channel;
	return 0;
}



int ft_adc_probe(struct udevice *dev)
{
	u32 reg;
	struct ft_adc_priv *priv = dev_get_priv(dev);
	/*
	 * Setup ctrl register:
	 * - Power up conversion module
	 * - Set the division by 4 as default
	 */
	reg = ADC_CTRL_REG_CLK_DIV(4);
	writel(reg, priv->regs + ADC_CTRL_REG);

	/* Set all the interrupt mask, unmask them when necessary. */
	writel(0x1ffffff, priv->regs + ADC_INTRMASK_REG);

	ft_adc_power_setup(priv, true);

	priv->active_channel = -1;
	
	return 0;
}


int ft_adc_of_to_plat(struct udevice *dev)
{
	struct adc_uclass_plat *uc_pdata = dev_get_uclass_plat(dev);
	struct ft_adc_priv *priv = dev_get_priv(dev);
	
	priv->regs = dev_read_addr_ptr(dev);
	if (!priv->regs)
		return -ENODATA;
	
	uc_pdata->data_mask = ADC_COV_RESULT_REG_MASK;
	uc_pdata->data_format = ADC_DATA_FORMAT_BIN;
	uc_pdata->data_timeout_us = FT_ADC_TIMEOUT;
	
	/* Mask available channel bits: [0:7] */
	uc_pdata->channel_mask = (1 << FT_MAX_CHANNELS) - 1;

	return 0;
}


static const struct adc_ops ft_adc_ops = {
	.start_channel = ft_adc_start_channel,
	.channel_data = ft_adc_channel_data,
};

static const struct udevice_id ft_adc_ids[] = {
	{ .compatible = "phytium,phytium-adc" },
	{ }
};

U_BOOT_DRIVER(ft_adc) = {
	.name		= "phytium_adc",
	.id		= UCLASS_ADC,
	.of_match	= ft_adc_ids,
	.ops		= &ft_adc_ops,
	.probe		= ft_adc_probe,
	.of_to_plat = ft_adc_of_to_plat,
	.priv_auto	= sizeof(struct ft_adc_priv),
};

