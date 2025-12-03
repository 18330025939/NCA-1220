// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022
 * Phytium Technology Ltd <www.phytium.com>
 */

#ifndef __POWER_MANAGE_H_
#define __POWER_MANAGE_H_

#include <linux/delay.h>

#define S3_SETUP_CPLD			8
#define S3_CLEAN_CPLD			1
#define REBOOT_CPLD				4
#define VTT_DISABLE_CPLD        5
#define VTT_ENABLE_CPLD         6
#define SHUTDOWN_CPLD			12

typedef struct pad_info{
	uint32_t pad_sel_reg;
	uint32_t bit_offset;
	uint32_t gpio_swport_ddr;
	uint32_t gpio_ext_port;
	uint32_t port;
}pad_info_t;

int gpio_get_s3_flag(void);
void send_cpld_ctr(uint32_t cmd);
int  get_s3_flag(void);
void pwr_s3_clean(void);
void pwr_reboot(void);
void pwr_vtt_disable(void);
void pwr_vtt_enable(void);
void pwr_s3_setup(void);
void pwr_shutdown(void);

#endif
