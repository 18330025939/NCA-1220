// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium gpio controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */
#include <dm.h>
#include <asm-generic/gpio.h>
#include <linux/io.h>
#include <linux/bitmap.h>
#include <dm/device_compat.h>

#define GPIO_SWPORT_DR	    0x00 
#define GPIO_SWPORT_DDR	    0x04
#define GPIO_EXT_PORT   	0x08

struct ft_gpio_priv {
	void __iomem		*regs;
};

static int ft_gpio_get_value(struct udevice *dev, unsigned offset)
{
	struct ft_gpio_priv *priv = dev_get_priv(dev);
	return !!(readl(priv->regs + GPIO_EXT_PORT) & (1 << offset));
}

static int ft_gpio_set_value(struct udevice *dev, unsigned offset,
			       int value)
{
	struct ft_gpio_priv *priv = dev_get_priv(dev);
	if (value)
		setbits_le32(priv->regs + GPIO_SWPORT_DR, 1 << offset);
	else
		clrbits_le32(priv->regs + GPIO_SWPORT_DR, 1 << offset);

	return 0;
}


static int ft_gpio_direction_input(struct udevice *dev, unsigned offset)
{
	struct ft_gpio_priv *priv = dev_get_priv(dev);

	clrbits_le32(priv->regs + GPIO_SWPORT_DDR, 1 << offset);

	return 0;
}

static int ft_gpio_direction_output(struct udevice *dev, unsigned offset,
				      int value)
{

	struct ft_gpio_priv *priv = dev_get_priv(dev);

	setbits_le32(priv->regs + GPIO_SWPORT_DDR, 1 << offset);

	if (value)
		setbits_le32(priv->regs + GPIO_SWPORT_DR, 1 << offset);
	else
		clrbits_le32(priv->regs + GPIO_SWPORT_DR, 1 << offset);

	return 0;
}


static int ft_gpio_get_function(struct udevice *dev, unsigned offset)
{
	struct ft_gpio_priv *priv = dev_get_priv(dev);
	u32 gpio;

	gpio = readl(priv->regs + GPIO_SWPORT_DDR);

	if (gpio & BIT(offset))
		return GPIOF_OUTPUT;
	else
		return GPIOF_INPUT;

	return 0;
}


static const struct dm_gpio_ops ft_gpio_ops = {
	.direction_input	= ft_gpio_direction_input,
	.direction_output	= ft_gpio_direction_output,
	.get_value		= ft_gpio_get_value,
	.set_value		= ft_gpio_set_value,
	.get_function		= ft_gpio_get_function,
};

static int ft_gpio_probe(struct udevice *dev)
{
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	struct ft_gpio_priv *priv = dev_get_priv(dev);
	u32 count;

	priv->regs = dev_read_addr_ptr(dev);
	uc_priv->bank_name = dev->name;

	if (dev_read_u32(dev, "gpio-ranges", &count)) {
		dev_err(dev, "Missing property gpio-count\n");
		return -EINVAL;
	}

	uc_priv->gpio_count = count;
	return 0;
}


static const struct udevice_id ft_gpio_ids[] = {
	{ .compatible = "phytium,phytium-gpio" },
	{ }
};

U_BOOT_DRIVER(ft_gpio) = {
	.name	= "phytium_gpio",
	.id	= UCLASS_GPIO,
	.of_match = ft_gpio_ids,
	.ops	= &ft_gpio_ops,
	.priv_auto	= sizeof(struct ft_gpio_priv),
	.probe	= ft_gpio_probe,
};


