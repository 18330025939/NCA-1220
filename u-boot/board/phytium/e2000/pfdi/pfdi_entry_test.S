// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * lixinde <lixinde@phytium.com.cn>
 */

#include <config.h>
#include <linux/linkage.h>


/*************************************************************************
 *
 * Phytium Firmware Dispatch Interface
 *
 *************************************************************************/

ENTRY(e2000_system_off_entry)
	ldr x1, =(0x30c40000 - 0x1000)
	/*fix GD*/
	mov	x18, x1
	
	mov	x2, x1
	sub x2, x2, 0x1000
	mov	sp, x2

	b	pfdi_service_system_off
ENDPROC(e2000_system_off_entry)

ENTRY(e2000_system_reset_entry)
	ldr x1, =(0x30c40000 - 0x1000)
	/*fix GD*/
	mov	x18, x1
	
	mov	x2, x1
	sub x2, x2, 0x1000
	mov	sp, x2	

	b	pfdi_service_system_reset
ENDPROC(e2000_system_reset_entry)

ENTRY(e2000_suspend_finish_entry)
	ldr x1, =(0x30c40000 - 0x1000)
	/*fix GD*/
	mov	x18, x1

	mov	x2, x1
	sub x2, x2, 0x1000
	mov	sp, x2

	b	pfdi_service_suspend_finish
ENDPROC(e2000_suspend_finish_entry)

ENTRY(e2000_suspend_start_entry)
	ldr x1, =(0x30c40000 - 0x1000)
	/*fix GD*/
	mov	x18, x1
	
	mov	x2, x1
	sub x2, x2, 0x1000
	mov	sp, x2

	mov	sp, x1
	b	pfdi_service_suspend_start
ENDPROC(e2000_suspend_start_entry)

ENTRY(e2000_suspend_end_entry)
	ldr 	x3, =0x28001000
	mov 	w4, 'F'
	strb	w4, [x3]

	ldr x1, =(0x30c40000 - 0x1000)
	/*fix GD*/
	mov	x18, x1
	
	mov	x2, x1
	sub x2, x2, 0x1000
	mov	sp, x2

	b	pfdi_service_suspend_end
ENDPROC(e2000_suspend_end_entry)

