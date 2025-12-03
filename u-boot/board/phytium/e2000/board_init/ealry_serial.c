// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2021
 * lixinde <lixinde@phytium.com.cn>
 */

#include <common.h>
#include <asm/io.h>
#include "ealry_serial.h"
#include "../cpu.h"

/*
 * We can use a combined structure for PL010 and PL011, because they overlap
 * only in common registers.
 */
struct pl01x_regs {
	u32	dr;			/* 0x00 Data register */
	u32	ecr;		/* 0x04 Error clear register (Write) */
	u32	pl010_lcrh;	/* 0x08 Line control register, high byte */
	u32	pl010_lcrm;	/* 0x0C Line control register, middle byte */
	u32	pl010_lcrl;	/* 0x10 Line control register, low byte */
	u32	pl010_cr;	/* 0x14 Control register */
	u32	fr;			/* 0x18 Flag register (Read only) */
#ifdef CONFIG_PL011_SERIAL_RLCR
	u32	pl011_rlcr;	/* 0x1c Receive line control register */
#else
	u32	reserved;
#endif
	u32	ilpr;		/* 0x20 IrDA low-power counter register */
	u32	pl011_ibrd;	/* 0x24 Integer baud rate register */
	u32	pl011_fbrd;	/* 0x28 Fractional baud rate register */
	u32	pl011_lcrh;	/* 0x2C Line control register */
	u32	pl011_cr;	/* 0x30 Control register */
};

#define UART_PL01x_RSR_OE               0x08
#define UART_PL01x_RSR_BE               0x04
#define UART_PL01x_RSR_PE               0x02
#define UART_PL01x_RSR_FE               0x01

#define UART_PL01x_FR_TXFE              0x80
#define UART_PL01x_FR_RXFF              0x40
#define UART_PL01x_FR_TXFF              0x20
#define UART_PL01x_FR_RXFE              0x10
#define UART_PL01x_FR_BUSY              0x08
#define UART_PL01x_FR_TMSK              (UART_PL01x_FR_TXFF + UART_PL01x_FR_BUSY)

/*
 *  PL010 definitions
 *
 */
#define UART_PL010_CR_LPE               (1 << 7)
#define UART_PL010_CR_RTIE              (1 << 6)
#define UART_PL010_CR_TIE               (1 << 5)
#define UART_PL010_CR_RIE               (1 << 4)
#define UART_PL010_CR_MSIE              (1 << 3)
#define UART_PL010_CR_IIRLP             (1 << 2)
#define UART_PL010_CR_SIREN             (1 << 1)
#define UART_PL010_CR_UARTEN            (1 << 0)

static struct pl01x_regs *base_regs = (struct pl01x_regs *)UART1_BASE;

static int pl01x_putc(struct pl01x_regs *regs, char c)
{
	/* Wait until there is space in the FIFO */
	if (readl(&regs->fr) & UART_PL01x_FR_TXFF)
		return -EAGAIN;

	/* Send the character */
	writel(c, &regs->dr);

	return 0;
}

static void pl01x_serial_putc(const char c)
{
	if (c == '\n')
		while (pl01x_putc(base_regs, '\r') == -EAGAIN);

	while (pl01x_putc(base_regs, c) == -EAGAIN);
}

void ealry_serial_putc(const char c)
{
	pl01x_serial_putc(c);
}

static void e_puts(const char *s)
{
	while(*s) {
		ealry_serial_putc((unsigned char)(*s++));
	}
}

#define PRINT_BUFFER_SIZE	512

int p_printf(const char *fmt, ...)
{
	va_list args;
	unsigned int i;
	char printbuffer[PRINT_BUFFER_SIZE];

	va_start(args, fmt);

	/* For this to work, printbuffer must be larger than
	 * anything we ever want to print.
	 */
	i = vsnprintf(printbuffer, sizeof(printbuffer), fmt, args);
	va_end(args);

	/* Print the string */
	e_puts(printbuffer);

	return i;
}
