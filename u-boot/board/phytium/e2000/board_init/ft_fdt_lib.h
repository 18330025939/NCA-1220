#ifndef _FTLIBFDT_H
#define _FTLIBFDT_H

#include <linux/libfdt_env.h>

int set_fdt_addr(void *base);


int fdt_add_node(const char * name);
int fdt_remove_node(const char * name);
int fdt_get_offset(const char * name);

int fdt_set_prop(int offset, const char * name, const void * value);
int fdt_add_prop(int offset, const char * name, const void * value, int len);
int ft_fdt_pack_word(void *buf, u32 value);

int fdt_get_addr_cell(int offset);
int fdt_get_size_cell(int offset);
int ft_fixup_mem(u64 mem00_size, u64 mem01_size);
int ft_fdt_set_mem_prop(int offset, const char * name, uint64_t start, uint64_t size);

#endif
