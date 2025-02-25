/*
 * lcd_kit_sysfs.h
 *
 * lcdkit sysfs function for lcdkit head file
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LCD_KIT_FNODE_H_
#define __LCD_KIT_FNODE_H_
#include "lcd_kit_common.h"

#define LCD_KIT_SYSFS_MAX 101

enum lcd_kit_sysfs_index {
	LCD_MODEL_INDEX,
	LCD_TYPE_INDEX,
	PANEL_INFO_INDEX,
	VOLTAGE_ENABLE_INDEX,
	ACL_INDEX,
	VR_INDEX,
	SUPPORT_MODE_INDEX,
	GAMMA_DYNAMIC_INDEX,
	FRAME_COUNT_INDEX,
	FRAME_UPDATE_INDEX,
	MIPI_DSI_CLK_UPT_INDEX,
	FPS_SCENCE_INDEX,
	ALPM_FUNCTION_INDEX,
	ALPM_SETTING_INDEX,
	FUNC_SWITCH_INDEX,
	REG_READ_INDEX,
	DDIC_OEM_INDEX,
	BL_MODE_INDEX,
	BL_SUPPORT_MODE_INDEX,
	EFFECT_BL_INDEX,
	DDIC_LV_DETECT_INDEX,
	HBM_MODE_INDEX,
	DDIC_ALPHA_INDEX,
	FPS_ORDER_INDEX,
	PANEL_SNCODE_INDEX,
	DISPLAY_IDLE_MODE,
	PRE_CAMERA_POSITION,
	PANEL_VERSION_INDEX,
	LCD_PRODUCT_TYPE_INDEX,
	PCD_CHECK_INDEX,
	PANEL_DIEID_INDEX,
	CAMERA_PROX_INDEX,
	LCD_DIMMING_INDEX,
	SEC_LCD_MODEL_INDEX,
};
/* sysfs support enum */
enum lcd_kit_sysfs_support {
	SYSFS_NOT_SUPPORT,
	SYSFS_SUPPORT,
};

struct lcd_kit_sysfs_ops *lcd_kit_get_sysfs_ops(void);
int lcd_kit_sysfs_ops_register(struct lcd_kit_sysfs_ops *ops);
int lcd_kit_sysfs_ops_unregister(struct lcd_kit_sysfs_ops *ops);
int lcd_kit_create_sysfs(struct kobject *obj);
void lcd_kit_remove_sysfs(struct kobject *obj);

struct lcd_kit_sysfs_ops {
	int (*check_support)(int index);
	ssize_t (*model_show)(struct device *dev, struct device_attribute *attr,
		char *buf);
	ssize_t (*sec_model_show)(struct device *dev, struct device_attribute *attr,
		char *buf);
	ssize_t (*type_show)(struct device *dev, struct device_attribute *attr,
		char *buf);
	ssize_t (*panel_info_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*voltage_enable_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*amoled_acl_ctrl_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*amoled_acl_ctrl_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*amoled_vr_mode_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*amoled_vr_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*oem_info_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*oem_info_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*effect_color_mode_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*effect_color_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*reg_read_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*reg_read_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*bl_mode_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*bl_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*gamma_dynamic_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*frame_count_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*frame_update_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*frame_update_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*mipi_dsi_clk_upt_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*mipi_dsi_clk_upt_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*fps_scence_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*fps_scence_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*fps_order_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*alpm_function_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*alpm_function_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*alpm_setting_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*alpm_setting_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*func_switch_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*func_switch_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*ddic_oem_info_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*ddic_oem_info_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*support_bl_mode_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*support_bl_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*effect_bl_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*effect_bl_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*ddic_lv_detect_test_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*ddic_lv_detect_test_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*hbm_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*hbm_mode_show)(struct device *dev,
		 struct device_attribute *attr, char *buf);
	ssize_t (*ddic_local_hbm_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*panel_sncode_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*panel_sncode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*pre_camera_position_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*panel_version_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*idle_mode_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
	ssize_t (*product_type_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*pcd_check_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*panel_dieid_info_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*camera_prox_show)(struct device *dev,
		struct device_attribute *attr, char *buf);
	ssize_t (*panel_dimming_switch_store)(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
};
#endif
