# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2022, Phytium Technology Co., Ltd.
#
obj-y += e2000.o

obj-y += board_init/pll_init.o
obj-y += board_init/ddr_init.o
obj-y += board_init/pcie_init.o
obj-y += board_init/phytium_init.o
obj-y += board_init/ealry_serial.o
obj-y += board_init/ft_fdt_update.o
obj-y += board_init/ft_fdt_lib.o
obj-y += board_init/e2000_lbc.o
obj-y += board_init/e2000_nand.o
obj-$(CONFIG_SCMI_PHYTIUM) += board_init/scmi.o

obj-y += parameter/parameter.o
obj-y += parameter/board_parameter.o
obj-y += parameter/do_parameter.o

obj-y += pfdi/pfdi_entry_test.o
obj-y += pfdi/pfdi_service_test.o

obj-y += power_manage/power_manage.o
obj-y += power_manage/cpld.o
