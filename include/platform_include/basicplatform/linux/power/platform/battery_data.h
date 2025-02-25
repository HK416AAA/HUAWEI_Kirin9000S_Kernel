/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: driver for battery
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

#ifndef _BATTERY_DATA
#define _BATTERY_DATA

#define MAX_SINGLE_LUT_COLS            20

#define PC_CC_ROWS                     29
#define PC_CC_COLS                     13

#define PC_TEMP_ROWS                   29
#define PC_TEMP_COLS                   8
#define TEMP_SAMPLING_POINTS           6

#define TEMP_PARA_LEVEL                10
#define VOLT_PARA_LEVEL                4
#define SEGMENT_PARA_LEVEL             3

#define BATT_IDENTIFY_BY_VOLT          "id_volt"
#define BATT_IDENTIFY_BY_SN            "id_sn"
#define ID_SN_SIZE                     6
#define DEFAULT_IFULL_SET              300
#define DEC                            10

struct single_row_lut {
	int x[MAX_SINGLE_LUT_COLS];
	int y[MAX_SINGLE_LUT_COLS];
	int cols;
};

/**
 * struct sf_lut -
 * @rows:	number of percent charge entries should be <= PC_CC_ROWS
 * @cols:	number of charge cycle entries should be <= PC_CC_COLS
 * @row_entries:	the charge cycles/temperature at which sf data
 *			is available in the table.
 *		The charge cycles must be in increasing order from 0 to rows.
 * @percent:	the percent charge at which sf data is available in the table
 *		The  percentcharge must be in decreasing order from 0 to cols.
 * @sf:		the scaling factor data
 */
struct sf_lut {
	int rows;
	int cols;
	int row_entries[PC_CC_COLS];
	int percent[PC_CC_ROWS];
	int sf[PC_CC_ROWS][PC_CC_COLS];
};

/**
 * struct pc_temp_ocv_lut -
 * @rows:	number of percent charge entries should be <= PC_TEMP_ROWS
 * @cols:	number of temperature entries should be <= PC_TEMP_COLS
 * @temp:	the temperatures at which ocv data is available in the table
 *		The temperatures must be in increasing order from 0 to rows.
 * @percent:	the percent charge at which ocv data is available in the table
 *		The  percentcharge must be in decreasing order from 0 to cols.
 * @ocv:	the open circuit voltage
 */
struct pc_temp_ocv_lut {
	int rows;
	int cols;
	int temp[PC_TEMP_COLS];
	int percent[PC_TEMP_ROWS];
	int ocv[PC_TEMP_ROWS][PC_TEMP_COLS];
};

enum temp_para_info {
	TEMP_PARA_TEMP_MIN = 0,
	TEMP_PARA_TEMP_MAX,
	TEMP_PARA_IIN,
	TEMP_PARA_ICHG,
	TEMP_PARA_VTERM,
	TEMP_PARA_TEMP_BACK,
	TEMP_PARA_TOTAL,
};

enum volt_para_info {
	VOLT_PARA_VOLT_MIN = 0,
	VOLT_PARA_VOLT_MAX,
	VOLT_PARA_IIN,
	VOLT_PARA_ICHG,
	VOLT_PARA_VOLT_BACK,
	VOLT_PARA_TOTAL,
};
enum segment_para_info {
	SEGMENT_PARA_VOLT_MIN = 0,
	SEGMENT_PARA_VOLT_MAX,
	SEGMENT_PARA_ICHG,
	SEGMENT_PARA_VTERM,
	SEGMENT_PARA_VOLT_BACK,
	SEGMENT_PARA_TOTAL,
};
enum bat_id_info {
	BAT_ID_VALID = 0,
	BAT_ID_INVALID,
};

struct chrg_para_lut {
	int temp_len;
	long temp_data[TEMP_PARA_LEVEL][TEMP_PARA_TOTAL];
	int volt_len;
	long volt_data[VOLT_PARA_LEVEL][VOLT_PARA_TOTAL];
	int segment_len;
	long segment_data[SEGMENT_PARA_LEVEL][SEGMENT_PARA_TOTAL];
};

/**
 * struct coul_battery_data -
 * @id_voltage_min: the min voltage on ID pin of this battery (mV)
 * @id_voltage_max: the max voltage on ID pin of this battery (mV)
 * @fcc:		full charge capacity (mAmpHour)
 * @fcc_temp_lut:	table to get fcc at a given temp
 * @pc_temp_ocv_lut:	table to get percent charge given batt temp and cycles
 * @pc_sf_lut:		table to get percent charge scaling factor given cycles
 *			and percent charge
 * @rbatt_sf_lut:	table to get battery resistance scaling factor given
 *			temperature and percent charge
 * @default_rbatt_mohm:	the default value of battery resistance to use when
 *			readings from bms are not available.
 * @delta_rbatt_mohm:	the resistance to be added towards lower soc to
 *			compensate for battery capacitance.
 */

struct coul_battery_data {
	char *identify_type;
	char *id_sn;
	unsigned int id_voltage_min;
	unsigned int id_voltage_max;
	unsigned int id_identify_min;
	unsigned int id_identify_max;
	unsigned int fcc;
	struct single_row_lut *fcc_temp_lut;
	struct single_row_lut *fcc_sf_lut;
	struct pc_temp_ocv_lut *pc_temp_ocv_lut; /* temp pointer */
	struct pc_temp_ocv_lut *pc_temp_ocv_lut0; /* new battery */
	struct pc_temp_ocv_lut *pc_temp_ocv_lut1; /* battery age level1 */
	struct pc_temp_ocv_lut *pc_temp_ocv_lut2; /* battery age level2 */
	struct pc_temp_ocv_lut *pc_temp_ocv_lut3; /* battery age level3 */
	unsigned int vol_dec1; /* voltage reduce */
	unsigned int vol_dec2; /* voltage reduce */
	unsigned int vol_dec3; /* voltage reduce */
	struct sf_lut *pc_sf_lut;
	struct sf_lut *rbatt_sf_lut;
	int default_rbatt_mohm;
	int delta_rbatt_mohm;
	unsigned int vbatt_max;
	int ifull;
	struct chrg_para_lut *chrg_para;
	char *batt_brand;
	unsigned int id_status;
};
struct coul_battery_data *get_battery_data(unsigned int id, unsigned int indentify_fcc);
#endif
