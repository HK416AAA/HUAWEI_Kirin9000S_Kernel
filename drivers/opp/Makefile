# SPDX-License-Identifier: GPL-2.0-only
ccflags-$(CONFIG_DEBUG_DRIVER)	:= -DDEBUG
obj-y				+= core.o cpu.o
obj-$(CONFIG_OF)		+= of.o
obj-$(CONFIG_OPP_DEBUG_FS)	+= debugfs.o
obj-$(CONFIG_ARM_TI_CPUFREQ)	+= ti-opp-supply.o
