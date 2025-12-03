/*
 * =====================================================================================
 *
 *       Filename:  ft_fdt_update.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2018年05月21日 16时35分13秒
 *       Revision:  none
 *
 *         Author:  lixinde, lixindei@phytium.com.cn
 *        Company:  Phytium
 *        License:  Dual BSD/GPL
 *
 * =====================================================================================
*/
#include <common.h>
#include <malloc.h>
#include "../cpu.h"
#include <linux/ctype.h>
#include <linux/libfdt.h>
#include <mapmem.h>

#include "ft_fdt_lib.h"

#define SCRATCHPAD	1024		/* bytes of scratchpad memory */

static void *fdt_addr;

//返回1表示未找到节点，已近删除完所有节点(cpu@n memory@n)
int fdt_remove_node(const char * name)
{
	int  nodeoffset;	/* node offset from libfdt */
	int  err;

	err = fdt_check_header(fdt_addr);
	if (err < 0) {
		printf("fdt_board_rm _node: %s\n", fdt_strerror(err));
		return err;
	}
	/*
	 * Get the path.  The root node is an oddball, the offset
	 * is zero and has no name.
	 */
	nodeoffset = fdt_path_offset (fdt_addr, name);
	if (nodeoffset < 0) {
		return 1;
	}
	/*
	 * Do the delete.  A fourth parameter means delete a property,
	 * otherwise delete the node.
	 */
	err = fdt_del_node(fdt_addr, nodeoffset);
	if (err < 0) {
		return err;
	}
	return 0;
}

/*在根节点下新建节点，输入节点名，返回节点偏移，重复节点名报错,放置节点位置以后修改*/
int fdt_add_node(const char * name)
{
	int   nodeoffset;
	int   err;

	err = fdt_check_header(fdt_addr);
	if (err < 0) {
		printf("fdt_board_add_node: %s\n", fdt_strerror(err));
		return err;
	}
	nodeoffset = fdt_subnode_offset(fdt_addr, 0,  name);

	/* create node. */
	if (nodeoffset == -FDT_ERR_NOTFOUND)
		nodeoffset = fdt_add_subnode(fdt_addr, 0, name);
	else if (nodeoffset < 0){
		printf("%s: %s: %s\n", __func__, name, fdt_strerror(nodeoffset));
		return nodeoffset;
	}
	else{
		printf("add node faile , the same node name \n");
		return (-6);
	}

	return nodeoffset;
}
/****************************************************************************/

/*
 * Parse the user's input, partially heuristic.  Valid formats:
 * <0x00112233 4 05>	- an array of cells.  Numbers follow standard
 *			C conventions.
 * [00 11 22 .. nn] - byte stream
 * "string"	- If the the value doesn't start with "<" or "[", it is
 *			treated as a string.  Note that the quotes are
 *			stripped by the parser before we get the string.
 * newval: An array of strings containing the new property as specified
 *	on the command line
 * count: The number of strings in the array
 * data: A bytestream to be placed in the property
 * len: The length of the resulting bytestream
 */
static int fdt_parse_prop(char * const *newval, int count, char *data, int *len)
{
	char *cp;		/* temporary char pointer */
	char *newp;		/* temporary newval char pointer */
	unsigned long tmp;	/* holds converted values */
	int stridx = 0;

	*len = 0;
	newp = newval[0];

	/* An array of cells */
	if (*newp == '<') {
		newp++;
		while ((*newp != '>') && (stridx < count)) {
			/*
			 * Keep searching until we find that last ">"
			 * That way users don't have to escape the spaces
			 */
			if (*newp == '\0') {
				newp = newval[++stridx];
				continue;
			}

			cp = newp;
			tmp = simple_strtoul(cp, &newp, 0);
			if (*cp != '?')
				*(fdt32_t *)data = cpu_to_fdt32(tmp);
			else
				newp++;

			data  += 4;
			*len += 4;

			/* If the ptr didn't advance, something went wrong */
			if ((newp - cp) <= 0) {
				printf("Sorry, I could not convert \"%s\"\n",
					cp);
				return 1;
			}

			while (*newp == ' ')
				newp++;
		}

		if (*newp != '>') {
			printf("Unexpected character '%c'\n", *newp);
			return 1;
		}
	} else if (*newp == '[') {
		/*
		 * Byte stream.  Convert the values.
		 */
		newp++;
		while ((stridx < count) && (*newp != ']')) {
			while (*newp == ' ')
				newp++;
			if (*newp == '\0') {
				newp = newval[++stridx];
				continue;
			}
			if (!isxdigit(*newp))
				break;
			tmp = simple_strtoul(newp, &newp, 16);
			*data++ = tmp & 0xFF;
			*len    = *len + 1;
		}
		if (*newp != ']') {
			printf("Unexpected character '%c'\n", *newp);
			return 1;
		}
	} else {
		/*
		 * Assume it is one or more strings.  Copy it into our
		 * data area for convenience (including the
		 * terminating '\0's).
		 */
		while (stridx < count) {
			size_t length = strlen(newp) + 1;
			strcpy(data, newp);
			data += length;
			*len += length;
			newp = newval[++stridx];
		}
	}
	return 0;
}
int fdt_get_offset(const char * name)
{

	int  nodeoffset;
	nodeoffset = fdt_path_offset (fdt_addr, name);
	if(nodeoffset < 0)
		printf("fdt : can not find %s  node\n", name);
	return	nodeoffset;
}

/*新增、修改属性 value输入带格式字符串*/
int fdt_set_prop(int offset, const char * name, const void * value)
{
	int nodeoffset = offset;	/* node offset from libfdt */
	int  len;		/* new length of the property */
	int err;
	static char data[SCRATCHPAD];	/* storage for the property */
	/*预留多个字符串填写，主要暂时匹配 fdt_parse_prop函数*/
	char * const *c_temp = (char * const *)&value;

	if (strlen(*c_temp) > SCRATCHPAD) {
		printf("prop (%ld) doesn't fit in scratchpad!\n",
			   strlen(*c_temp));
		return 1;
	}
	err = fdt_parse_prop(c_temp, 1, data, &len);
	if (err != 0)
		return err;

	err = fdt_setprop(fdt_addr, nodeoffset, name, data, len);
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n",
		       name, fdt_strerror(err));
		return err;
	}

	return 0;
}
/*新增、修改属性 value输入大端格式的数值*/
int fdt_add_prop(int offset, const char * name, const void * value, int len)
{
	int nodeoffset = offset;
	int err;
	err = fdt_setprop(fdt_addr, nodeoffset, name, value, len);
	if (err < 0) {
		printf("WARNING: could not set %s %s.\n",
		       name, fdt_strerror(err));
		return err;
	}

	return 0;
}

int ft_fdt_pack_word(void *buf, u32 value)
{
    char *p = buf;

    *(fdt32_t *)p = cpu_to_fdt32(value);
    p += 4 ;

    return p - (char *)buf;
}

int fdt_get_addr_cell(int offset)
{
	return fdt_address_cells(fdt_addr, offset);
}

int fdt_get_size_cell(int offset)
{
	return fdt_size_cells(fdt_addr, offset);
}

int set_fdt_addr(void *base)
{
	int err;
	fdt_addr = map_sysmem((phys_addr_t)base, 0);

	printf("fdt_addr %p\n", fdt_addr);
	err = fdt_check_header(fdt_addr);
	if (err < 0) {
		printf("can not find fdt, error : %s\n", fdt_strerror(err));
		return err;
	}
	return 0;
}

int ft_fixup_mem(u64 mem00_size, u64 mem01_size)
{
	const char *path;
	int offs;
	int ret;
	u32 temp[4];

	printf("fdt_addr %p\n", fdt_addr);

	/*
	 *
	*/
	path = "/memory@00";

	offs = fdt_path_offset(fdt_addr, path);
	if (offs < 0) {
		debug("Node %s not found.\n", path);
		return 0;
	}

	temp[0] = cpu_to_fdt32(0x0);
	temp[1] = cpu_to_fdt32(0x80000000);

	temp[2] = cpu_to_fdt32((mem00_size >> 32) & 0xffffffff);
	temp[3] = cpu_to_fdt32(mem00_size & 0xffffffff);

	ret = fdt_setprop(fdt_addr, offs, "reg", temp, 4 * sizeof(u32));

	if (ret < 0) {
		printf("Could not add reg property to node %s: %s\n",
			   path, fdt_strerror(ret));
		return ret;
	}

	/*
	 *
	 */
	path = "/memory@01";

	offs = fdt_path_offset(fdt_addr, path);
	if (offs < 0) {
		debug("Node %s not found.\n", path);
		return 0;
	}

	temp[0] = cpu_to_fdt32(0x20);
	temp[1] = cpu_to_fdt32(0x0);

	temp[2] = cpu_to_fdt32((mem01_size >> 32) & 0xffffffff);
	temp[3] = cpu_to_fdt32(mem01_size & 0xffffffff);

	ret = fdt_setprop(fdt_addr, offs, "reg", temp, 4 * sizeof(u32));

	if (ret < 0) {
		printf("Could not add reg property to node %s: %s\n",
		       path, fdt_strerror(ret));
		return ret;
	}


	return 0;

}

/****************************************/
static inline u64 get_unaligned_be64(const void *p)
{
	return be64_to_cpup((__be64 *)p);
}
/*
 * fdt_pack_reg - pack address and size array into the "reg"-suitable stream
 */
static int fdt_pack_reg(const void *fdt, void *buf, u64 address, u64 size)
{
	int address_cells = fdt_address_cells(fdt, 0);
	int size_cells = fdt_size_cells(fdt, 0);
	char *p = buf;

	if (address_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(address);
	else
		*(fdt32_t *)p = cpu_to_fdt32(address);
	p += 4 * address_cells;

	if (size_cells == 2)
		*(fdt64_t *)p = cpu_to_fdt64(size);
	else
		*(fdt32_t *)p = cpu_to_fdt32(size);
	p += 4 * size_cells;

	return (p - (char *)buf);
}

int ft_fdt_set_mem_prop(int offset, const char * name, uint64_t start, uint64_t size)
{
	int len, len1;
	const fdt64_t *val;
	int nodeoffset = offset;
	u8 temp[16]; /* Up to 64-bit address + 64-bit size */
	int err;

	if(offset < 0){
		//设备树中没有第二段,添加第二段
		printf("fdt : add node memory@01\n");
		nodeoffset = fdt_add_node("memory@01");

		len = ft_fdt_pack_word(temp, 0);
		err = fdt_add_prop(nodeoffset, "numa-node-id", temp, len);

		len = fdt_pack_reg(fdt_addr, temp, start, size);
		err = fdt_add_prop(nodeoffset, "reg", temp, len);

		err = fdt_add_prop(nodeoffset, "device_type", "memory", strlen("memory")+1);
	}else{

		//获取之前设备树描述的内存大小
		val = fdt_getprop(fdt_addr, nodeoffset, name, &len);
		if(val < 0){
			printf("WARNING: could not get %s prop!\n", name);
			return -1;
		}
	//	printf("addr: 0x%llx\n", get_unaligned_be64(&val[0]));
	//	printf("size: 0x%llx\n", get_unaligned_be64(&val[1]));

		//修改
	//	len1 = fdt_pack_reg(fdt_addr, temp, get_unaligned_be64(&val[0]), size);	//只修改size
		len1 = fdt_pack_reg(fdt_addr, temp, start, size);						//start , size 都修改
		err = fdt_setprop(fdt_addr, nodeoffset, "reg", temp, len1);
		if(err < 0){
			printf("WARNING: could not set %s %s.\n", "reg", fdt_strerror(err));
			return err;
		}

	}

	return 0;
}
/****************************************/

