# Makefile for the irda drivers.
#

# Each configuration option enables a list of files.
CFLAGS_huawei_ts_kit_core.o := -I$(src)
CFLAGS_huawei_ts_kit_api.o := -I$(src)

ifeq ($(CONFIG_HUAWEI_DEVKIT_HISI_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o hisi_adapter/tpkit_platform_adapter.o huawei_ts_kit_misc.o
EXTRA_CFLAGS += -I$(srctree)/drivers/hisi/tzdriver
EXTRA_CFLAGS += -I$(srctree)/drivers/tzdriver
EXTRA_CFLAGS += -I$(srctree)/drivers/media/huawei/camera/pmic
endif

ifeq ($(CONFIG_HUAWEI_DEVKIT_QCOM_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o huawei_ts_kit_misc.o qcom_adapter/tpkit_cmdline_parse.o qcom_adapter/tpkit_platform_adapter.o
EXTRA_CFLAGS += -Itechpack/display
EXTRA_CFLAGS += -Itechpack/display/msm/dsi
endif

ccflags-y += -I$(srctree)/include/oeminfo
ifeq ($(CONFIG_HUAWEI_DEVKIT_MTK_3_0),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0) += tpkits.o
tpkits-objs += huawei_ts_kit_core.o huawei_ts_kit_fsnode.o huawei_ts_kit_api.o huawei_ts_kit_algo.o huawei_ts_kit_csvparse.o huawei_ts_kit_misc.o mtk_adapter/tpkit_platform_adapter.o
EXTRA_CFLAGS += -Idrivers/tzdriver
endif

obj-$(CONFIG_HUAWEI_TS_KIT_COMMON_3_0) += huawei_tp_common.o
obj-$(CONFIG_HUAWEI_TP_KIT_COLOR_INFO_3_0) += tp_color.o
ifeq ($(filter taro, $(TARGET_BOARD_PLATFORM)),)
ifneq ($(CONFIG_TS_KIT_NO_NEED_THIRDPARTY),y)
obj-$(CONFIG_HUAWEI_TS_KIT_3_0)  += panel/
endif
endif
EXTRA_CFLAGS += -I$(srctree)/drivers/hwspinlock
EXTRA_CFLAGS += -I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/common/include
ifeq ($(CONFIG_HUAWEI_DEVKIT_QCOM_3_0),y)
EXTRA_CFLAGS += -Idrivers/video/fbdev/hisi/dss
ccflags-y += -Idrivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1
else
EXTRA_CFLAGS += -I$(srctree)/drivers/video/fbdev/dkmd/dpu/azalea
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sensors-1.0/sensorHub/inc_v1
endif

ifeq ($(CONFIG_HUAWEI_TS_KIT_TEST_3_0),y)
EXTRA_CFLAGS += -DHUAWEI_TS_KIT_TEST
endif
