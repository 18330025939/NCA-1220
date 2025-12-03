#include <common.h>
#include <asm/io.h>
#include "phytium_qspi.h"

#define PAGE_SIZE						 256
#define SECTOR_SIZE						 (1024 * 4)
#define BLOCK_SIZE_32K					 (1024 * 32)
#define BLOCK_SIZE_64K					 (1024 * 64)
#define SPI_CFG_CMD_READ_ID              (0x90<<24)
#define SPI_CFG_CMD_RDID                 (0x9F<<24)
#define SPI_CFG_CMD_RDSR                 (0x05<<24)
#define SPI_CFG_CMD_SE_64KB              (0xD8<<24)
#define SPI_CFG_CMD_WREN                 (0x06<<24)
#define SPI_CFG_CMD_READ                 (0x03<<24)
#define SPI_CFG_FLASH_SEL_0              (0x0<<19)
#define SPI_CFG_ADDR_TRANSFER            (0x1<<15)  /* cmd_addr, 1: , 0:    */

#define SPI_CFG_DUMMY_SEND               (0x1<<14)  /* 1:send, 表示读数据有延迟        , 0:not send */
#define SPI_CFG_DUMMY_CYCLE              (0x1<<7)   /* 当 latency=1 表示读数据有延迟, 延迟的周期数 */

#define SPI_CFG_DATA_TRANSFER			 (0x1<<13)	/* 1:send, data_transfer , 0:not data_transfer */
#define SPI_CFG_ADDR_CNT_3               (0x0<<12)  /* address_cnt = 3 */
#define SPI_CFG_ADDR_CNT_4               (0x1<<12)  /* address_cnt = 4 */

#define SPI_CFG_RW_NUM_1                 (0x0<<3)
#define SPI_CFG_RW_NUM_2                 (0x1<<3)
#define SPI_CFG_RW_NUM_3                 (0x2<<3)
#define SPI_CFG_RW_NUM_4                 (0x3<<3)
#define SPI_CFG_RW_NUM_8                 (0x7<<3)

#define SPI_ACCESS_CMD_MODE_MASK         (0x3<<6)
#define SPI_ACCESS_ADDR_MODE_MASK        (0x1<<5)
#define PHYTIUM_QSPI_1_1_1					0
#define PHYTIUM_QSPI_1_1_2					1
#define PHYTIUM_QSPI_1_1_4					2
#define PHYTIUM_QSPI_1_2_2					3
#define PHYTIUM_QSPI_1_4_4				(4<<16)
#define PHYTIUM_QSPI_2_2_2					5
#define PHYTIUM_QSPI_4_4_4				(6<<16)
#define PHYTIUM_CMD_SCK_SEL					0x07

#define spi_cmd_read_id()      SPI_CFG_FLASH_SEL_0  \
							  | SPI_CFG_CMD_READ_ID   \
                              | SPI_CFG_ADDR_TRANSFER     \
                              | SPI_CFG_ADDR_CNT_3    \
                              | SPI_CFG_DATA_TRANSFER		\
                              | SPI_CFG_RW_NUM_4;
//0x9F002040
#define spi_cmd_rdid()                SPI_CFG_FLASH_SEL_0	\
									| SPI_CFG_CMD_RDID		\
									| SPI_CFG_DATA_TRANSFER		\
									| SPI_CFG_RW_NUM_4;
#define spi_cmd_rdsr1()          SPI_CFG_CMD_RDSR	  \
								| PHYTIUM_QSPI_4_4_4	\
								| SPI_CFG_DATA_TRANSFER	  \
								| SPI_CFG_RW_NUM_1;
#define spi_write_addr(addr)                           \
      *(u32 *)SPI_ADDR_PORT_ADDR = addr;

#define spi_read_h_data(value)                         \
      value = *(u32 *)SPI_HDATA_PORT_ADDR;

#define spi_read_l_data(value)                         \
      value = *(u32 *)SPI_LDATA_PORT_ADDR;

#define CMD_WRITE_ENABLE		0x06
#define CMD_WRITE_DISABLE		0x04
#define CMD_SECTOR_ERASE		0x20
#define CMD_BLOCK_ERASE_32K		0x52
#define CMD_BLOCK_ERASE_64K		0xD8
#define CMD_CHIP_EARSE			0x60
#define CMD_READ_ID				0x90
#define CMD_PAGE_PROGRAM		0x02

typedef struct qspi_reg
{
	uint32_t reg_flash_cap;
	uint32_t reg_rd_cfg;
	uint32_t reg_wr_cfg;
	uint32_t reg_flush_reg;
	uint32_t reg_cmd_port;
	uint32_t reg_addr_port;
	uint32_t reg_hd_port;
	uint32_t reg_ld_port;
	uint32_t reg_fun_set;
	uint32_t reg_err_log;
	uint32_t reg_wp_reg;
	uint32_t reg_mode_reg;
}qspi_reg_t;

qspi_reg_t *qspi_reg_info = (void *)0x28008000;

//block - 64k
int qspi_erase_block(uint64_t block_address)
{
	int ret = 0;

	writel(0x400000 | (CMD_WRITE_ENABLE << 24), &qspi_reg_info->reg_cmd_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);
	isb();
	dsb();

	writel(0x408000 | (CMD_BLOCK_ERASE_64K << 24), &qspi_reg_info->reg_cmd_port);
	writel(block_address, &qspi_reg_info->reg_addr_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);
	isb();
	dsb();

	return ret;
}

void qspinor_erase(uint64_t addr, uint32_t len)
{
	if(len < BLOCK_SIZE_64K){
		printf("error: len cannot be less than 64K!\n");
	}else{
		while (len)
		{
			qspi_erase_block(addr);
			addr += BLOCK_SIZE_64K;
			len -= BLOCK_SIZE_64K;
		}
	}
}

int qspi_write_word(uint64_t address, uint32_t value)
{
	uint32_t ret = 0;

	if (value == 0xffffffff)
	return -1;

	if(address % 4 != 0) {
		printf( "Address not aligne 4 byte \n");
		return -1;
	}

	writel(0x400000 | (CMD_WRITE_ENABLE << 24), &qspi_reg_info->reg_cmd_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);
	isb();
	dsb();

	writel(0x000208 | (CMD_PAGE_PROGRAM << 24), &qspi_reg_info->reg_wr_cfg);
	writel(value, address);
	isb();
	dsb();

	writel(0x1, &qspi_reg_info->reg_flush_reg);
	writel(0x0, &qspi_reg_info->reg_wr_cfg);

	return ret;
}

/*
 * function:
 * input   :
 * output  :
 * used    :
 * remark  :
 * version :
 * notice  :  pp_num must less than 64, waveform of pp is 256 byte;
 *            correctness is not sure
 *            flush_buffer will be flushed, so data to be programed will be lost
 */
void qspi_page_program_flush(uint64_t src_addr, uint64_t page_addr, uint32_t pp_num)
{
	uint32_t i = 0;
	uint32_t const *p = (void const*)src_addr;

	writel(0x400000 | ( CMD_WRITE_ENABLE << 24), &qspi_reg_info->reg_cmd_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);
	isb();
	dsb();

	writel(0x000208 | (CMD_PAGE_PROGRAM << 24), &qspi_reg_info->reg_wr_cfg);
	isb();
	dsb();
	for(i = 0; i < PAGE_SIZE/4; i++){
		*(volatile uint32_t*)page_addr = p[i];
	}
	isb();
	dsb();

	writel(0x1, &qspi_reg_info->reg_flush_reg);
	writel(0x0, &qspi_reg_info->reg_wr_cfg);
}

/*
 * function:
 * input   :
 * output  :
 * used    :  must write in 32bits
 * remark  :
 * version :
 */
int qspi_page_program(uint64_t src_addr, uint64_t page_addr, uint32_t len)
{
	uint32_t i = 0, page_num = 0;

	if(len <= PAGE_SIZE){
		page_num = 1;
	}else {
		if((len % PAGE_SIZE)>0)
			page_num = len/PAGE_SIZE + 1;
		else
			page_num = len/PAGE_SIZE;
	}

	for(i = 0; i < page_num; i++)
		qspi_page_program_flush(src_addr + (i*PAGE_SIZE), page_addr + (i*PAGE_SIZE), PAGE_SIZE);

	return 0;
}

/* Read Manufacturer and Device ID (READ_ID: 90h) */
u16 qspi_read_id(void)
{
    u16 id_data=0;
    u8 data = 0;

	writel(spi_cmd_read_id(), &qspi_reg_info->reg_cmd_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);

	writel(0x100000, &qspi_reg_info->reg_addr_port);
	isb();
	dsb();

    data = readl(&qspi_reg_info->reg_hd_port);
	printf("reg_hd_port = %x\n", data);

	id_data = (u16)(data<<16);
    data = readl( &qspi_reg_info->reg_ld_port);
	printf("reg_ld_port = %x\n", data);

	id_data |= data;

    return id_data;
}

/* Read Identification (RDID: 9Fh) */
u16 spi_rdid(void)
{
    u16 id_data=0;

	writel(spi_cmd_rdid(), &qspi_reg_info->reg_cmd_port);
	isb();
	dsb();

	printf("(RDID: 9Fh) reg_ld_port = %x\n", readl(&qspi_reg_info->reg_ld_port));
	printf("(RDID: 9Fh) reg_hd_port = %x\n", readl(&qspi_reg_info->reg_hd_port));

    id_data = readl( &qspi_reg_info->reg_ld_port);

	return id_data;
}

u16 spi_rdsr1(void)
{
    u16 id_data=0;

	writel(spi_cmd_rdsr1(), &qspi_reg_info->reg_cmd_port);
	writel(0x1, &qspi_reg_info->reg_ld_port);
	isb();
	dsb();

	printf("(RDSR1: 05h) reg_hd_port = %x\n", readl(&qspi_reg_info->reg_hd_port));
	printf("(RDSR1: 05h) reg_ld_port = %x\n", readl(&qspi_reg_info->reg_ld_port));

    id_data = readl( &qspi_reg_info->reg_ld_port);

    return id_data;
}

int qspinor_read_sr1(void)
{
	writel(					0x5 << 24
						|	1 << 21						\
						|	PHYTIUM_QSPI_1_1_1 << 16 	\
						|	1 << 13						\
						|	0 << 3						\
						|	0x5,
	&qspi_reg_info->reg_cmd_port);

	return readl(&qspi_reg_info->reg_ld_port);
}
