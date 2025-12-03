#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <cpu_func.h>
#include <linux/arm-smccc.h>
#include <e_uart.h>
#include "pfdi_service.h"
#include "../power_manage/power_manage.h"
#include "../cpu.h"
#include "../board_init/board.h"
#include <asm/system.h>

DECLARE_GLOBAL_DATA_PTR;

typedef  unsigned long u_register_t ;
typedef struct pfdi_vectors_s {
	u_register_t system_off_entry;
	u_register_t system_reset_entry;
	u_register_t suspend_start_entry;
	u_register_t suspend_end_entry;
	u_register_t suspend_finish_entry;
} pfdi_vectors_t;

static void pfdi_runtime_init(u64 gd_b)
{
 	struct global_data *gd_ptr;
 	gd_ptr = (struct global_data *)gd_b;
	memset(gd_ptr, '\0', sizeof(gd_t));
	gd->have_console = 1;
}

void pfdi_service_system_off(u64 pfdi_mum, u64 gd_base)
{
	pfdi_runtime_init(gd_base);
	p_printf("u-boot : get pfdi : %lld , gd_base = 0x%llx\n", pfdi_mum, gd_base);

	pwr_shutdown();
	while(1);
}

void pfdi_service_system_reset(u64 pfdi_mum, u64 gd_base)
{
	pfdi_runtime_init(gd_base);
	p_printf("u-boot : get pfdi : %lld , gd_base = 0x%llx\n", pfdi_mum, gd_base);

	pwr_reboot();
	while(1);
}

void pfdi_service_suspend_finish(u64 pfdi_mum, u64 gd_base)
{
	u_register_t k;
	struct arm_smccc_res res;

	pfdi_runtime_init(gd_base);
	p_printf("u-boot : get pfdi : %lld , gd_base = 0x%llx\n", pfdi_mum, gd_base);

	arm_smccc_smc(PFDI_DONE, (uint64_t)&k, 0, 0, 0, 0, 0, 0, &res);
	while(1);
}

void pfdi_service_suspend_start(u64 pfdi_mum, u64 gd_base)
{
	u_register_t k;
	struct arm_smccc_res res;

	pfdi_runtime_init(gd_base);
	p_printf("u-boot : get pfdi : %lld , gd_base = 0x%llx\n", pfdi_mum, gd_base);

	arm_smccc_smc(PFDI_DONE, (uint64_t)&k, 0, 0, 0, 0, 0, 0, &res);
	while(1);
}

void scmi_send_command_s3s4s5(uint32_t power_state)
{
   p_printf("will send comand\n");
   writel(0x1, MHU_CONFIG_BASE+0xc);


	mailbox_mem_e_t *mbx_mem = (mailbox_mem_e_t*)AP_TO_SCP_SHARED_MEM_BASE;

	mbx_mem->payload[0]=power_state;
	mbx_mem->payload[1]=0x13f;//0xff;
	mbx_mem->payload[2]=0x0;//0xffffffff;//gpionum//bit map

	mbx_mem->len=(sizeof(mbx_mem->msg_header)+12);

	mbx_mem->msg_header=SCMI_MSG_CREATE_E(0x81, 0x5, 0);
	mbx_mem->flags=0x0;

	mbx_mem->status=0x0;
	writel(mbx_mem->msg_header, MHU_UBOOT_SCP_BASE+0x8);
/*
   asm volatile ("dsb sy");
   wfi();
*/
	return;

}

void pfdi_service_suspend_end(u64 pfdi_mum, u64 gd_base)
{
	struct arm_smccc_res res;
	pfdi_runtime_init(gd_base);

	printf("u-boot : get pfdi : %lld , gd_base = 0x%llx\n", pfdi_mum, gd_base);

	arm_smccc_smc(0xC2000F04, 1, 0, 0, 0, 0, 0, 0, &res);
	mdelay(10);
	//pwr_vtt_disable();
	mdelay(40);
	arm_smccc_smc(0xC2000F04, 2, 0, 0, 0, 0, 0, 0, &res);
	//pwr_s3_setup();
	mdelay(10);

	//flush_dcache_all();
	scmi_send_command_s3s4s5(0x1);

	//mdelay(10);
#if 0 //后续调试可能用
       writel(0x3, 0x32880038);
       mdelay(10);
       writeb(0x3, 0x3280018d);
       mdelay(10);
       writel(0x3, 0x328c0038);
       mdelay(10);
       writeb(0x3, 0x3284018d);
       mdelay(10);

	p_printf("read 0x80038: 0x%x\n",readl(0x32880038));
	p_printf("read 0x0018d: 0x%x\n",readb(0x3280018d));
	p_printf("read 0x0018e: 0x%x\n",readb(0x3280018e));
	p_printf("read 0x0018f: 0x%x\n",readb(0x3280018f));
	p_printf("read 0x00190: 0x%x\n",readb(0x32800190));
#endif

	pwr_s3_setup();

	asm volatile ("dsb sy");
	wfi();
	wfi();

	while(1);

}

void register_pfdi(void)
{
	pfdi_vectors_t pfdi_svc;

	struct arm_smccc_res res;
	memset(&pfdi_svc, 0, sizeof(pfdi_vectors_t));

	pfdi_svc.system_off_entry = (u_register_t)e2000_system_off_entry;
	pfdi_svc.system_reset_entry = (u_register_t)e2000_system_reset_entry;
	pfdi_svc.suspend_finish_entry = (u_register_t)e2000_suspend_finish_entry;
	pfdi_svc.suspend_end_entry = (u_register_t)e2000_suspend_end_entry;
	pfdi_svc.suspend_start_entry = (u_register_t)e2000_suspend_start_entry;

	printf("system_off_entry addr =0x%lx\n", pfdi_svc.system_off_entry);
	printf("system_reset_entry addr =0x%lx\n", pfdi_svc.system_reset_entry);
	printf("suspend_entry addr =0x%lx\n", pfdi_svc.suspend_start_entry);
	printf("suspend_end_entry addr =0x%lx\n", pfdi_svc.suspend_end_entry);
	printf("suspend_finish_entry addr =0x%lx\n", pfdi_svc.suspend_finish_entry);
	dsb();
	isb();
	flush_dcache_range((unsigned long)&pfdi_svc, sizeof(pfdi_vectors_t));
	arm_smccc_smc(PFDI_REGISTER, (uint64_t)&pfdi_svc, 0, 0, 0, 0, 0, 0, &res);
}
