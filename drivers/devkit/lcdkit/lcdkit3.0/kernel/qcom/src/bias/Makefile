obj-$(CONFIG_LCD_KIT_QCOM) 	+= tps65132.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= rt4801h.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= icn68116.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= isl98608.o
obj-$(CONFIG_LCD_KIT_QCOM) 	+= qpnp_lcdb_regulator.o

EXTRA_CFLAGS +=-I$(srctree)/drivers/staging/android \
		-I$(srctree)/drivers/atfdriver \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0 \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/common/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/include \
		-I$(srctree)/drivers/devkit/lcdkit/lcdkit3.0/kernel/qcom/adapt \
		-I$(srctree)/include/drm \
		-I$(srctree)/drivers/gpu/drm \
		-I$(srctree)/techpack/display/msm \
		-I$(srctree)/techpack/display/msm/dsi \
		-I$(srctree)/techpack/display/include \
		-I$(srctree)/techpack/display/include/uapi/display \
		-I$(srctree)/drivers/clk/qcom
