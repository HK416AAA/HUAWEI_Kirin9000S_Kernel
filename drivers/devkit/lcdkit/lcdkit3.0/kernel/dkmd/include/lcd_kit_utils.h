/*
 * lcd_kit_utils.h
 *
 * lcdkit utils function for lcd driver head file
 *
 * Copyright (c) 2018-2022 Huawei Technologies Co., Ltd.
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

#ifndef __LCD_KIT_UTILS_H_
#define __LCD_KIT_UTILS_H_
#include <linux/kernel.h>
#include "lcd_kit_common.h"
#include "dkmd_lcd_interface.h"
#include "dkmd_dfr_info.h"
#include "dkmd_dfr_interface.h"
#include <dsm/dsm_pub.h>
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>

#define LCD_KIT_DUAL_MIPI_VIDEO BIT(5)
#define LCD_KIT_DUAL_MIPI_CMD BIT(6)
#define LONG_PRESS_RST_CONFIG_BIT 0x06
#define LONG_PRESS_RST_CONFIG1 0
#define LONG_PRESS_RST_CONFIG2 2
#define LONG_PRESS_RST_CONFIG3 4
#define LONG_PRESS_RST_CONFIG4 6
#define LONG_PRESS_8S_LCD_TIMER_LEN 1700
#define LONG_PRESS_10S_LCD_TIMER_LEN 3700
#define LONG_PRESS_30S_LCD_TIMER_LEN 23700
#define LONG_PRESS_60S_LCD_TIMER_LEN 53700
/* dsi index */
#define DSI0_INDEX BIT(0)
#define DSI1_INDEX BIT(1)
#define DSI2_INDEX BIT(2)
#define DSI_MAX    3
#define DETECT_NUM 4
#define VAL_NUM    2
#define DETECT_LOOPS   6
#define LCD_KIT_DEFAULT_PANEL  "auo_otm1901a_5p2_1080p_video_default"

#define POWERIC_DEFAULT_GPIO 0
#define POWERIC_DEFAULT_GPIO_VAL 0
#define SECONDARY_PANEL_CMD_TYPE 0x1000

/* mipi error detect */
#define MIPI_DETECT_NUM       4
#define MIPI_DETECT_LOOP      30
#define MIPI_DETECT_START     0
#define MIPI_DETECT_BASE      1
#define MIPI_READ_COUNT       5
#define MIPI_DETECT_DELAY     2
#define DMD_DET_ERR_LEN      600

/* mbist detect */
#define MBIST_LOOPS        15

/* pcd errflag detect */
#define PCD_ERRFLAG_SUCCESS       0
#define PCD_FAIL                  1
#define ERRFLAG_FAIL              2
#define LCD_KIT_PCD_SIZE          3
#define LCD_KIT_ERRFLAG_SIZE      8
#define DMD_ERR_INFO_LEN         50
#define LCD_KIT_PCD_DETECT_OPEN   1
#define LCD_KIT_PCD_DETECT_CLOSE  0
#define MAX_MIPI_ERR_READ_COUNT   10

#define PANEL_DIEID_DATA_SIZE_MAX 500
#define LCD_KIT_ESD_TIME 500

#define SN_CODE_23_NUMB_SIZE 23
#define SN_CODE_2_NONE_SIZE 2

#define SN_CODE_READ_TIMES 6
#define SN_CODE_FRONT_BIT 4

enum {
	LCD_OFFLINE = 0,
	LCD_ONLINE = 1,
};

enum {
	LCD_KIT_UNEQUAL = 0,
	LCD_KIT_EQUAL = 1,
	LCD_KIT_LESS = 2,
	LCD_KIT_MORE = 3,
};

enum {
	PCD_COMPARE_MODE_EQUAL = 0,
	PCD_COMPARE_MODE_BIGGER = 1,
	PCD_COMPARE_MODE_MASK = 2,
};

enum {
	DIMMING_SEQ_120HZ_INDEX = 0,
	DIMMING_SEQ_90HZ_INDEX = 1,
	DIMMING_SEQ_60HZ_INDEX = 2,
};

enum {
	PWR_OFF_NO_NEED_DISABLE_ESD = 0,
	PWR_OFF_NEED_DISABLE_ESD = 1,
	PWR_OFF_DISABLED_ESD = 2,
	PWR_OFF_ESD_STATUS_BUTT,
};

struct lcd_kit_quickly_sleep_out {
	u32 support;
	u32 panel_on_tag;
	u32 interval;
	struct timespec64 panel_on_record_tv;
};

struct lcd_kit_2d_barcode {
	u32 support;
	u32 block_num;
	struct lcd_kit_dsi_panel_cmds cmds;
	struct lcd_kit_dsi_panel_cmds sn_cmds[SN_CODE_READ_TIMES];
};

struct lcd_kit_oem_info {
	u32 support;
	/* 2d barcode */
	struct lcd_kit_2d_barcode barcode_2d;
	u32 multiple_reads;
};

struct lcd_kit_panel_dieid_info {
	u32 support;
	struct lcd_kit_dsi_panel_cmds cmds;
	char dieid_info_data[PANEL_DIEID_DATA_SIZE_MAX];
};

#define FPS_144 144
#define FPS_120 120
#define FPS_90  90
#define FPS_60  60
#define FPS_45  45
#define FPS_40  40
#define FPS_30  30
#define FPS_24  24
#define FPS_20  20
#define FPS_18  18
#define FPS_15  15
#define FPS_10  10
#define FPS_1   1

struct fps_cmd_table {
	int val;
	struct lcd_kit_dsi_panel_cmds *cmds;
};

enum {
	PRIMARY_PANEL_AOD_BIT = 8,
	SECONDARY_PANEL_AOD_BIT = 9,
};

int lcd_kit_mipi_set_backlight(int panel_id, unsigned int level);
int lcd_kit_utils_init(int panel_id, struct device_node *np, struct dpu_panel_info *pinfo);
void lcd_kit_set_mipi_link(int panel_id, int link_state);
void lcd_kit_disp_on_record_time(int panel_id);
void lcd_kit_disp_on_check_delay(int panel_id);
bool lcd_kit_get_panel_on_state(int panel_id);
void lcd_kit_recovery_display(int panel_id);
int lcd_kit_check_pcd_errflag_check(void);
int lcd_kit_gpio_pcd_errflag_check(void);
bool is_dual_mipi_panel(unsigned int panel_type);
int lcd_kit_get_dsi_index(unsigned int *dsi_index,
	unsigned int connector_id);
bool lcd_kit_support(void);
void check_panel_on_state(void);
void lcd_kit_get_bl_cmds(int panel_id, int bl_level, struct dsi_cmds *dsi_cmds);
void lcd_kit_ddic_lv_detect_dmd_report(u8 reg_val[DETECT_LOOPS][DETECT_NUM][VAL_NUM]);
bool lcd_kit_get_skip_power_on_off(void);
void lcd_kit_set_skip_power_on_off(bool status);
void lcd_kit_mipi_error_dmd_report(u8 reg_val[MIPI_DETECT_LOOP][MIPI_DETECT_NUM][VAL_NUM]);
void lcd_kit_check_sum2_dmd_report(u8 reg_val[MIPI_DETECT_NUM][VAL_NUM]);
void lcd_kit_mbist_detect_dmd_report(u8 reg_val[MBIST_LOOPS][DETECT_NUM][VAL_NUM]);
int lcd_kit_panel_version_init(int panel_id);
void lcd_kit_parse_spr_info(int panel_id, struct device_node *np, struct dpu_panel_info *pinfo);
int lcd_kit_get_power_status(int panel_id);
void  lcd_kit_clean_aod_panel_state(int panel_id);
int  lcd_kit_get_aod_panel_state(int panel_id);
void lcd_esd_enable(int panel_id, unsigned int enable);
void lcd_kit_check_mipi_error(int panel_id);
int lcd_kit_parse_switch_cmd(int panel_id, const char *command);
int get_sn_code_multiple_reads(int panel_id, unsigned char *out);

#ifdef LCD_KIT_DEBUG_ENABLE
int lcd_kit_debug_init(void);
#endif
void lcd_kit_sn_reprocess(uint32_t panel_id, char *buff, unsigned int len);
#endif
