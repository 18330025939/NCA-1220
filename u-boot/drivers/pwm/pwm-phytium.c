// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium pwm controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */
#include <dm.h>
#include <pwm.h>
#include <linux/io.h>


#define REG_TCNT		0x00
#define REG_TCTRL		0x04
#define REG_STAT		0x08
#define REG_PWMPERIOD	0x0c
#define REG_PWMCTRL		0x10
#define REG_PWMCCR		0x14

#define PWM_GECTL       0x2807e020
#define PWM_NUM  2

#define SW_RST          0x01
#define PWM_N(x)		((0x400)*(x+1))

#define PWM_CLK_MHZ     50

struct ft_pwm_priv {
	void __iomem *base;
	
};

static int ft_pwm_reset(void __iomem *base, uint channel)
{
	u32 val;
	iowrite32(SW_RST,base+PWM_N(channel)+REG_TCTRL);
	
	do{
		val = ioread32(base+PWM_N(channel)+REG_TCTRL);
	}while(val&SW_RST);
	
	return 0;
}

static int ft_pwm_set_enable(struct udevice *dev, uint channel, bool enable)
{
	u32 val,cur_status;
	struct ft_pwm_priv *priv = dev_get_priv(dev);
	
	if(channel>=PWM_NUM)
		return -EINVAL;
	
	val = ioread32(priv->base+PWM_N(channel)+REG_TCTRL);
	cur_status=!!(val&BIT(1));

	if(!(enable ^ cur_status))
		return 0;

	if(enable)
		val |= BIT(1);
	else
		val &= ~(BIT(1));

	iowrite32(val,priv->base+PWM_N(channel)+REG_TCTRL);

	return 0;
}

static int ft_pwm_set_config(struct udevice *dev, uint channel,
			       uint period_ns, uint duty_ns)
{
	u32 period_cnt,duty_cnt,val;
	struct ft_pwm_priv *priv = dev_get_priv(dev);
	
	if(channel>=PWM_NUM)
		return -EINVAL;

	if(duty_ns>period_ns)
		return -EINVAL;

	if(period_ns>0)
		period_ns--;
	if(duty_ns>0)
		duty_ns--;
	
	period_cnt=period_ns/(1000/PWM_CLK_MHZ);
	duty_cnt=duty_ns/(1000/PWM_CLK_MHZ);

	iowrite32(period_cnt,priv->base+PWM_N(channel)+REG_PWMPERIOD);
	
	val=ioread32(priv->base+PWM_N(channel)+REG_PWMCTRL);
	if(!(val&0xf0))
		iowrite32(BIT(2)|BIT(6),priv->base+PWM_N(channel)+REG_PWMCTRL);

	iowrite32(duty_cnt,priv->base+PWM_N(channel)+REG_PWMCCR);
	
	val=ioread32(priv->base+PWM_N(channel)+REG_TCTRL);
	val |= BIT(5);
	iowrite32(val,priv->base+PWM_N(channel)+REG_TCTRL);
	return 0;
}

static int ft_pwm_set_invert(struct udevice *dev, uint channel,
			       bool polarity)
{
	u32 val;
	struct ft_pwm_priv *priv = dev_get_priv(dev);
	
	if(channel>=PWM_NUM)
		return -EINVAL;

	val=ioread32(priv->base+PWM_N(channel)+REG_PWMCTRL);
	val &= ~0xf0;
	if(polarity)
		val |= 0x40;
	else
		val |= 0x30;

	iowrite32(val,priv->base+PWM_N(channel)+REG_PWMCTRL);
	return 0;
}


static int ft_pwm_probe(struct udevice *dev)
{
	struct ft_pwm_priv *priv = dev_get_priv(dev);
	priv->base = dev_read_addr_ptr(dev);
	
	if (!priv->base)
		return -ENODATA;

	ft_pwm_reset(priv->base,0);
	ft_pwm_reset(priv->base,1);

	iowrite32(0xff,(void __iomem *)PWM_GECTL);
	return 0;
}


static const struct pwm_ops ft_pwm_ops = {
	.set_config = ft_pwm_set_config,
	.set_enable = ft_pwm_set_enable,
	.set_invert = ft_pwm_set_invert,
};

static const struct udevice_id ft_pwm_of_match[] = {
	{ .compatible = "phytium,phytium-pwm" },
	{ }
};

U_BOOT_DRIVER(ft_pwm) = {
	.name = "phytium_pwm",
	.id = UCLASS_PWM,
	.of_match = ft_pwm_of_match,
	.probe = ft_pwm_probe,
	.priv_auto = sizeof(struct ft_pwm_priv),
	.ops = &ft_pwm_ops,
};

