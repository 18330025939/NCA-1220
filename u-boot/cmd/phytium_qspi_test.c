#include <common.h>
#include <command.h>
#include <spi_flash.h>
#include "../drivers/spi/phytium_qspi.h"

static int do_flerase(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	static unsigned long start_addr = 0, length = 0;

	if(argc != 3)
		return CMD_RET_USAGE;

	start_addr = simple_strtoul(argv[1], NULL, 16);
	length = simple_strtoul(argv[2], NULL, 16);
	printf("erase qspi flash\n");

	qspinor_erase(start_addr,length);//JHB
	printf("erase done\n");

	return 0;
}

static int do_flash_write(struct cmd_tbl *cmdtp, int flag, int argc, char * const argv[])
{
	static unsigned long src_addr = 0;
	static unsigned long dst_addr = 0;
	static unsigned long length = 0;

	if(argc != 4)
		return CMD_RET_USAGE;

	src_addr = simple_strtoul(argv[1], NULL, 16);
	dst_addr = simple_strtoul(argv[2], NULL, 16);
	length = simple_strtoul(argv[3], NULL, 16);

	printf("write ...\n");
	qspi_page_program(src_addr, dst_addr, length);// old
	printf("write done\n");

	return 0;
}

U_BOOT_CMD(
	flashe,  3,   0,  do_flerase,
	"erase QSPI FLASH \n",
	"start length\n"
	"    - erase FLASH size 'length' from addr 'start'\n"
	"    - note: erase must more than one sector [64K*N]\n"
);
U_BOOT_CMD(
	flashw,  4,  0,   do_flash_write,
	"qspi flash write",
	"write [source address] [destion address] [length]"
);
