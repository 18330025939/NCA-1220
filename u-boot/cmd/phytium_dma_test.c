// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium gdma controller driver
 *
 * Copyright (C) 2022 Phytium Corporation
 *
 */

#include <command.h>
#include <dm.h>
#include <dma.h>
#include <test/ut.h>


static int dm_test_dma_m2m(const char * dev_anme)
{
	struct udevice *dev;
	struct dma dma_m2m;
	u8 src_buf[512];
	u8 dst_buf[512];
	size_t len = 512;
	int i,ret;

	ret=uclass_get_device_by_name(UCLASS_DMA, dev_anme, &dev);
	if (ret) {
		pr_err("Can't get the DMA %s: %d\n", dev_anme, ret);
		return CMD_RET_FAILURE;
	}
	
	ret=dma_get_by_name(dev, "m2m", &dma_m2m);
	if(ret)
		return CMD_RET_FAILURE;

	memset(dst_buf, 0, len);
	for (i = 0; i < len; i++)
		src_buf[i] = i;

	ret=dma_memcpy(dst_buf, src_buf, len);
	if(ret)
		return CMD_RET_FAILURE;

	for (i = 0; i < len; i++)
	{
		if(src_buf[i] != dst_buf[i])
			return CMD_RET_FAILURE;
	}
	
	return CMD_RET_SUCCESS;
}

static int do_dma(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	int ret;
	const char * dev_name;
	
	if (argc < 2)
		return CMD_RET_USAGE;

	dev_name=argv[1];

	ret = dm_test_dma_m2m(dev_name);
	if(ret == CMD_RET_SUCCESS)
		printf("DMA m2m test success\n");
	else
		printf("DMA m2m test fail\n");
	return ret;
}


U_BOOT_CMD(dma, 2, 0, do_dma,
	   "control dma channels",
	   "<name> - enable dma");

