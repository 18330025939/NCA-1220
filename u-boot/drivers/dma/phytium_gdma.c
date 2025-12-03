// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium gdma controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */
#include <dm.h>
#include <dma-uclass.h>
#include <asm/io.h>



#define FT_DMA_CTRL        		0x00
#define FT_DMA_CX_CTRL(x)  		(0x20 + x*0x60)
#define FT_DMA_CX_MODE(x)  		(0x24 + x*0x60)
#define FT_DMA_CX_STATE(x)      (0x2c + x*0x60)
#define FT_DMA_CX_TS(x)    		(0x34 + x*0x60)
#define FT_DMA_CX_LWSADDR(x)  	(0x3c + x*0x60)
#define FT_DMA_CX_LWDADDR(x)  	(0x44 + x*0x60)
#define FT_DMA_CX_XFER_CFG(x)  	(0x48 + x*0x60)
#define FT_DMA_ENABLE_MASK	    BIT(0)
#define FT_DMA_OT_CTL_MASK		GENMASK(11, 8)
#define FT_CX_MODE_DIRECT       0x00
#define FT_CX_ENABLE_MASK       BIT(0)
#define FT_CHAN_TRANS_END       BIT(3)

#define FT_DMA_CX_ARLEN         BIT(26)
#define FT_DMA_CX_ARSIZE        0x0
#define FT_DMA_CX_ARBURST		BIT(16)
#define FT_DMA_CX_AWLEN         BIT(10)
#define FT_DMA_CX_AWSIZE        0x0
#define FT_DMA_CX_AWBURST		BIT(0)


struct ft_gdma_dev {
	void __iomem		*regs;
};

void __ft_gdma_transfer(void __iomem *gdma_base_addr, unsigned int gdma_chan_num,
		      void *dst, void *src, size_t len)
{
	u32 ret;
	
	writel(FT_DMA_ENABLE_MASK|FT_DMA_OT_CTL_MASK,gdma_base_addr + FT_DMA_CTRL);
	writel(FT_CX_MODE_DIRECT,gdma_base_addr + FT_DMA_CX_MODE(gdma_chan_num));
	writel((u64)src,gdma_base_addr + FT_DMA_CX_LWSADDR(gdma_chan_num));
	writel((u64)dst,gdma_base_addr + FT_DMA_CX_LWDADDR(gdma_chan_num));

	ret = FT_DMA_CX_ARLEN|FT_DMA_CX_ARSIZE|FT_DMA_CX_ARBURST|FT_DMA_CX_AWLEN|FT_DMA_CX_AWSIZE|FT_DMA_CX_AWBURST;
	writel(ret,gdma_base_addr + FT_DMA_CX_XFER_CFG(gdma_chan_num));
	writel(len,gdma_base_addr + FT_DMA_CX_TS(gdma_chan_num));
	writel(FT_CX_ENABLE_MASK,gdma_base_addr + FT_DMA_CX_CTRL(gdma_chan_num));
	
	do{
		ret = readl(gdma_base_addr + FT_DMA_CX_STATE(gdma_chan_num));
	}while(!(ret&FT_CHAN_TRANS_END));
	writel(ret,gdma_base_addr + FT_DMA_CX_STATE(gdma_chan_num));
}

static int ft_gdma_transfer(struct udevice *dev, int direction, void *dst,
			     void *src, size_t len)
{
	struct ft_gdma_dev *priv = dev_get_priv(dev);

	switch (direction) {
	case DMA_MEM_TO_MEM:
		__ft_gdma_transfer(priv->regs, 0, dst, src, len);
		break;
	default:
		pr_err("Transfer type not implemented in DMA driver\n");
		break;
	}

	return 0;
}

static int ft_gdma_probe(struct udevice *dev)
{
	struct ft_gdma_dev *priv = dev_get_priv(dev);
	struct dma_dev_priv *uc_priv = dev_get_uclass_priv(dev);
	int delay=100;
	u32 reg_val;

	priv->regs = dev_read_addr_ptr(dev);
	uc_priv->supported = DMA_SUPPORTS_MEM_TO_MEM;

	reg_val =readl(priv->regs + FT_DMA_CTRL);
	reg_val &= ~BIT(0);
	writel(reg_val,priv->regs + FT_DMA_CTRL);

	reg_val =readl(priv->regs + FT_DMA_CTRL);
	reg_val |= BIT(1);
	writel(reg_val,priv->regs + FT_DMA_CTRL);
	
	while(--delay>0)
		;

	reg_val =readl(priv->regs + FT_DMA_CTRL);
	reg_val &= ~BIT(1);
	writel(reg_val,priv->regs + FT_DMA_CTRL);
	return 0;
}


static const struct dma_ops ft_gdma_ops = {
	.transfer	= ft_gdma_transfer,
};


static const struct udevice_id ft_gdma_ids[] = {
	{ .compatible = "phytium,phytium-gdma" },
	{ }
};

U_BOOT_DRIVER(ft_gdma) = {
	.name	= "phytium_gdma",
	.id	= UCLASS_DMA,
	.of_match = ft_gdma_ids,
	.ops	= &ft_gdma_ops,
	.probe = ft_gdma_probe,
	.priv_auto	= sizeof(struct ft_gdma_dev),
};

