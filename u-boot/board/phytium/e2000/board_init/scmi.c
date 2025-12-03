// SPDX-License-Identifier: GPL-2.0+
/*
 * Phytium SCMI API
 *
 * Copyright (C) 2022 Phytium Corporation
 */

#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <malloc.h>
#include <scmi_protocols.h>
#include "../cpu.h"
#include "board.h"

#define SCMI_MAX_PARM_CPUNT 10
#define MHU_UBOOT_SCP_CHN   1
#define MHU_SET             0x8

void scmi_send_sync_command(mailbox_mem_e_t *mbx_mem)
{
    SCMI_MARK_CHANNEL_BUSY(mbx_mem->status);
	dmb();
	writel(mbx_mem->msg_header, (MHU_UBOOT_SCP_BASE + MHU_SET));
	dmb();
	/* Wait for channel to be free */
	while (!SCMI_IS_CHANNEL_FREE(mbx_mem->status));
	dmb();
}

//API
int phytium_scmi(uint32_t count, ...)
{
    int length, i;
    va_list args;
    uint32_t *argv;
    uint32_t protocol_id, message_id;
    mailbox_mem_e_t *mbx_mem, *mbx_mem_back;
    
    argv = (uint32_t *)malloc(sizeof(uint32_t) * count); 
    memset(argv, 0, sizeof(int) * count);
 
    writel(0x1, MHU_CONFIG_BASE + 0xc);
    mbx_mem = (mailbox_mem_e_t *)AP_TO_SCP_SHARED_MEM_BASE;

	va_start(args, count);
    protocol_id = va_arg(args, uint32_t);
    message_id = va_arg(args, uint32_t);
    for(i=0; i<(count-1); i++){
	    mbx_mem->payload[i] = va_arg(args, uint32_t);
    }
    va_end(args);
    
    printf("Phytium scmi:\n");
    printf("AP -> SCP:\n");
    printf("protocol_id:0x%x\nmessage_id:0x%x\n", protocol_id, message_id);
    for (i=2; i<=(count-2); i++){
        printf("payload[%d]:0x%x\n", i, argv[i]);
    }

    mbx_mem->len = (sizeof(mbx_mem->msg_header) + ((count-2)*4));
	mbx_mem->msg_header = SCMI_MSG_CREATE_E(protocol_id, message_id, 0);
	mbx_mem->flags = 0x0;
	mbx_mem->status = 0x0;
	writel(mbx_mem->msg_header, MHU_UBOOT_SCP_BASE + 0x8);
    
    scmi_send_sync_command(mbx_mem);
    mbx_mem_back = mbx_mem; 
    
    if(SCMI_SUCCESS != mbx_mem_back->payload[0]){
        printf("SCMI ERROR:protocol_id:0x%x, message_id:0x%x, error is:0x%x\n", protocol_id, message_id, (mbx_mem_back->payload[0]));
        return -1;
    }else{
        printf("SCP -> AP:\n");
        for((length=(mbx_mem_back->len)-sizeof(mbx_mem_back->msg_header)),i=0;length>0;i++){
            printf("payload[%d]:0x%x\n", i, mbx_mem_back->payload[i]);
            length -= 4;
        }
    }

    return 0;
}

//cmdline
static int do_scmi(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
    //[0]="scmi",[1]=count,[2]=protocol_id,[3]=message_id,[n]=payload
    uint32_t parm[SCMI_MAX_PARM_CPUNT] = {0};
    uint32_t count = simple_strtoul(argv[1], NULL, 16);

	if (argc < 4)
		return CMD_RET_USAGE;
    
    for (int i=0; i<count; i++){
        parm[i] = simple_strtoul(argv[i+2], NULL, 16);
    }
    
    //phytium_scmi(2, 0x10, 0x0);//demo,get scmi version
    phytium_scmi(count, parm[0], parm[1], parm[2], parm[3], parm[4], parm[5], parm[6], parm[7], parm[8], parm[9]);

    return 0;
}

U_BOOT_CMD(
    scmi, 10, 0, do_scmi,
    "scmi command",
    "[protocol_id] [count-max:10] [message_id] [payload...]"
);
