/*
 * lcd_kit_factory.c
 *
 * lcdkit factory function for lcdkit
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include "lcd_kit_factory.h"
#include "lcd_kit_common.h"
#include "lcd_kit_parse.h"
#if defined(CONFIG_LCD_KIT_HISI)
#include "lcd_kit_ext_disp.h"
#endif

static char *g_sence_array[] = {
	"LCD_INCOME0",   "MMI0",   "RUNNINGTEST0", "PROJECT_MENU0",
	"LCD_INCOME1",   "MMI1",   "RUNNINGTEST1",  "PROJECT_MENU1",
	"LCD_INCOME2",   "MMI2",   "RUNNINGTEST2",  "PROJECT_MENU2",
	"LCD_INCOME3",   "MMI3",   "RUNNINGTEST3",  "PROJECT_MENU3",
	"LCD_INCOME4",   "MMI4",   "RUNNINGTEST4",  "PROJECT_MENU4",
	"LCD_INCOME5",   "MMI5",   "RUNNINGTEST5",  "PROJECT_MENU5",
	"LCD_INCOME6",   "MMI6",   "RUNNINGTEST6",  "PROJECT_MENU6",
	"LCD_INCOME7",   "MMI7",   "RUNNINGTEST7",  "PROJECT_MENU7",
	"LCD_INCOME8",   "MMI8",   "RUNNINGTEST8",  "PROJECT_MENU8",
	"LCD_INCOME9",   "MMI9",   "RUNNINGTEST9",  "PROJECT_MENU9",
	"LCD_INCOME10",  "MMI10",  "RUNNINGTEST10",  "PROJECT_MENU10",
	"LCD_INCOME11",  "MMI11",  "RUNNINGTEST11",  "PROJECT_MENU11",
	"LCD_INCOME12",  "MMI12",  "RUNNINGTEST12",  "PROJECT_MENU12",
	"LCD_INCOME13",  "MMI13",  "RUNNINGTEST13",  "PROJECT_MENU13",
	"LCD_INCOME14",  "MMI14",  "RUNNINGTEST14",  "PROJECT_MENU14",
	"LCD_INCOME15",  "MMI15",  "RUNNINGTEST15",  "PROJECT_MENU15",
	"LCD_INCOME16",  "MMI16",  "RUNNINGTEST16",  "PROJECT_MENU16",
	"LCD_INCOME17",  "MMI17",  "RUNNINGTEST17",  "PROJECT_MENU17",
	"LCD_INCOME18",  "MMI18",  "RUNNINGTEST18",  "PROJECT_MENU18",
	"LCD_INCOME19",  "MMI19",  "RUNNINGTEST19",  "PROJECT_MENU19",
	"LCD_INCOME20",  "MMI20",  "RUNNINGTEST20",  "PROJECT_MENU20",
	"LCD_INCOME21",  "MMI21",  "RUNNINGTEST21",  "PROJECT_MENU21",
	"LCD_INCOME22",  "MMI22",  "RUNNINGTEST22",  "PROJECT_MENU22",
	"LCD_INCOME23",  "MMI23",  "RUNNINGTEST23",  "PROJECT_MENU23",
	"LCD_INCOME24",  "MMI24",  "RUNNINGTEST24",  "PROJECT_MENU24",
	"LCD_INCOME25",  "MMI25",  "RUNNINGTEST25",  "PROJECT_MENU25",
	"LCD_INCOME26",  "MMI26",  "RUNNINGTEST26",  "PROJECT_MENU26",
	"LCD_INCOME27",  "MMI27",  "RUNNINGTEST27",  "PROJECT_MENU27",
	"LCD_INCOME28",  "MMI28",  "RUNNINGTEST28",  "PROJECT_MENU28",
	"LCD_INCOME29",  "MMI29",  "RUNNINGTEST29",  "PROJECT_MENU29",
	"LCD_INCOME30",  "MMI30",  "RUNNINGTEST30",  "PROJECT_MENU30",
	"LCD_INCOME31",  "MMI31",  "RUNNINGTEST31",  "PROJECT_MENU31",
	"LCD_INCOME32",  "MMI32",  "RUNNINGTEST32",  "PROJECT_MENU32",
	"LCD_INCOME33",  "MMI33",  "RUNNINGTEST33",  "PROJECT_MENU33",
	"LCD_INCOME34",  "MMI34",  "RUNNINGTEST34",  "PROJECT_MENU34",
	"LCD_INCOME35",  "MMI35",  "RUNNINGTEST35",  "PROJECT_MENU35",
	"LCD_INCOME36",  "MMI36",  "RUNNINGTEST36",  "PROJECT_MENU36",
	"LCD_INCOME37",  "MMI37",  "RUNNINGTEST37",  "PROJECT_MENU37",
	"LCD_INCOME38",  "MMI38",  "RUNNINGTEST38",  "PROJECT_MENU38",
	"LCD_INCOME39",  "MMI39",  "RUNNINGTEST39",  "PROJECT_MENU39",
	"LCD_INCOME40",  "MMI40",  "RUNNINGTEST40",  "PROJECT_MENU40",
	"LCD_INCOME41",  "MMI41",  "RUNNINGTEST41",  "PROJECT_MENU41",
	"LCD_INCOME42",  "MMI42",  "RUNNINGTEST42",  "PROJECT_MENU42",
	"CURRENT1_0",    "CURRENT1_1", "CURRENT1_2",  "CURRENT1_3",
	"CURRENT1_4",    "CURRENT1_5", "CHECKSUM1",  "CHECKSUM2",
	"CHECKSUM3",     "CHECKSUM4", "BL_OPEN_SHORT",  "PCD_ERRORFLAG",
	"DOTINVERSION",  "CHECKREG", "COLUMNINVERSION",   "POWERONOFF",
	"BLSWITCH", "CURRENT_TEST_MODE", "OVER_CURRENT_DETECTION",
	"OVER_VOLTAGE_DETECTION", "GENERAL_TEST", "VERTICAL_LINE",
	"DDIC_LV_DETECT", "AVDD_DETECT",  "ONESIDE_DETECT", "MIPI_DETECT",
};

static char *g_cmd_array[] = {
	"CURRENT1_0",      "CURRENT1_0",      "CURRENT1_0",      "CURRENT1_0",
	"CURRENT1_1",      "CURRENT1_1",      "CURRENT1_1",      "CURRENT1_1",
	"CURRENT1_2",      "CURRENT1_2",      "CURRENT1_2",      "CURRENT1_2",
	"CURRENT1_3",      "CURRENT1_3",      "CURRENT1_3",      "CURRENT1_3",
	"CURRENT1_4",      "CURRENT1_4",      "CURRENT1_4",      "CURRENT1_4",
	"CURRENT1_5",      "CURRENT1_5",      "CURRENT1_5",      "CURRENT1_5",
	"CHECKSUM1",       "CHECKSUM1",       "CHECKSUM1",       "CHECKSUM1",
	"CHECKSUM2",       "CHECKSUM2",       "CHECKSUM2",       "CHECKSUM2",
	"CHECKSUM3",       "CHECKSUM3",       "CHECKSUM3",       "CHECKSUM3",
	"CHECKSUM4",       "CHECKSUM4",       "CHECKSUM4",       "CHECKSUM4",
	"BL_OPEN_SHORT",   "BL_OPEN_SHORT",   "BL_OPEN_SHORT",   "BL_OPEN_SHORT",
	"PCD_ERRORFLAG",   "PCD_ERRORFLAG",   "PCD_ERRORFLAG",   "PCD_ERRORFLAG",
	"DOTINVERSION",    "DOTINVERSION",    "DOTINVERSION",    "DOTINVERSION",
	"CHECKREG",        "CHECKREG",        "CHECKREG",        "CHECKREG",
	"COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION", "COLUMNINVERSION",
	"POWERONOFF",      "POWERONOFF",      "POWERONOFF",      "POWERONOFF",
	"BLSWITCH",        "BLSWITCH",        "BLSWITCH",        "BLSWITCH",
	"GPU_TEST",        "GPU_TEST",        "GPU_TEST",        "GPU_TEST",
	"GENERAL_TEST",    "GENERAL_TEST",    "GENERAL_TEST",    "GENERAL_TEST",
	"VERTICAL_LINE_1",      "VERTICAL_LINE_1",      "VERTICAL_LINE_1",
	"VERTICAL_LINE_1",      "VERTICAL_LINE_2",      "VERTICAL_LINE_2",
	"VERTICAL_LINE_2",      "VERTICAL_LINE_2",      "VERTICAL_LINE_3",
	"VERTICAL_LINE_3",      "VERTICAL_LINE_3",      "VERTICAL_LINE_3",
	"VERTICAL_LINE_4",      "VERTICAL_LINE_4",      "VERTICAL_LINE_4",
	"VERTICAL_LINE_4",      "VERTICAL_LINE_5",      "VERTICAL_LINE_5",
	"VERTICAL_LINE_5",      "VERTICAL_LINE_5",      "DDIC_LV_DET_CMD_1",
	"DDIC_LV_DET_CMD_1",    "DDIC_LV_DET_CMD_1",    "DDIC_LV_DET_CMD_1",
	"DDIC_LV_DET_RESULT_1", "DDIC_LV_DET_RESULT_1", "DDIC_LV_DET_RESULT_1",
	"DDIC_LV_DET_RESULT_1", "DDIC_LV_DET_CMD_2",    "DDIC_LV_DET_CMD_2",
	"DDIC_LV_DET_CMD_2",    "DDIC_LV_DET_CMD_2",    "DDIC_LV_DET_RESULT_2",
	"DDIC_LV_DET_RESULT_2", "DDIC_LV_DET_RESULT_2", "DDIC_LV_DET_RESULT_2",
	"DDIC_LV_DET_CMD_3",    "DDIC_LV_DET_CMD_3",    "DDIC_LV_DET_CMD_3",
	"DDIC_LV_DET_CMD_3",    "DDIC_LV_DET_RESULT_3", "DDIC_LV_DET_RESULT_3",
	"DDIC_LV_DET_RESULT_3", "DDIC_LV_DET_RESULT_3", "DDIC_LV_DET_CMD_4",
	"DDIC_LV_DET_CMD_4",    "DDIC_LV_DET_CMD_4",    "DDIC_LV_DET_CMD_4",
	"DDIC_LV_DET_RESULT_4", "DDIC_LV_DET_RESULT_4", "DDIC_LV_DET_RESULT_4",
	"DDIC_LV_DET_RESULT_4", "MIPI_DET_ENTER_CMD1",  "MIPI_DET_ENTER_CMD1",
	"MIPI_DET_ENTER_CMD1",  "MIPI_DET_ENTER_CMD1",  "MIPI_DET_READ_CMD1",
	"MIPI_DET_READ_CMD1",   "MIPI_DET_READ_CMD1",   "MIPI_DET_READ_CMD1",
	"MIPI_DET_ENTER_CMD2",  "MIPI_DET_ENTER_CMD2",  "MIPI_DET_ENTER_CMD2",
	"MIPI_DET_ENTER_CMD2",  "MIPI_DET_READ_CMD2",   "MIPI_DET_READ_CMD2",
	"MIPI_DET_READ_CMD2",   "MIPI_DET_READ_CMD2",   "MIPI_DET_ENTER_CMD3",
	"MIPI_DET_ENTER_CMD3",  "MIPI_DET_ENTER_CMD3",  "MIPI_DET_ENTER_CMD3",
	"MIPI_DET_READ_CMD3",   "MIPI_DET_READ_CMD3",   "MIPI_DET_READ_CMD3",
	"MIPI_DET_READ_CMD3",   "MIPI_DET_ENTER_CMD4",  "MIPI_DET_ENTER_CMD4",
	"MIPI_DET_ENTER_CMD4",  "MIPI_DET_ENTER_CMD4",  "MIPI_DET_READ_CMD4",
	"MIPI_DET_READ_CMD4",   "MIPI_DET_READ_CMD4",   "MIPI_DET_READ_CMD4",
	"AVDD_DET_GET_RESULT",  "AVDD_DET_GET_RESULT",  "AVDD_DET_GET_RESULT",
	"AVDD_DET_GET_RESULT",  "AOD_DET_CMD_ON",       "AOD_DET_CMD_ON",
	"AOD_DET_CMD_ON",       "AOD_DET_CMD_ON",       "AOD_DET_CMD_OFF",
	"AOD_DET_CMD_OFF",      "AOD_DET_CMD_OFF",      "AOD_DET_CMD_OFF",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/ina231/ina231_0/ina231_set," \
	"/sys/class/ina231/ina231_0/ina231_value," \
	"1,9999999,1,9999999,1,99999",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/lcd_checksum",
	"/sys/class/graphics/fb0/bl_self_test",
	"/sys/class/graphics/fb0/amoled_pcd_errflag_check",
	"/sys/class/graphics/fb0/lcd_inversion_mode",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"/sys/class/graphics/fb0/lcd_inversion_mode",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"/sys/class/graphics/fb0/lcd_check_reg",
	"LCD_CUR_DET_MODE",
	"OVER_CURRENT_DETECTION",
	"OVER_VOLTAGE_DETECTION",
	"/sys/class/graphics/fb0/lcd_general_test",
	"/sys/class/graphics/fb0/vertical_line_test",
	"/sys/class/graphics/fb0/ddic_lv_detect",
	"/sys/class/graphics/fb0/avdd_detect",
	"/sys/class/graphics/fb0/lcd_oneside_mode",
	"/sys/class/graphics/fb0/mipi_detect",
};

static struct lcd_fact_ops *g_lcd_fact_ops;
struct lcd_fact_desc g_fact_info[MAX_ACTIVE_PANEL];

int lcd_fact_ops_register(struct lcd_fact_ops *ops)
{
	if (g_lcd_fact_ops) {
		LCD_KIT_ERR("g_lcd_fact_ops has already registered!\n");
		return LCD_KIT_FAIL;
	}
	g_lcd_fact_ops = ops;
	LCD_KIT_INFO("g_lcd_fact_ops register success!\n");
	return LCD_KIT_OK;
}

int lcd_fact_ops_unregister(struct lcd_fact_ops *ops)
{
	if (g_lcd_fact_ops == ops) {
		g_lcd_fact_ops = NULL;
		LCD_KIT_INFO("g_lcd_fact_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_lcd_fact_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_fact_ops *lcd_get_fact_ops(void)
{
	return g_lcd_fact_ops;
}

struct lcd_fact_desc *lcd_kit_get_fact_info(uint32_t panel_id)
{
	return &g_fact_info[panel_id];
}

static ssize_t lcd_kit_inversion_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->inversion_mode_show)
		ret = fact_ops->inversion_mode_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_inversion_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->inversion_mode_store) {
		ret = fact_ops->inversion_mode_store(dev, attr, buf, count);
		if (ret)
			LCD_KIT_ERR("inversion mode set fail!\n");
	}

	return count;
}

static ssize_t lcd_kit_scan_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->scan_mode_show)
		ret = fact_ops->scan_mode_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_scan_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->scan_mode_store) {
		ret = fact_ops->scan_mode_store(dev, attr, buf, count);
		if (ret)
			LCD_KIT_ERR("scan mode set fail!\n");
	}
	return count;
}

static ssize_t lcd_kit_check_reg_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->check_reg_show)
		ret = fact_ops->check_reg_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_gram_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->gram_check_show)
		ret = fact_ops->gram_check_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_gram_check_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->gram_check_store)
		(void)fact_ops->gram_check_store(dev, attr, buf, count);
	return count;
}

static ssize_t lcd_kit_sleep_ctrl_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->sleep_ctrl_show)
		ret = fact_ops->sleep_ctrl_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_sleep_ctrl_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->sleep_ctrl_store) {
		ret = fact_ops->sleep_ctrl_store(dev, attr, buf, count);
		if (ret)
			LCD_KIT_ERR("sleep ctrl set fail!\n");
	}
	return count;
}

static ssize_t lcd_kit_poweric_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->poweric_det_show)
		ret = fact_ops->poweric_det_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_poweric_det_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->poweric_det_store) {
		ret = fact_ops->poweric_det_store(dev, attr, buf, count);
		if (!ret)
			LCD_KIT_ERR("poweric det fail!\n");
	}
	return count;
}

static ssize_t lcd_kit_amoled_pcd_errflag_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->pcd_errflag_check_store)
		fact_ops->pcd_errflag_check_store(dev, attr, buf);
	return count;
}

static ssize_t lcd_kit_amoled_pcd_errflag_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->pcd_errflag_check_show)
		ret = fact_ops->pcd_errflag_check_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_test_config_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->test_config_show)
		ret = fact_ops->test_config_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_test_config_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->test_config_store)
		(void)fact_ops->test_config_store(dev, attr, buf, count);
	return count;
}

static ssize_t lcd_kit_lv_detect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->lv_detect_show)
		ret = fact_ops->lv_detect_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_current_detect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->current_detect_show)
		ret = fact_ops->current_detect_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_ldo_check_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->ldo_check_show)
		ret = fact_ops->ldo_check_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_general_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (fact_ops == NULL) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->general_test_show)
		ret = fact_ops->general_test_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_vertical_line_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (fact_ops == NULL) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->vtc_line_test_show)
		ret = fact_ops->vtc_line_test_show(dev, attr, buf);

	return ret;
}

static ssize_t lcd_kit_vertical_line_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (fact_ops == NULL) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->vtc_line_test_store)
		(void)fact_ops->vtc_line_test_store(dev, attr,
			buf, count);
	return count;
}

static ssize_t lcd_kit_hkadc_debug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->hkadc_debug_show)
		ret = fact_ops->hkadc_debug_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_hkadc_debug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->hkadc_debug_store) {
		ret = fact_ops->hkadc_debug_store(dev, attr, buf, count);
		if (ret)
			LCD_KIT_ERR("hkadc set fail\n");
	}
	return count;
}

static ssize_t lcd_kit_bl_self_test_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->bl_self_test_show)
		ret = fact_ops->bl_self_test_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_avdd_detect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->avdd_detect_show)
		ret = fact_ops->avdd_detect_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_oneside_mode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (fact_ops->oneside_mode_show)
		ret = fact_ops->oneside_mode_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_oneside_mode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (!fact_ops->oneside_mode_store) {
		LCD_KIT_ERR("oneside_mode_store is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (fact_ops->oneside_mode_store) {
		ret = fact_ops->oneside_mode_store(dev, attr, buf, count);
		if (!ret)
			LCD_KIT_ERR("oneside mode set fail\n");
	}
	return count;
}

static ssize_t lcd_kit_fpc_det_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->fpc_det_show)
		ret = fact_ops->fpc_det_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_fps_cmd_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("fact_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}
	if (fact_ops->fps_cmd_show)
		ret = fact_ops->fps_cmd_show(dev, attr, buf);
	return ret;
}

static ssize_t lcd_kit_fps_cmd_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct lcd_fact_ops *fact_ops = NULL;
	ssize_t ret;

	fact_ops = lcd_get_fact_ops();
	if (!fact_ops) {
		LCD_KIT_ERR("sysfs_ops is NULL!\n");
		return LCD_KIT_FAIL;
	}

	if (fact_ops->fps_cmd_store) {
		ret = fact_ops->fps_cmd_store(dev, attr, buf, count);
		if (!ret)
			LCD_KIT_ERR("fps cmd set fail\n");
	}
	return count;
}

ssize_t lcd_kit_mipi_detect_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t ret = LCD_KIT_OK;
	struct lcd_fact_ops *fact_ops = NULL;

	LCD_KIT_INFO("enter\n");
	fact_ops = lcd_get_fact_ops();
	if (fact_ops == NULL) {
		LCD_KIT_ERR("sysfs_ops is NULL\n");
		return LCD_KIT_FAIL;
	}
	mutex_lock(&COMMON_LOCK->sysfs_lock.mipi_detect_lock);
	if (fact_ops->mipi_detect_show)
		ret = fact_ops->mipi_detect_show(dev, attr, buf);
	mutex_unlock(&COMMON_LOCK->sysfs_lock.mipi_detect_lock);
	LCD_KIT_INFO("exit\n");
	return ret;
}

ssize_t lcd_kit_mipi_detect_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret;
	struct lcd_fact_ops *fact_ops = NULL;

	LCD_KIT_INFO("enter\n");
	fact_ops = lcd_get_fact_ops();
	if (fact_ops == NULL) {
		LCD_KIT_ERR("sysfs_ops is NULL\n");
		return LCD_KIT_FAIL;
	}
	mutex_lock(&COMMON_LOCK->sysfs_lock.mipi_detect_lock);
	if (fact_ops->mipi_detect_store) {
		ret = fact_ops->mipi_detect_store(dev, attr,
			buf, count);
		if (ret)
			LCD_KIT_ERR("mipi error detect store fail\n");
	}
	mutex_unlock(&COMMON_LOCK->sysfs_lock.mipi_detect_lock);
	LCD_KIT_INFO("exit\n");
	return count;
}

static DEVICE_ATTR(lcd_inversion_mode, 0644, lcd_kit_inversion_mode_show,
	lcd_kit_inversion_mode_store);
static DEVICE_ATTR(lcd_scan_mode, 0644, lcd_kit_scan_mode_show,
	lcd_kit_scan_mode_store);
static DEVICE_ATTR(lcd_check_reg, 0444, lcd_kit_check_reg_show, NULL);
static DEVICE_ATTR(lcd_checksum, 0644, lcd_kit_gram_check_show,
	lcd_kit_gram_check_store);
static DEVICE_ATTR(lcd_sleep_ctrl, 0644, lcd_kit_sleep_ctrl_show,
	lcd_kit_sleep_ctrl_store);
static DEVICE_ATTR(amoled_pcd_errflag_check, 0644,
	lcd_kit_amoled_pcd_errflag_show,
	lcd_kit_amoled_pcd_errflag_store);
static DEVICE_ATTR(lcd_test_config, 0640, lcd_kit_test_config_show,
	lcd_kit_test_config_store);
static DEVICE_ATTR(lv_detect, 0640, lcd_kit_lv_detect_show, NULL);
static DEVICE_ATTR(current_detect, 0640, lcd_kit_current_detect_show, NULL);
static DEVICE_ATTR(lcd_ldo_check, 0444, lcd_kit_ldo_check_show, NULL);
static DEVICE_ATTR(lcd_general_test, 0444, lcd_kit_general_test_show, NULL);
static DEVICE_ATTR(vertical_line_test, 0644,
	lcd_kit_vertical_line_test_show, lcd_kit_vertical_line_test_store);
static DEVICE_ATTR(lcd_hkadc, 0644, lcd_kit_hkadc_debug_show,
	lcd_kit_hkadc_debug_store);
static DEVICE_ATTR(bl_self_test, 0644, lcd_kit_bl_self_test_show, NULL);
static DEVICE_ATTR(lcd_poweric_det, 0644, lcd_kit_poweric_det_show,
	lcd_kit_poweric_det_store);
static DEVICE_ATTR(avdd_detect, 0644, lcd_kit_avdd_detect_show, NULL);
static DEVICE_ATTR(lcd_oneside_mode, 0644, lcd_kit_oneside_mode_show,
	lcd_kit_oneside_mode_store);
static DEVICE_ATTR(ddic_lv_detect, 0644, lcd_kit_ddic_lv_detect_test_show,
	lcd_kit_ddic_lv_detect_test_store);
static DEVICE_ATTR(lcd_fpc_detect, 0644, lcd_kit_fpc_det_show, NULL);
static DEVICE_ATTR(lcd_fps_cmd, 0644, lcd_kit_fps_cmd_show, lcd_kit_fps_cmd_store);
static DEVICE_ATTR(mipi_detect, 0644, lcd_kit_mipi_detect_show,
	lcd_kit_mipi_detect_store);

/* factory attrs */
struct attribute *lcd_factory_attrs[] = {
	&dev_attr_lcd_inversion_mode.attr,
	&dev_attr_lcd_scan_mode.attr,
	&dev_attr_lcd_check_reg.attr,
	&dev_attr_lcd_checksum.attr,
	&dev_attr_lcd_sleep_ctrl.attr,
	&dev_attr_amoled_pcd_errflag_check.attr,
	&dev_attr_lcd_test_config.attr,
	&dev_attr_lv_detect.attr,
	&dev_attr_current_detect.attr,
	&dev_attr_lcd_ldo_check.attr,
	&dev_attr_lcd_general_test.attr,
	&dev_attr_vertical_line_test.attr,
	&dev_attr_lcd_hkadc.attr,
	&dev_attr_bl_self_test.attr,
	&dev_attr_lcd_poweric_det.attr,
	&dev_attr_avdd_detect.attr,
	&dev_attr_lcd_oneside_mode.attr,
	&dev_attr_ddic_lv_detect.attr,
	&dev_attr_lcd_fpc_detect.attr,
	&dev_attr_lcd_fps_cmd.attr,
	&dev_attr_mipi_detect.attr,
};

static struct attribute* lcd_fac_sysfs_attrs[FAC_SYSFS_ATTRS_NUM] = { NULL };
struct attribute_group lcd_fac_sysfs_attr_group = {
	.attrs = lcd_fac_sysfs_attrs,
};

int lcd_create_fact_sysfs(struct kobject *obj)
{
	int rc, i;

	for (i = 0; i < (int)(ARRAY_SIZE(lcd_factory_attrs)); i++) {
		if (lcd_factory_attrs[i] && (i < FAC_SYSFS_ATTRS_NUM))
			lcd_fac_sysfs_attrs[i] = lcd_factory_attrs[i];
	}
	rc = sysfs_create_group(obj, &lcd_fac_sysfs_attr_group);
	if (rc)
		LCD_KIT_ERR("sysfs group creation failed, rc=%d\n", rc);
	return rc;
}

int lcd_kit_inversion_get_mode(char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (buf == NULL) {
		LCD_KIT_ERR("null pointer\n");
		return LCD_KIT_FAIL;
	}
	if (!FACT_INFO->inversion.support) {
		LCD_KIT_DEBUG("not support inversion\n");
		return ret;
	}
	ret = snprintf(buf, PAGE_SIZE, "%d\n", FACT_INFO->inversion.mode);
	return ret;
}

int lcd_kit_inversion_set_mode(void *hdl, u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops or mipi_tx!\n");
		return LCD_KIT_FAIL;
	}
	if (!FACT_INFO->inversion.support) {
		LCD_KIT_DEBUG("not support inversion\n");
		return ret;
	}
	switch (mode) {
	case COLUMN_MODE:
		ret = adapt_ops->mipi_tx(hdl, &FACT_INFO->inversion.column_cmds);
		break;
	case DOT_MODE:
		ret = adapt_ops->mipi_tx(hdl, &FACT_INFO->inversion.dot_cmds);
		break;
	default:
		return LCD_KIT_FAIL;
	}
	FACT_INFO->inversion.mode = (int)mode;
	LCD_KIT_INFO("inversion.support = %d,inversion.mode = %d\n",
		FACT_INFO->inversion.support, FACT_INFO->inversion.mode);
	return ret;
}

int lcd_kit_scan_get_mode(char *buf)
{
	int ret = LCD_KIT_OK;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (FACT_INFO->scan.support)
		ret = snprintf(buf, PAGE_SIZE, "%d\n", FACT_INFO->scan.mode);
	return ret;
}

int lcd_kit_scan_set_mode(void *hdl,  u32 mode)
{
	int ret = LCD_KIT_OK;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops || !adapt_ops->mipi_tx) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (!FACT_INFO->scan.support) {
		LCD_KIT_DEBUG("not support scan\n");
		return ret;
	}
	switch (mode) {
	case FORWORD_MODE:
		ret = adapt_ops->mipi_tx(hdl, &FACT_INFO->scan.forword_cmds);
		break;

	case REVERT_MODE:
		ret = adapt_ops->mipi_tx(hdl, &FACT_INFO->scan.revert_cmds);
		break;

	default:
		return LCD_KIT_FAIL;
	}
	FACT_INFO->scan.mode = (int)mode;
	LCD_KIT_INFO("scan.support = %d,scan.mode = %d\n",
		FACT_INFO->scan.support, FACT_INFO->scan.mode);
	return ret;
}

int lcd_kit_check_reg(void *hdl, char *buf)
{
	int ret = LCD_KIT_OK;
	uint8_t read_value[MAX_REG_READ_COUNT] = {0};
	int i = 0;
	char *expect_ptr = NULL;
	struct lcd_kit_adapt_ops *adapt_ops = NULL;
	uint32_t panel_id = lcd_get_active_panel_id();

	adapt_ops = lcd_kit_get_adapt_ops();
	if (!adapt_ops) {
		LCD_KIT_ERR("can not register adapt_ops!\n");
		return LCD_KIT_FAIL;
	}
	if (FACT_INFO->check_reg.support) {
		expect_ptr = (char *)FACT_INFO->check_reg.value.buf;
		if (adapt_ops->mipi_rx)
			ret = adapt_ops->mipi_rx(hdl, read_value,
				MAX_REG_READ_COUNT - 1,
				&FACT_INFO->check_reg.cmds);
		for (i = 0; i < FACT_INFO->check_reg.cmds.cmd_cnt; i++) {
			if ((char)read_value[i] != expect_ptr[i]) {
				ret = -1;
				LCD_KIT_ERR("read_value[%u] = 0x%x, but expect_ptr[%u] = 0x%x!\n",
					i, read_value[i], i, expect_ptr[i]);
				break;
			}
			LCD_KIT_INFO("read_value[%u] = 0x%x same with expect value!\n",
						 i, read_value[i]);
		}
		if (ret == 0)
			ret = snprintf(buf, PAGE_SIZE, "OK\n");
		else
			ret = snprintf(buf, PAGE_SIZE, "FAIL\n");
		LCD_KIT_INFO("check_reg result:%s\n", buf);
	}
	return ret;
}

int lcd_kit_is_enter_pt_mode(void)
{
	uint32_t panel_id = lcd_get_active_panel_id();

	if (FACT_INFO->pt.support)
		return FACT_INFO->pt.mode;
	return LCD_KIT_OK;
}

int lcd_kit_get_pt_station_status(uint32_t panel_id)
{
	int mode = 0;

	if (FACT_INFO->pt.support)
		mode = FACT_INFO->pt.mode;
	LCD_KIT_INFO("mode = %d\n", mode);
	return mode;
}

int lcd_kit_get_sleep_mode(uint32_t panel_id, char *buf)
{
	int ret = LCD_KIT_OK;

	if (FACT_INFO->pt.support)
		ret = snprintf(buf, PAGE_SIZE,
			"panel_id %u:PT test mod = %d, PT test support = %d\n",
			panel_id, FACT_INFO->pt.mode, FACT_INFO->pt.support);
	return ret;
}

int lcd_kit_set_sleep_mode(uint32_t panel_id, u32 mode)
{
	int ret = LCD_KIT_OK;

	if (FACT_INFO->pt.support)
		FACT_INFO->pt.mode = mode;
		LCD_KIT_INFO("panel_id %u:pt.support = %d, pt.mode = %d\n",
		panel_id, FACT_INFO->pt.support, FACT_INFO->pt.mode);
	return ret;
}

int lcd_kit_get_test_config(uint32_t panel_id, char *buf)
{
	int i;
	int ret = LCD_KIT_FAIL;

	for (i = 0; i < (int)ARRAY_SIZE(g_sence_array); i++) {
		if (g_sence_array[i] == NULL) {
			ret = LCD_KIT_FAIL;
			LCD_KIT_INFO("Sence cmd is end, total num is:%d\n", i);
			break;
		}
		if (!strncmp(FACT_INFO->lcd_cmd_now, g_sence_array[i],
			strlen(FACT_INFO->lcd_cmd_now))) {
			LCD_KIT_INFO("current test cmd:%s,return cmd:%s\n",
				FACT_INFO->lcd_cmd_now, g_cmd_array[i]);
			return snprintf(buf, PAGE_SIZE, g_cmd_array[i]);
		}
	}
	return ret;
}

int lcd_kit_set_test_config(uint32_t panel_id, const char *buf)
{
	if (buf == NULL) {
		LCD_KIT_ERR("buf is null\n");
		return LCD_KIT_FAIL;
	}
	if (strlen(buf) < LCD_KIT_CMD_NAME_MAX) {
		memcpy(FACT_INFO->lcd_cmd_now, buf, strlen(buf) + 1);
		LCD_KIT_INFO("current test cmd:%s\n", FACT_INFO->lcd_cmd_now);
	} else {
		memcpy(FACT_INFO->lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
		LCD_KIT_INFO("invalid test cmd:%s\n", FACT_INFO->lcd_cmd_now);
	}
	return LCD_KIT_OK;
}

int lcd_kit_is_enter_sleep_mode(void)
{
	int sleep_mode = 0;
	uint32_t panel_id = lcd_get_active_panel_id();

	if (FACT_INFO->pt.support)
		sleep_mode = FACT_INFO->pt.mode;
	return sleep_mode;
}

static void parse_dt_inversion(uint32_t panel_id, struct device_node *np)
{
	/* dot-colomn inversion */
	lcd_kit_parse_u32(np, "lcd-kit,inversion-support",
		&FACT_INFO->inversion.support, 0);
	if (FACT_INFO->inversion.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,inversion-column-cmds",
			"lcd-kit,inversion-column-cmds-state",
			&FACT_INFO->inversion.column_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,inversion-dot-cmds",
			"lcd-kit,inversion-dot-cmds-state",
			&FACT_INFO->inversion.dot_cmds);
	}
}

static void parse_dt_scan(uint32_t panel_id, struct device_node *np)
{
	/* forword-revert scan */
	lcd_kit_parse_u32(np, "lcd-kit,scan-support",
		&FACT_INFO->scan.support, 0);
	if (FACT_INFO->scan.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,scan-forword-cmds",
			"lcd-kit,scan-forword-cmds-state",
			&FACT_INFO->scan.forword_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,scan-revert-cmds",
			"lcd-kit,scan-revert-cmds-state",
			&FACT_INFO->scan.revert_cmds);
	}
}

static void parse_dt_check_reg(uint32_t panel_id, struct device_node *np)
{
	/* check reg */
	lcd_kit_parse_u32(np, "lcd-kit,check-reg-support",
		&FACT_INFO->check_reg.support, 0);
	if (FACT_INFO->check_reg.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,check-reg-cmds",
			"lcd-kit,check-reg-cmds-state",
			&FACT_INFO->check_reg.cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,check-reg-value",
			&FACT_INFO->check_reg.value);
	}
}
static void parse_dt_checksum(uint32_t panel_id, struct device_node *np)
{
	unsigned int checksum_value_size = CHECKSUM_VALUE_SIZE;
	/* check sum */
	lcd_kit_parse_u32(np, "lcd-kit,checksum-support",
		&FACT_INFO->checksum.support, 0);
	if (FACT_INFO->checksum.support) {
		lcd_kit_parse_u32(np,
			"lcd-kit,checksum-special-support",
			&FACT_INFO->checksum.special_support, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-enable-cmds",
			"lcd-kit,checksum-enable-cmds-state",
			&FACT_INFO->checksum.enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-disable-cmds",
			"lcd-kit,checksum-disable-cmds-state",
			&FACT_INFO->checksum.disable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,checksum-cmds",
			"lcd-kit,checksum-cmds-state",
			&FACT_INFO->checksum.checksum_cmds);
		lcd_kit_parse_u32(np, "lcd-kit,checksum-value-size",
			&checksum_value_size, CHECKSUM_VALUE_SIZE);
		lcd_kit_parse_arrays_data(np, "lcd-kit,checksum-value",
			&FACT_INFO->checksum.value, checksum_value_size);
		lcd_kit_parse_arrays_data(np, "lcd-kit,checksum-value-ext",
			&FACT_INFO->checksum.value_ext, checksum_value_size);
		lcd_kit_parse_arrays_data(np, "lcd-kit,checksum-dsi1-value",
			&FACT_INFO->checksum.dsi1_value, CHECKSUM_VALUE_SIZE);
		/* checksum stress test */
		lcd_kit_parse_u32(np,
			"lcd-kit,checksum-stress-test-support",
			&FACT_INFO->checksum.stress_test_support, 0);
		if (FACT_INFO->checksum.stress_test_support) {
			lcd_kit_parse_u32(np,
				"lcd-kit,checksum-stress-test-vdd",
				&FACT_INFO->checksum.vdd, 0);
			lcd_kit_parse_u32(np,
				"lcd-kit,checksum-stress-test-mipi",
				&FACT_INFO->checksum.mipi_clk, 0);
		}
	}
}
static void parse_dt_pt(uint32_t panel_id, struct device_node *np)
{
	/* pt test */
	lcd_kit_parse_u32(np, "lcd-kit,pt-support",
		&FACT_INFO->pt.support, 0);
	if (FACT_INFO->pt.support) {
		lcd_kit_parse_u32(np,
			"lcd-kit,pt-panel-ulps-support",
			&FACT_INFO->pt.panel_ulps_support, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,pt-avdd-enable-cmds",
			"lcd-kit,pt-avdd-enable-cmds-state",
			&FACT_INFO->pt.avdd_enable_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,pt-avdd-disable-cmds",
			"lcd-kit,pt-avdd-disable-cmds-state",
			&FACT_INFO->pt.avdd_disable_cmds);
	}
}

static void parse_dt_pcd_errflag(uint32_t panel_id, struct device_node *np)
{
	/* pcd errflag check */
	lcd_kit_parse_u32(np, "lcd-kit,pcd-errflag-check-support",
		&FACT_INFO->pcd_errflag_check.pcd_errflag_check_support, 0);
	if (FACT_INFO->pcd_errflag_check.pcd_errflag_check_support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,pcd-detect-open-cmds",
			"lcd-kit,pcd-read-cmds-state",
			&FACT_INFO->pcd_errflag_check.pcd_detect_open_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,pcd-detect-close-cmds",
			"lcd-kit,pcd-read-cmds-state",
			&FACT_INFO->pcd_errflag_check.pcd_detect_close_cmds);
	}
}

static void parse_dt_oneside_mode(uint32_t panel_id, struct device_node *np)
{
	/* oneside mode test */
	lcd_kit_parse_u32(np, "lcd-kit,oneside-mode-support",
		&FACT_INFO->oneside_mode.support, 0);
	if (FACT_INFO->oneside_mode.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,oneside-mode-left",
			"lcd-kit,oneside-mode-left-state",
			&FACT_INFO->oneside_mode.left_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,oneside-mode-right",
			"lcd-kit,oneside-mode-right-state",
			&FACT_INFO->oneside_mode.right_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,oneside-mode-exit",
			"lcd-kit,oneside-mode-exit-state",
			&FACT_INFO->oneside_mode.exit_cmds);
	}
}

static void parse_dt_color_aod_detect(uint32_t panel_id, struct device_node *np)
{
	/* color aod detect */
	lcd_kit_parse_u32(np, "lcd-kit,color-aod-detect-support",
		&FACT_INFO->color_aod_det.support, 0);
	if (FACT_INFO->color_aod_det.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,color-aod-detect-enter",
			"lcd-kit,color-aod-detect-enter-state",
			&FACT_INFO->color_aod_det.enter_cmds);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,color-aod-detect-exit",
			"lcd-kit,color-aod-detect-exit-state",
			&FACT_INFO->color_aod_det.exit_cmds);
	}
}
static void parse_dt_lv_det(uint32_t panel_id, struct device_node *np)
{
	/* low voltage detect */
	lcd_kit_parse_u32(np, "lcd-kit,lv-det-support",
		&FACT_INFO->lv_det.support, 0);
	if (FACT_INFO->lv_det.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,lv-det-cmds",
			"lcd-kit,lv-det-cmds-state",
			&FACT_INFO->lv_det.detect_cmds);
		lcd_kit_parse_array_data(np, "lcd-kit,lv-det-value",
			&FACT_INFO->lv_det.value);
	}
	/* ddic low voltage detect test */
	lcd_kit_parse_u32(np, "lcd-kit,ddic-lv-det-support",
		&FACT_INFO->ddic_lv_detect.support, 0);
	if (FACT_INFO->ddic_lv_detect.support) {
		FACT_INFO->ddic_lv_detect.pic_index = INVALID_INDEX;
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-enter1-cmds",
			"lcd-kit,ddic-lv-det-enter1-cmds-state",
			&FACT_INFO->ddic_lv_detect.enter_cmds[DET1_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-enter2-cmds",
			"lcd-kit,ddic-lv-det-enter2-cmds-state",
			&FACT_INFO->ddic_lv_detect.enter_cmds[DET2_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-enter3-cmds",
			"lcd-kit,ddic-lv-det-enter3-cmds-state",
			&FACT_INFO->ddic_lv_detect.enter_cmds[DET3_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-enter4-cmds",
			"lcd-kit,ddic-lv-det-enter4-cmds-state",
			&FACT_INFO->ddic_lv_detect.enter_cmds[DET4_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-rd1-cmds",
			"lcd-kit,ddic-lv-det-rd1-cmds-state",
			&FACT_INFO->ddic_lv_detect.rd_cmds[DET1_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-rd2-cmds",
			"lcd-kit,ddic-lv-det-rd2-cmds-state",
			&FACT_INFO->ddic_lv_detect.rd_cmds[DET2_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-rd3-cmds",
			"lcd-kit,ddic-lv-det-rd3-cmds-state",
			&FACT_INFO->ddic_lv_detect.rd_cmds[DET3_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,ddic-lv-det-rd4-cmds",
			"lcd-kit,ddic-lv-det-rd4-cmds-state",
			&FACT_INFO->ddic_lv_detect.rd_cmds[DET4_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,ddic-lv-det-value1",
			&FACT_INFO->ddic_lv_detect.value[DET1_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,ddic-lv-det-value2",
			&FACT_INFO->ddic_lv_detect.value[DET2_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,ddic-lv-det-value3",
			&FACT_INFO->ddic_lv_detect.value[DET3_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,ddic-lv-det-value4",
			&FACT_INFO->ddic_lv_detect.value[DET4_INDEX]);
	}
}

static void parse_dt_vtc_line(uint32_t panel_id, struct device_node *np)
{
	/* vertical line test */
	lcd_kit_parse_u32(np, "lcd-kit,vtc-line-support",
		&FACT_INFO->vtc_line.support, 0);
	if (FACT_INFO->vtc_line.support) {
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,vtc-line-cmds",
			"lcd-kit,vtc-line-cmds-state",
			&FACT_INFO->vtc_line.vtc_cmds);
		lcd_kit_parse_u32(np, "lcd-kit,vtc-line-vsp",
			&FACT_INFO->vtc_line.vtc_vsp, 0);
		lcd_kit_parse_u32(np, "lcd-kit,vtc-line-vsn",
			&FACT_INFO->vtc_line.vtc_vsn, 0);
		lcd_kit_parse_u32(np, "lcd-kit,vtc-line-no-reset",
			&FACT_INFO->vtc_line.vtc_no_reset, 0);
	}
}

static void parse_dt_hor_line(uint32_t panel_id, struct device_node *np)
{
	/* horizontal line test */
	lcd_kit_parse_u32(np, "lcd-kit,hor-line-support",
		&FACT_INFO->hor_line.support, 0);
	if (FACT_INFO->hor_line.support) {
		lcd_kit_parse_u32(np, "lcd-kit,hor-line-duration",
			&FACT_INFO->hor_line.duration, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,hor-line-cmds",
			"lcd-kit,hor-line-cmds-state",
			&FACT_INFO->hor_line.hl_cmds);
	}
}

static void parse_dt_poweric_det(uint32_t panel_id, struct device_node *np)
{
	/* poweric detect */
	lcd_kit_parse_u32(np, "lcd-kit,poweric-detect-support",
		&FACT_INFO->poweric_detect.support, 0);
	if (FACT_INFO->poweric_detect.support) {
		lcd_kit_parse_u32(np, "lcd-kit,poweric-number",
			&FACT_INFO->poweric_detect.poweric_num, 0);
		lcd_kit_parse_u32(np, "lcd-kit,poweric-detect-value",
			&FACT_INFO->poweric_detect.detect_val, 0);
		lcd_kit_parse_array_data(np, "lcd-kit,poweric-gpio-enable-list",
			&FACT_INFO->poweric_detect.gpio_list);
		lcd_kit_parse_array_data(np, "lcd-kit,poweric-gpio-enable-val",
			&FACT_INFO->poweric_detect.gpio_val);
		lcd_kit_parse_array_data(np, "lcd-kit,poweric-gpio-enable-number-list",
			&FACT_INFO->poweric_detect.gpio_num_list);
		lcd_kit_parse_u32(np, "lcd-kit,elvdd-detect-gpio",
			&FACT_INFO->poweric_detect.detect_gpio, 0);
	}
}

static void parse_dt_fpc_det(uint32_t panel_id, struct device_node *np)
{
	/* fpc detect */
	lcd_kit_parse_u32(np, "lcd-kit,fpc-detect-support",
		&FACT_INFO->fpc_detect.support, 0);
	if (FACT_INFO->fpc_detect.support) {
		lcd_kit_parse_u32(np, "lcd-kit,fpc-detect-pinctrl-cfg",
			&FACT_INFO->fpc_detect.pinctrl_cfg, 0);
		lcd_kit_parse_array_data(np, "lcd-kit,fpc-det-gpio-list",
			&FACT_INFO->fpc_detect.det_gpio_list);
		lcd_kit_parse_array_data(np, "lcd-kit,fpc-exp-gpio-val-list",
			&FACT_INFO->fpc_detect.exp_gpio_val_list);
	}
}

static void parse_dt_mipi_error_detect(uint32_t panel_id, struct device_node *np)
{
	/* low voltage detect */
	lcd_kit_parse_u32(np, "lcd-kit,mipi-detect-support",
		&FACT_INFO->mipi_detect.support, 0);

	if (FACT_INFO->mipi_detect.support) {
		FACT_INFO->mipi_detect.pic_index = INVALID_INDEX;
		lcd_kit_parse_u32(np, "lcd-kit,mipi-detect-threshold",
			&FACT_INFO->mipi_detect.threshold, 0);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-enter1-cmds",
			"lcd-kit,mipi-detect-enter1-cmds-state",
			&FACT_INFO->mipi_detect.enter_cmds[DET1_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-enter2-cmds",
			"lcd-kit,mipi-detect-enter2-cmds-state",
			&FACT_INFO->mipi_detect.enter_cmds[DET2_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-enter3-cmds",
			"lcd-kit,mipi-detect-enter3-cmds-state",
			&FACT_INFO->mipi_detect.enter_cmds[DET3_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-enter4-cmds",
			"lcd-kit,mipi-detect-enter4-cmds-state",
			&FACT_INFO->mipi_detect.enter_cmds[DET4_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-rd1-cmds",
			"lcd-kit,mipi-detect-rd1-cmds-state",
			&FACT_INFO->mipi_detect.rd_cmds[DET1_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-rd2-cmds",
			"lcd-kit,mipi-detect-rd2-cmds-state",
			&FACT_INFO->mipi_detect.rd_cmds[DET2_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-rd3-cmds",
			"lcd-kit,mipi-detect-rd3-cmds-state",
			&FACT_INFO->mipi_detect.rd_cmds[DET3_INDEX]);
		lcd_kit_parse_dcs_cmds(np, "lcd-kit,mipi-detect-rd4-cmds",
			"lcd-kit,mipi-detect-rd4-cmds-state",
			&FACT_INFO->mipi_detect.rd_cmds[DET4_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,mipi-detect-value1",
			&FACT_INFO->mipi_detect.value[DET1_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,mipi-detect-value2",
			&FACT_INFO->mipi_detect.value[DET2_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,mipi-detect-value3",
			&FACT_INFO->mipi_detect.value[DET3_INDEX]);
		lcd_kit_parse_array_data(np, "lcd-kit,mipi-detect-value4",
			&FACT_INFO->mipi_detect.value[DET4_INDEX]);
	}
}

static void lcd_fact_parse_dt(uint32_t panel_id, struct device_node *np)
{
	parse_dt_inversion(panel_id, np);
	parse_dt_scan(panel_id, np);
	parse_dt_check_reg(panel_id, np);
	parse_dt_pt(panel_id, np);
	parse_dt_pcd_errflag(panel_id, np);
	parse_dt_oneside_mode(panel_id, np);
	parse_dt_color_aod_detect(panel_id, np);
	/* parse low voltage detect */
	parse_dt_lv_det(panel_id, np);
	/* parse vertical line test */
	parse_dt_vtc_line(panel_id, np);
	/* parse horizontal line test */
	parse_dt_hor_line(panel_id, np);
	parse_dt_poweric_det(panel_id, np);
	parse_dt_checksum(panel_id, np);
	parse_dt_fpc_det(panel_id, np);
	/* mipi detect parse */
	parse_dt_mipi_error_detect(panel_id, np);
}

void lcd_kit_fact_init(uint32_t panel_id, struct device_node *np)
{
	lcd_fact_parse_dt(panel_id, np);
}
