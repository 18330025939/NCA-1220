// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <e_uart.h>
#include "power_manage.h"
#include "../parameter/parameter.h"

void send_pwr_cmd(uint32_t cmd)
{
	switch(pm_get_power_manager()){
	case 0:
		send_cpld_ctr(cmd);
		break;
	default:
		while(1);
	}
}

int get_s3_flag(void)
{
	uint32_t ret = 0;
	switch(pm_get_power_manager()){
	case 0:
		ret = gpio_get_s3_flag();
		break;
	default:
		while(1);
	}

	return ret;
}

void pwr_s3_clean(void)
{
	send_pwr_cmd(S3_CLEAN_CPLD);
}

void pwr_reboot(void)
{
	send_pwr_cmd(REBOOT_CPLD);
}

void pwr_vtt_disable(void)
{
	send_pwr_cmd(VTT_DISABLE_CPLD);
}

void pwr_vtt_enable(void)
{
	send_pwr_cmd(VTT_ENABLE_CPLD);
}

void pwr_s3_setup(void)
{
	send_pwr_cmd(S3_SETUP_CPLD);
}

void pwr_shutdown(void)
{
	send_pwr_cmd(SHUTDOWN_CPLD);
}
