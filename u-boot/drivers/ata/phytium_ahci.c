// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium SATA platform driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */

#include <common.h>
#include <dm.h>
#include <ahci.h>
#include <scsi.h>
#include <asm/io.h>

struct ft_ahci_priv {
	void *base;
	void *wrapper_base;
};

static int ft_ahci_bind(struct udevice *dev)
{
	struct udevice *scsi_dev;

	return ahci_bind_scsi(dev, &scsi_dev);
}

static int ft_ahci_ofdata_to_platdata(struct udevice *dev)
{
	struct ft_ahci_priv *priv = dev_get_priv(dev);
	fdt_addr_t addr;

	priv->base = map_physmem(devfdt_get_addr(dev), sizeof(void *),
				 MAP_NOCACHE);

	addr = devfdt_get_addr_index(dev, 1);
	if (addr != FDT_ADDR_T_NONE) {
		priv->wrapper_base = map_physmem(addr, sizeof(void *),
						 MAP_NOCACHE);
	} else {
		priv->wrapper_base = NULL;
	}

	return 0;
}

static int ft_ahci_probe(struct udevice *dev)
{
	struct ft_ahci_priv *priv = dev_get_priv(dev);

	return ahci_probe_scsi(dev, (ulong)priv->base);
}

static const struct udevice_id ft_ahci_ids[] = {
	{ .compatible = "phytium,phytium_ahci" },
	{ }
};

U_BOOT_DRIVER(ft_ahci) = {
	.name	= "phytium_ahci",
	.id	= UCLASS_AHCI,
	.of_match = ft_ahci_ids,
	.bind	= ft_ahci_bind,
	.of_to_plat = ft_ahci_ofdata_to_platdata,
	.ops	= &scsi_ops,
	.probe	= ft_ahci_probe,
	.priv_auto = sizeof(struct ft_ahci_priv),
};
