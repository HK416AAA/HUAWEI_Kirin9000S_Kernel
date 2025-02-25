/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: private interface for coul module
 *
 * This software is licensed under the terms of the GNU General Public
 * License, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifdef CONFIG_HUAWEI_BATTERY_INFORMATION
#include <huawei_platform/power/batt_info_pub.h>
#endif
#include <platform_include/basicplatform/linux/power/platform/coul/coul_event.h>
#ifdef CONFIG_BATT_EIS
#include <platform_include/basicplatform/linux/power/platform/hisi_eis.h>
#endif
#include "coul_private_interface.h"
#include "coul_temp.h"
#include <securec.h>

#ifdef CONFIG_BATT_SOH
#include <platform_include/basicplatform/linux/power/platform/soh/hisi_soh_interface.h>
#else
#define ACR_CHECK_CYCLE_S               (20 * 60)
#define ACR_MAX_BATTERY_CURRENT_MA      100
#define DCR_CHECK_CYCLE_S               (20 * 60)
#endif

#define ABNORMAL_BATT_TEMPERATURE_LOW   (-40)
#define ABNORMAL_BATT_TEMPERATURE_HIGH  80
#define INVALID_BATT_ID_VOL             (-999)
#define IAVG_TIME_2MIN                  6 /* 6 * 20s */
#define FCC_MAX_PERCENT                 135
#define CAPACITY_DENSE_AREA_3200        3200000
#define CAPACITY_DENSE_AREA_3670        3670000
#define CAPACITY_DENSE_AREA_3700        3700000
#define CAPACITY_DENSE_AREA_3720        3720000
#define CAPACITY_DENSE_AREA_3810        3810000
#define CAPACITY_DENSE_AREA_3900        3900000
#define VOL_CUR_BUF_DEPTH       3
#define DEFAULT_SCALING_FACTOR  100
#define PC_UPPER_LIMIT          1000
#define PC_LOWER_LIMIT          0
#define SOC_TENTH       10
#define SOC_TWENTY      20
#define SOC_LOWER_LIMIT 0
#define SOC_UPPER_LIMIT 100
#define SEC_PER_MIN     60
#define BASIC_YEAR      1900
#define LOW_TEMP_TENTH  50
#define LOW_SOC         30
#define HALF            2
#define QUARTER         4

#define ACR_MAX_BATTERY_CURRENT_MA      100

#define check_coul_dev_ops(_di, _name) \
	do { if ((_di)->coul_dev_ops->_name == NULL) { \
		coul_core_err("%s %s is null\n", __func__, #_name); \
		return -EINVAL;} } while (0)

static unsigned int g_pd_charge_flag;

static int __init early_parse_pdcharge_cmdline(char *p)
{
	if (p != NULL) {
		if (strncmp(p, "charger", strlen("charger")) == 0)
			g_pd_charge_flag = 1;
		else
			g_pd_charge_flag = 0;

		coul_core_info("power down charge p:%s, pd_charge_flag :%u\n",
			p, g_pd_charge_flag);
	}
	return 0;
}

early_param("androidboot.mode", early_parse_pdcharge_cmdline);

unsigned int coul_core_get_pd_charge_flag(void)
{
	return g_pd_charge_flag;
}

int coul_core_get_battery_temperature(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_err("error, di is NULL, return default temp\n");
		return DEFAULT_TEMP;
	}
	return (di->batt_temp / TENTH);
}

int coul_core_is_battery_exist(void *data)
{
	int temp;
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_err("error, di is NULL, return default exist\n");
		return 1;
	}

#ifdef CONFIG_HLTHERM_RUNTEST
	di->batt_exist = 0;
	return 0;
#endif

	temp = coul_core_get_battery_temperature(di);
	if ((temp <= ABNORMAL_BATT_TEMPERATURE_LOW) ||
		(temp >= ABNORMAL_BATT_TEMPERATURE_HIGH))
		di->batt_exist = 0;
	else
		di->batt_exist = 1;
	return di->batt_exist;
}

int coul_core_get_battery_capacity(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_err("error, di is NULL, return default exist\n");
		return 0;
	}

	return di->batt_soc;
}

void coul_core_get_battery_voltage_and_current(
	struct smartstar_coul_device *di, int *ibat_ua, int *vbat_uv)
{
	int vol[VOL_CUR_BUF_DEPTH], cur[VOL_CUR_BUF_DEPTH];
	int i;
	static int log_cnt;

	if ((di == NULL) || (di->coul_dev_ops == NULL) ||
		(ibat_ua == NULL) || (vbat_uv == NULL)) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return;
	}
	for (i = 0; i < VOL_CUR_BUF_DEPTH; i++) {
		vol[i] = di->coul_dev_ops->get_battery_voltage_uv(di->batt_index);
		cur[i] = di->coul_dev_ops->get_battery_current_ua(di->batt_index);
	}

	if ((vol[0] == vol[1]) && (cur[0] == cur[1])) {
		*ibat_ua = cur[0];
		*vbat_uv = vol[0];
	} else if ((vol[1] == vol[2]) && (cur[1] == cur[2])) {
		*ibat_ua = cur[1];
		*vbat_uv = vol[1];
	} else {
		*ibat_ua = cur[2];
		*vbat_uv = vol[2];
	}

	if (log_cnt == 200) {
		coul_core_info("%s:batt_index %d, vbat_uv %d, ibat_ua %d, r_pcb %d\n",
			__func__, di->batt_index, *vbat_uv, *ibat_ua, di->r_pcb);
		log_cnt = 0;
	}
	log_cnt++;

	*vbat_uv += (*ibat_ua / PERMILLAGE) * (di->r_pcb / PERMILLAGE);
}

int coul_core_get_battery_voltage_mv(void *data)
{
	int ibat_ua = 0;
	int vbat_uv = 0;
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return -1;
	}

	coul_core_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
	return vbat_uv / PERMILLAGE;
}

int coul_core_get_battery_current_ma(void *data)
{
	int cur;
	struct smartstar_coul_device *di = data;

	if ((di == NULL) || (di->coul_dev_ops == NULL)) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	cur = di->coul_dev_ops->get_battery_current_ua(di->batt_index);
	return cur / PERMILLAGE;
}

int coul_get_battery_unfiltered_capacity(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_soc_real / TENTH;
}

int coul_core_get_battery_rm(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_ruc / PERMILLAGE;
}

int coul_core_get_battery_fcc(void *data)
{
	struct smartstar_coul_device *di  = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_fcc / PERMILLAGE;
}

int coul_core_get_battery_uuc(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_uuc / PERMILLAGE;
}

int coul_core_get_battery_cc(void *data)
{
	int cc;
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	cc = di->coul_dev_ops->calculate_cc_uah(di->batt_index);
	return cc;
}

int coul_core_get_battery_cc_cache(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->base_info.cc_uash;
}

int coul_core_get_battery_delta_rc(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_delta_rc / PERMILLAGE;
}

int coul_core_get_battery_ocv(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->batt_ocv;
}

int coul_core_get_battery_resistance(void *data)
{
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return 0;
	}

	if (!coul_core_is_battery_exist(di))
		return 0;

	return di->rbatt;
}

int coul_core_get_chip_temp(void *data)
{
	int chip_temp;
	struct smartstar_coul_device *di = data;

	if(di == NULL)
		return -1;

	if ((di->coul_dev_ops != NULL) && (di->coul_dev_ops->get_chip_temp != NULL)) {
		chip_temp = di->coul_dev_ops->get_chip_temp();
		coul_core_info("%s:chip_temp=%d\n", __func__, chip_temp);
		return chip_temp;
	}

	coul_core_err("%s:coul_core_get_chip_temp failed", __func__);
	return -1;
}

void coul_calibration_adc(struct smartstar_coul_device *di, int temp_care)
{
	int current_sec;
	int temp;

	if ((di == NULL) || (di->coul_dev_ops == NULL) ||
		(di->coul_dev_ops->cali_adc == NULL) ||
		(di->coul_dev_ops->get_coul_time == NULL)) {
		coul_core_err("coul ops is NULL\n");
		return;
	}

	if (di->cali_adc_disable)
		return;

	current_sec = (int)di->coul_dev_ops->get_coul_time(di->batt_index);
	/* Calibration every 5 minutes */
	if((current_sec - di->g_last_cali_time > CALIBRATE_INTERVAL / PERMILLAGE) ||
		(current_sec - di->g_last_cali_time < 0))
		goto adc_cali;

	/* Temperature changes more than 2 degrees */
	if(temp_care) {
		temp = di->batt_temp;
		if(abs(temp - di->g_last_cali_temp) >= TEMP_THRESHOLD_CALI * TENTH)
			goto adc_cali;
	}

	return;

adc_cali:

	di->coul_dev_ops->cali_adc(di->batt_index);
	di->g_last_cali_time = (int)di->coul_dev_ops->get_coul_time(di->batt_index);
	di->g_last_cali_temp = di->batt_temp;
}

void coul_get_last_cali_info(struct smartstar_coul_device *di,
	int *last_cali_temp, int *last_cali_time)
{
	*last_cali_temp = di->g_last_cali_temp;
	*last_cali_time = di->g_last_cali_time;
}

/* get the average current(ma) of the coul fifo */
int coul_core_get_fifo_avg_current_ma(void *data)
{
	short i;
	int valid = 0;
	int current_ua;
	int totalcur = 0;
	int fifo_depth;
	struct smartstar_coul_device *di = data;

	if ((di == NULL) || (di->coul_dev_ops == NULL)) {
		coul_core_info("[%s]di is null\n", __func__);
		return 0;
	}

	fifo_depth = di->coul_dev_ops->get_fifo_depth();
	for (i = 0; i < fifo_depth; i++) {
		current_ua = di->coul_dev_ops->get_battery_cur_ua_from_fifo(di->batt_index, i);
		if (current_ua == 0)
			continue;
		valid++;
		totalcur += current_ua;
	}

	if (valid)
		current_ua = totalcur / valid;
	else
		current_ua = di->coul_dev_ops->get_battery_current_ua(di->batt_index);

	return (current_ua / PERMILLAGE);
}

/* judge whether value locates in zone [left, right] or [right, left] 1: yes 0: no */
int coul_is_between(int left, int right, int value)
{
	if ((left >= right) && (left >= value) && (value >= right))
		return 1;
	if ((left <= right) && (left <= value) && (value <= right))
		return 1;

	return 0;
}

/* get y according to : y = ax +b, a = (y1 - y0) / (x1 - x0) */
int coul_linear_interpolate(int y0, int x0, int y1, int x1, int x)
{
	if ((y0 == y1) || (x == x0))
		return y0;
	if ((x1 == x0) || (x == x1))
		return y1;

	return y0 + ((y1 - y0) * (x - x0) / (x1 - x0));
}

/* get y according to : y = ax +b a = (y1 - y0) / (x1 - x0)  x0 <x  and x1 > x */
int coul_interpolate_single_lut(const struct single_row_lut *lut, int x)
{
	int i, result;

	if (lut == NULL)
		return -1;

	if (x < lut->x[0])
		return lut->y[0];

	if (x > lut->x[lut->cols - 1])
		return lut->y[lut->cols - 1];

	for (i = 0; i < lut->cols; i++) {
		if (x <= lut->x[i])
			break;
	}

	if ((x == lut->x[i]) || (i == 0))
		result = lut->y[i];
	else
		result = coul_linear_interpolate(lut->y[i - 1], lut->x[i - 1],
					lut->y[i], lut->x[i], x);
	return result;
}

int coul_interpolate_single_y_lut(const struct single_row_lut *lut, int y)
{
	int i, result;

	if (lut == NULL)
		return -1;

	if (y > lut->y[0])
		return lut->x[0];

	if (y < lut->y[lut->cols - 1])
		return lut->x[lut->cols - 1];

	for (i = 0; i < lut->cols; i++) {
		if (y >= lut->y[i])
			break;
	}

	if ((y == lut->y[i]) || (i == 0))
		result = lut->x[i];
	else
		result = coul_linear_interpolate(lut->x[i - 1], lut->y[i - 1],
			lut->x[i], lut->y[i], y);
	return result;
}

static int calc_scalingfactor(const struct sf_lut *sf_lut, int row_entry,
	int pc, int row1, int row2)
{
	int i, scalefactorrow1, scalefactorrow2, scalefactor, cols;

	cols = sf_lut->cols;
	row_entry = clamp_val(row_entry, sf_lut->row_entries[0],
			      sf_lut->row_entries[cols - 1]);
	for (i = 0; i < cols; i++) {
		if (row_entry <= sf_lut->row_entries[i])
			break;
	}
	if ((row_entry == sf_lut->row_entries[i]) || (i == 0)) {
		scalefactor = coul_linear_interpolate(sf_lut->sf[row1][i],
			sf_lut->percent[row1], sf_lut->sf[row2][i],
			sf_lut->percent[row2], pc);
		return scalefactor;
	}

	scalefactorrow1 = coul_linear_interpolate(sf_lut->sf[row1][i - 1],
		sf_lut->row_entries[i - 1], sf_lut->sf[row1][i],
		sf_lut->row_entries[i], row_entry);

	scalefactorrow2 = coul_linear_interpolate(sf_lut->sf[row2][i - 1],
		sf_lut->row_entries[i - 1], sf_lut->sf[row2][i],
		sf_lut->row_entries[i], row_entry);

	scalefactor = coul_linear_interpolate(scalefactorrow1, sf_lut->percent[row1],
		scalefactorrow2, sf_lut->percent[row2], pc);

	return scalefactor;
}

/* get y according to : y = ax +b, return scalefactor of pc */
int coul_interpolate_scalingfactor(const struct sf_lut *sf_lut, int row_entry, int pc)
{
	int i, rows;
	int row1 = 0;
	int row2 = 0;

	/*
	 * sf table could be null when no battery aging data is available, in
	 * that case return 100%
	 */
	if (sf_lut == NULL)
		return DEFAULT_SCALING_FACTOR;

	if ((sf_lut->rows < 1) || (sf_lut->cols < 1))
		return DEFAULT_SCALING_FACTOR;

	rows = sf_lut->rows;
	if (pc > sf_lut->percent[0]) {
		row1 = 0;
		row2 = 0;
	}
	if (pc < sf_lut->percent[rows - 1]) {
		row1 = rows - 1;
		row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == sf_lut->percent[i]) {
			row1 = i;
			row2 = i;
			break;
		}
		if (pc > sf_lut->percent[i]) {
			if (i == 0)
				row1 = i;
			else
				row1 = i - 1;
			row2 = i;
			break;
		}
	}
	return calc_scalingfactor(sf_lut, row_entry, pc, row1, row2);
}

/* look for fcc value by battery temperature, this function is called  before fcc self_study */
int coul_interpolate_fcc(struct smartstar_coul_device *di, int batt_temp)
{
	if (di == NULL)
		return -1;
	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp / TENTH;
	return coul_interpolate_single_lut(di->batt_data->fcc_temp_lut, batt_temp);
}

/* look for fcc scaling factory value by battery charge cycles used to adjust fcc before fcc self_study */
static int coul_interpolate_scalingfactor_fcc(
	struct smartstar_coul_device *di, int cycles)
{
	/* sf table could be null when no battery aging data is available, in that case return 100% */
	if (di->batt_data->fcc_sf_lut)
		return coul_interpolate_single_lut(di->batt_data->fcc_sf_lut, cycles);

	return DEFAULT_SCALING_FACTOR;
}

/* look for fcc value by battery temperature, this function is called  after fcc self_study */
static int coul_interpolate_fcc_adjusted(struct smartstar_coul_device *di,
	int batt_temp)
{
	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp / TENTH;
	return coul_interpolate_single_lut(di->adjusted_fcc_temp_lut, batt_temp);
}

/* calculate fcc value by battery temperature and chargecycles, lookup table is different, seperated by self_study */
int coul_calculate_fcc_uah(struct smartstar_coul_device *di)
{
	int initfcc, result, scalefactor;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (di->adjusted_fcc_temp_lut == NULL) {
		initfcc = coul_interpolate_fcc(di, di->batt_temp);
		scalefactor = coul_interpolate_scalingfactor_fcc(di,
			(int)di->batt_chargecycles / PERCENT);

		/* Multiply the initial FCC value by the scale factor. */
		result = (initfcc * scalefactor * PERMILLAGE) / PERCENT;
	} else {
		return PERMILLAGE * coul_interpolate_fcc_adjusted(di, di->batt_temp);
	}

	return result;
}

static void get_pc_idx(const struct pc_temp_ocv_lut *lut, int pc,
	int *row1, int *row2)
{
	int i, rows;

	rows = lut->rows;
	if (pc > lut->percent[0] * TENTH) {
		*row1 = 0;
		*row2 = 0;
	}
	if (pc < lut->percent[rows - 1] * TENTH) {
		*row1 = rows - 1;
		*row2 = rows - 1;
	}
	for (i = 0; i < rows; i++) {
		if (pc == lut->percent[i] * TENTH) {
			*row1 = i;
			*row2 = i;
			break;
		}
		if (pc > lut->percent[i] * TENTH) {
			*row1 = i - 1;
			*row2 = i;
			break;
		}
	}
}

/* look for ocv according to temp, lookup table and pc */
int coul_interpolate_ocv(const struct pc_temp_ocv_lut *lut, int batt_temp_degc, int pc)
{
	int i, ocvrow1, ocvrow2, ocv;
	int row1 = 0;
	int row2 = 0;

	if (lut == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	if (pc > PC_UPPER_LIMIT)
		pc = PC_UPPER_LIMIT;
	else if (pc < PC_LOWER_LIMIT)
		pc = PC_LOWER_LIMIT;

	get_pc_idx(lut, pc, &row1, &row2);

	batt_temp_degc = clamp_val(batt_temp_degc, lut->temp[0],
		lut->temp[lut->cols - 1]);
	for (i = 0; i < lut->cols; i++) {
		if (batt_temp_degc <= lut->temp[i])
			break;
	}
	if ((batt_temp_degc == lut->temp[i]) || (i == 0)) {
		ocv = coul_linear_interpolate(lut->ocv[row1][i],
			lut->percent[row1] * TENTH, lut->ocv[row2][i],
			lut->percent[row2] * TENTH, pc);
		return ocv;
	}

	ocvrow1 = coul_linear_interpolate(lut->ocv[row1][i - 1], lut->temp[i - 1],
		lut->ocv[row1][i], lut->temp[i], batt_temp_degc);

	ocvrow2 = coul_linear_interpolate(lut->ocv[row2][i - 1], lut->temp[i - 1],
		lut->ocv[row2][i], lut->temp[i], batt_temp_degc);

	ocv = coul_linear_interpolate(ocvrow1, lut->percent[row1] * TENTH, ocvrow2,
		lut->percent[row2] * TENTH, pc);

	return ocv;
}

static int calc_pc_by_ocv_not_exceed_bound(
	const struct pc_temp_ocv_lut *lut, int batt_temp, int ocv, int j, int rows)
{
	int i, pcj, pcj_minus_one, pc;

	pcj_minus_one = 0;
	pcj = 0;
	for (i = 0; i < rows - 1; i++) {
		if ((pcj == 0) &&
			coul_is_between(lut->ocv[i][j], lut->ocv[i + 1][j], ocv))
			pcj = coul_linear_interpolate(
				lut->percent[i] * PERMILLAGE, lut->ocv[i][j],
				lut->percent[(int)(i + 1)] * PERMILLAGE,
				lut->ocv[i + 1][j], ocv);

		if ((pcj_minus_one == 0) &&
			coul_is_between(lut->ocv[i][j - 1], lut->ocv[i + 1][j - 1], ocv))
			pcj_minus_one = coul_linear_interpolate(
				lut->percent[i] * PERMILLAGE, lut->ocv[i][j - 1],
				lut->percent[(int)(i + 1)] * PERMILLAGE,
				lut->ocv[i + 1][j - 1], ocv);

		if (pcj && pcj_minus_one) {
			pc = coul_linear_interpolate(pcj_minus_one, lut->temp[j - 1],
				pcj, lut->temp[j], batt_temp);
			return pc;
		}
	}

	if (pcj)
		return pcj;

	if (pcj_minus_one)
		return pcj_minus_one;

	coul_core_err("%d ocv wasn't found for temp %d in the LUT returning 100%%\n",
		ocv, batt_temp);
	return PERMILLAGE * SOC_FULL;
}

static int calc_pc_between_temps(const struct pc_temp_ocv_lut *lut,
	int batt_temp, int ocv, int j)
{
	int pcj, pcj_minus_one, pc, rows;

	rows = lut->rows;
	pcj_minus_one = 0;
	pcj = 0;
	if (ocv >= lut->ocv[0][j]) {
		pcj = coul_linear_interpolate(lut->percent[0] * PERMILLAGE,
			lut->ocv[0][j], lut->percent[1] * PERMILLAGE,
			lut->ocv[1][j], ocv);
		pcj_minus_one = coul_linear_interpolate(lut->percent[0] * PERMILLAGE,
			lut->ocv[0][j - 1], lut->percent[1] * PERMILLAGE,
			lut->ocv[1][j - 1], ocv);
		if (pcj && pcj_minus_one) {
			pc = coul_linear_interpolate(pcj_minus_one, lut->temp[j - 1],
				pcj, lut->temp[j], batt_temp);
			return pc;
		}
		if (pcj)
			return pcj;

		if (pcj_minus_one)
			return pcj_minus_one;
	}

	if (ocv <= lut->ocv[rows - 1][j - 1])
		return lut->percent[rows - 1] * PERMILLAGE;

	return calc_pc_by_ocv_not_exceed_bound(lut, batt_temp, ocv, j, rows);
}

static int calc_pc_by_ocv(const struct pc_temp_ocv_lut *lut, int ocv,
	int i, int j)
{
	int pc;

	if ((ocv == lut->ocv[i][j]) || (i == 0))
		return lut->percent[i] * PERMILLAGE;
	pc = coul_linear_interpolate(lut->percent[i] * PERMILLAGE, lut->ocv[i][j],
		lut->percent[i - 1] * PERMILLAGE, lut->ocv[i - 1][j], ocv);
	return pc;
}

/* look for pc, percent of uah, may exceed PERMILLAGE * SOC_FULL */
int coul_coul_interpolate_pc_high_precision(const struct pc_temp_ocv_lut *lut,
	int batt_temp, int ocv)
{
	int i, j, pc, rows;

	/* batt_temp is in tenths of degC - convert it to degC for lookups */
	batt_temp = batt_temp / TENTH;
	if (lut == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return PERMILLAGE * SOC_FULL;
	}
	if ((lut->rows < 1) || (lut->cols < 1)) {
		coul_core_info("rows:%d, cols:%d are small in [%s]\n",
			lut->rows, lut->cols, __func__);
		return PERMILLAGE * SOC_FULL;
	}

	rows = lut->rows;
	batt_temp = clamp_val(batt_temp, lut->temp[0], lut->temp[lut->cols - 1]);
	for (j = 0; j < lut->cols; j++) {
		if (batt_temp <= lut->temp[j])
			break;
	}
	if ((batt_temp == lut->temp[j]) || (j == 0)) {
		/* found an exact match for temp in the table */
		if (ocv >= lut->ocv[0][j]) {
			pc = coul_linear_interpolate(lut->percent[1] * PERMILLAGE,
				lut->ocv[1][j], lut->percent[0] * PERMILLAGE,
				lut->ocv[0][j], ocv);
			return pc;
		}

		if (ocv <= lut->ocv[rows - 1][j])
			return lut->percent[rows - 1] * PERMILLAGE;
		for (i = 0; i < rows; i++) {
			if (ocv >= lut->ocv[i][j])
				return calc_pc_by_ocv(lut, ocv, i, j);
		}
	}

	/* batt_temp is within temperature for column j-1 and j */
	return calc_pc_between_temps(lut, batt_temp, ocv, j);
}

int coul_interpolate_pc(const struct pc_temp_ocv_lut *lut, int batt_temp, int ocv)
{
	int pc;

	if (lut == NULL) {
		coul_core_err("%s input para is null\n", __func__);
		return TENTH * SOC_FULL;
	}

	pc = coul_coul_interpolate_pc_high_precision(lut, batt_temp, ocv) / PERCENT;
	return (pc < TENTH * SOC_FULL) ? pc : TENTH * SOC_FULL;
}

/* percent of uah */
int coul_calculate_pc(struct smartstar_coul_device *di, int ocv_uv,
	int batt_temp, int chargecycles)
{
	int pc, scalefactor;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	pc = coul_interpolate_pc(di->batt_data->pc_temp_ocv_lut,
		batt_temp, ocv_uv / PERMILLAGE);

	scalefactor = coul_interpolate_scalingfactor(di->batt_data->pc_sf_lut,
		chargecycles, pc / TENTH);
	pc = (pc * scalefactor) / PERCENT;
	return pc;
}

/* calculate battery resistence */
int coul_get_rbatt(struct smartstar_coul_device *di, int soc_rbatt, int batt_temp)
{
	int rbatt, scalefactor;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return -1;
	}
	rbatt = di->batt_data->default_rbatt_mohm;
	if (di->batt_data->rbatt_sf_lut == NULL)
		return rbatt;
	/* Convert the batt_temp to DegC from deciDegC */
	batt_temp = batt_temp / TENTH;
	scalefactor = coul_interpolate_scalingfactor(di->batt_data->rbatt_sf_lut,
		batt_temp, soc_rbatt);
	rbatt = (rbatt * scalefactor) / PERCENT;

	if (coul_is_between(SOC_TWENTY, SOC_TENTH, soc_rbatt) != 0)
		rbatt += ((SOC_TWENTY - soc_rbatt) *
				di->batt_data->delta_rbatt_mohm) / TENTH;
	else if (coul_is_between(SOC_TENTH, SOC_ZERO, soc_rbatt) != 0)
		rbatt += di->batt_data->delta_rbatt_mohm;
	return rbatt;
}

int coul_basp_full_pc_by_voltage(struct smartstar_coul_device *di)
{
	int delta_ocv, batt_fcc_ocv;
	int pc = 0;

	if (di == NULL)
		return 0;

	delta_ocv = (int)di->nondc_volt_dec;
	if (delta_ocv) {
		batt_fcc_ocv = coul_interpolate_ocv(di->batt_data->pc_temp_ocv_lut0,
			di->batt_temp / TENTH, TENTH * SOC_FULL);
		pc = coul_interpolate_pc(di->batt_data->pc_temp_ocv_lut0,
			di->batt_temp, batt_fcc_ocv - delta_ocv);
		coul_core_info(BASP_TAG "batt_fcc_ocv = %d mV, delta_ocv = %dmV, pc = %d\n",
			batt_fcc_ocv, delta_ocv, pc);
	}
	return pc;
}

static int calculate_qmax_uah(struct smartstar_coul_device *di,
	int fcc_uah)
{
	int pc, max_fcc_uah;
	int qmax = fcc_uah;

	pc = coul_basp_full_pc_by_voltage(di);
	if (pc)
		qmax = (int)((s64)fcc_uah * PERMILLAGE / pc);

	max_fcc_uah = ((int)di->batt_data->fcc * UA_PER_MA / PERCENT) *
		FCC_MAX_PERCENT;
	if (qmax >= max_fcc_uah) {
		coul_core_err(BASP_TAG "qmax(%d uAh) is above max_fcc(%d uAh), use max_fcc\n",
			qmax, max_fcc_uah);
		qmax = max_fcc_uah;
	}

	coul_core_info(BASP_TAG "fcc_real = %dmAh, qmax = %dmAh\n",
		fcc_uah / UA_PER_MA, di->qmax / UA_PER_MA);
	return qmax;
}

int coul_core_get_qmax(struct smartstar_coul_device *di)
{
	int fcc_uah, index, qmax_basp, qmax_nonbasp;

	if ((di == NULL) || !di->batt_exist)
		return 0;

	index = (di->nv_info.latest_record_index - 1);
	if (!di->nv_info.fcc_check_sum_ext)
		return coul_calculate_fcc_uah(di);

	fcc_uah = di->nv_info.real_fcc_record[(int)
		(index + MAX_RECORDS_CNT) % MAX_RECORDS_CNT] * UA_PER_MA;
	qmax_basp = calculate_qmax_uah(di, fcc_uah);
	fcc_uah = coul_calculate_fcc_uah(di);
	qmax_nonbasp = calculate_qmax_uah(di, fcc_uah);
	if (qmax_basp * PERCENT > qmax_nonbasp * FCC_MAX_PERCENT)
		return qmax_nonbasp;

	return qmax_basp;
}

/* qmax(mah) */
int coul_core_get_battery_qmax(void *data)
{
	int tmp_qmax;
	struct smartstar_coul_device *di = data;

	if (di == NULL) {
		coul_core_err("[%s],di null\n", __func__);
		return 0;
	}
	if (!di->high_pre_qmax)
		tmp_qmax = coul_interpolate_fcc(di, DEFAULT_TEMP * TENTH);
	else
		tmp_qmax = di->high_pre_qmax;
	return tmp_qmax;
}

static void battery_para_verification(struct smartstar_coul_device *di,
	struct pc_temp_ocv_lut *current_pc_temp_ocv_lut, int basp_vol)
{
	int ocv1, ocv2;

	/* new battery ocv */
	ocv1 = coul_interpolate_ocv(di->batt_data->pc_temp_ocv_lut0,
		DEFAULT_TEMP, PERMILLAGE);
	/* ocv-pc data verification */
	ocv2 = coul_interpolate_ocv(current_pc_temp_ocv_lut,
		DEFAULT_TEMP, PERMILLAGE);
	if (ocv1 - ocv2 <= basp_vol)
		di->batt_data->pc_temp_ocv_lut = current_pc_temp_ocv_lut;
	coul_core_info("battery para changed ocv1 =%d,ocv2 =%d,ocv1-ocv2 =%d\n",
		       ocv1, ocv2, ocv1 - ocv2);
}

static void check_para_change(struct smartstar_coul_device *di,
	unsigned int nondc_volt_dec)
{
	if ((di->batt_data->vol_dec1 > 0) &&
		(nondc_volt_dec == di->batt_data->vol_dec1) &&
		di->batt_data->pc_temp_ocv_lut1)
		battery_para_verification(di, di->batt_data->pc_temp_ocv_lut1,
			nondc_volt_dec);
	else if ((di->batt_data->vol_dec2 > 0) &&
		(nondc_volt_dec == di->batt_data->vol_dec2) &&
		di->batt_data->pc_temp_ocv_lut2)
		battery_para_verification(di, di->batt_data->pc_temp_ocv_lut2,
			nondc_volt_dec);
	else if ((di->batt_data->vol_dec3 > 0) &&
		(nondc_volt_dec == di->batt_data->vol_dec3) &&
		di->batt_data->pc_temp_ocv_lut3)
		battery_para_verification(di, di->batt_data->pc_temp_ocv_lut3,
			nondc_volt_dec);
	else
		coul_core_info("battery age para no changed\n");
}

int coul_battery_para_changed(struct smartstar_coul_device *di)
{
	int new_ocv;
	unsigned int nondc_volt_dec;

	if ((di == NULL) || (di->batt_data == NULL)) {
		coul_core_err("di or batt_data is NULL\n");
		return -1;
	}

	nondc_volt_dec = di->nondc_volt_dec;
	if (!nondc_volt_dec)
		return 0;

	if (!di->basp_level)
		return 0;

	check_para_change(di, nondc_volt_dec);

	new_ocv = coul_interpolate_ocv(di->batt_data->pc_temp_ocv_lut,
		DEFAULT_TEMP, PERMILLAGE);
	coul_core_info("nondc_volt_dec = %u,new table max vol = %d\n",
		nondc_volt_dec, new_ocv);
	return 0;
}

void coul_force_ocv_update(struct smartstar_coul_device *di)
{
	if (di == NULL)
		return;

	di->last_ocv_level = OCV_LEVEL_7;
	di->coul_dev_ops->save_ocv_level(di->batt_index, di->last_ocv_level);
	coul_get_ocv_by_vol(di);
	coul_core_info("force ocv update: ocv %d, ocv_temp %d\n",
		di->batt_ocv, di->batt_ocv_temp);
}

/* judge if cc could update with cc dischage 5% fcc, Return TRUE or FALSE */
bool coul_could_cc_update_ocv(struct smartstar_coul_device *di)
{
	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return FALSE;
	}

	coul_core_info("soc_now = %d, stop_soc:%d, dis_percent:%d, fcc:%u\n",
		di->batt_soc_real, di->charging_stop_soc,
		di->dischg_ocv_soc, di->batt_data->fcc);
	if ((di->charging_stop_soc - di->batt_soc_real) >
		(di->dischg_ocv_soc * TENTH))
		return TRUE;
	return FALSE;
}

/* judege ocv vol whether in capacity dense area, Return TRUE or FALSE */
bool coul_is_in_capacity_dense_area(struct smartstar_coul_device *di, int ocv_uv)
{
	if (ocv_uv < CAPACITY_DENSE_AREA_3200 ||
		(ocv_uv > CAPACITY_DENSE_AREA_3670 &&
		ocv_uv < CAPACITY_DENSE_AREA_3700) ||
		(ocv_uv > CAPACITY_DENSE_AREA_3720 &&
		ocv_uv < CAPACITY_DENSE_AREA_3810))
		return TRUE;
	/* restrict [3800mv,3830mv] && (3200mv,3730mv) ocv update with cc dischage 5% fcc */
	if ((ocv_uv > CAPACITY_DENSE_AREA_3200 &&
		ocv_uv < CAPACITY_DENSE_AREA_3720) ||
		(ocv_uv >= CAPACITY_DENSE_AREA_3810 &&
		ocv_uv <= CAPACITY_DENSE_AREA_3900)) {
		if (coul_could_cc_update_ocv(di) == FALSE)
			return TRUE;
	}
	return FALSE;
}

int coul_get_timestamp(char *str, int len)
{
	struct timespec64 tv = {0};
	struct rtc_time tm = {0};

	if (str == NULL) {
		coul_core_err("%s input para is null\n", __func__);
		return -EINVAL;
	}

	ktime_get_real_ts64(&tv);
	tv.tv_sec -= (long)sys_tz.tz_minuteswest * SEC_PER_MIN;
	rtc_time64_to_tm(tv.tv_sec, &tm);

	(void)sprintf_s(str, len, "%04d-%02d-%02d %02d:%02d:%02d",
		 tm.tm_year + BASIC_YEAR, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		 tm.tm_min, tm.tm_sec);

	return strlen(str);
}

void coul_batt_notify(struct smartstar_coul_device *di,
	enum batt_coul_event event)
{
	struct ocv_update_data data;
	struct timespec64 tv = {0};
	int ret;

	if (di == NULL)
		return;

	switch (event) {
	case BATT_OCV_UPDATE:
		ktime_get_coarse_real_ts64(&tv);
		data.sample_time_sec = tv.tv_sec;
		data.ocv_volt_uv = di->batt_ocv;
		coul_drv_cal_uah_by_ocv(di->batt_ocv, &data.ocv_soc_uah);
		data.cc_value = di->iscd_full_update_cc;
		data.tbatt = di->batt_temp;
		data.pc = coul_coul_interpolate_pc_high_precision(
			di->batt_data->pc_temp_ocv_lut,
			di->batt_temp, di->batt_ocv / UVOLT_PER_MVOLT);
		data.ocv_level = di->last_ocv_level;
		data.batt_chargecycles = (int)di->batt_chargecycles;
		ret = call_coul_blocking_notifiers(BATT_OCV_UPDATE, (void *)&data);
		if ((ret != NOTIFY_DONE) && (ret != NOTIFY_OK))
			coul_core_err("ocv update notify failed, ret %d\n", ret);
		break;
	default:
		break;
	}
}

static int coul_dev_ops_check_fail_part_two(struct smartstar_coul_device *di)
{
	check_coul_dev_ops(di, irq_enable);
	check_coul_dev_ops(di, set_battery_moved_magic_num);
	check_coul_dev_ops(di, get_last_soc);
	check_coul_dev_ops(di, save_last_soc);
	check_coul_dev_ops(di, get_last_soc_flag);
	check_coul_dev_ops(di, clear_last_soc_flag);
	check_coul_dev_ops(di, get_offset_vol_mod);
	check_coul_dev_ops(di, set_offset_vol_mod);
	check_coul_dev_ops(di, irq_disable);
	check_coul_dev_ops(di, cali_auto_off);
	check_coul_dev_ops(di, save_ocv_level);
	check_coul_dev_ops(di, get_drained_battery_flag);
	check_coul_dev_ops(di, clear_drained_battery_flag);
	check_coul_dev_ops(di, get_ocv_level);
	check_coul_dev_ops(di, set_i_in_event_gate);
	check_coul_dev_ops(di, set_i_out_event_gate);
	check_coul_dev_ops(di, get_chip_temp);
	check_coul_dev_ops(di, get_bat_temp);
	check_coul_dev_ops(di, convert_regval2uv);
	check_coul_dev_ops(di, convert_regval2ua);
	check_coul_dev_ops(di, convert_regval2temp);
	check_coul_dev_ops(di, convert_uv2regval);
	check_coul_dev_ops(di, convert_regval2uah);
	check_coul_dev_ops(di, set_bootocv_sample);
	check_coul_dev_ops(di, set_dev_cali_param);
	check_coul_dev_ops(di, coul_dev_check);
	return 0;
}

static int coul_dev_ops_check_fail(struct smartstar_coul_device *di)
{
	if ((di == NULL) || (di->coul_dev_ops == NULL))
		return -EINVAL;

	check_coul_dev_ops(di, calculate_cc_uah);
	check_coul_dev_ops(di, convert_ocv_regval2ua);
	check_coul_dev_ops(di, convert_ocv_regval2uv);
	check_coul_dev_ops(di, is_battery_moved);
	check_coul_dev_ops(di, get_fifo_avg_data);
	check_coul_dev_ops(di, get_delta_rc_ignore_flag);
	check_coul_dev_ops(di, get_nv_read_flag);
	check_coul_dev_ops(di, set_nv_save_flag);
	check_coul_dev_ops(di, get_battery_current_ua);
	check_coul_dev_ops(di, get_coul_time);
	check_coul_dev_ops(di, clear_cc_register);
	check_coul_dev_ops(di, cali_adc);
	check_coul_dev_ops(di, get_battery_voltage_uv);
	check_coul_dev_ops(di, get_abs_cc);
	check_coul_dev_ops(di, get_battery_vol_uv_from_fifo);
	check_coul_dev_ops(di, exit_eco);
	check_coul_dev_ops(di, enter_eco);
	check_coul_dev_ops(di, show_key_reg);
	check_coul_dev_ops(di, get_offset_current_mod);
	check_coul_dev_ops(di, save_ocv);
	check_coul_dev_ops(di, get_ocv);
	check_coul_dev_ops(di, clear_coul_time);
	check_coul_dev_ops(di, set_low_low_int_val);
	check_coul_dev_ops(di, clear_ocv);
	check_coul_dev_ops(di, get_fcc_invalid_up_flag);
	check_coul_dev_ops(di, get_ate_a);
	check_coul_dev_ops(di, get_ate_b);
	check_coul_dev_ops(di, calculate_eco_leak_uah);
	check_coul_dev_ops(di, save_cc_uah);
	check_coul_dev_ops(di, get_fifo_depth);
	check_coul_dev_ops(di, get_battery_cur_ua_from_fifo);
	check_coul_dev_ops(di, save_ocv_temp);
	check_coul_dev_ops(di, get_ocv_temp);
	check_coul_dev_ops(di, clear_ocv_temp);
	check_coul_dev_ops(di, get_use_saved_ocv_flag);
	return coul_dev_ops_check_fail_part_two(di);
}

/* get charging done max battery current for acr start judgment */
void check_chg_done_max_avg_cur_flag(
	struct smartstar_coul_device *di)
{
	if (di == NULL)
		return;
	if (di->charging_state == CHARGING_STATE_CHARGE_DONE) {
		if (coul_core_get_fifo_avg_current_ma(di) > ACR_MAX_BATTERY_CURRENT_MA)
			di->chg_done_max_avg_cur_flag = 1;
	}
}

#ifdef CONFIG_BATT_SOH
static void acr_check(struct smartstar_coul_device *di, u32 charged_cnt)
{
	static int chg_done_acr_enter_time;
	int acr_time_inc, chg_done_now_time;

	if (charged_cnt == 0)
		chg_done_acr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);

	chg_done_now_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
	/* get acr cal period */
	acr_time_inc = chg_done_now_time - chg_done_acr_enter_time;
	/*
	* if current is more than max value,
	* acr condition check is restarted after ACR_CHECK_CYCLE_MS
	*/
	if (di->chg_done_max_avg_cur_flag) {
		/*
		* if current is more than max value,
		* acr condition check is restarted after ACR_CHECK_CYCLE_MS
		*/
		chg_done_acr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		di->chg_done_max_avg_cur_flag = 0;
		coul_core_info("acr notifier, avg cur is more than max\n");
		return;
	}

	if (acr_time_inc > ACR_CHECK_CYCLE_S) {
		chg_done_acr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		call_coul_blocking_notifiers(BATT_SOH_ACR, NULL);
		coul_core_info("acr notify success,acr_time_inc = [%d]\n",
			acr_time_inc);
	}
}
#endif

#ifdef CONFIG_BATT_SOH
static void dcr_check(struct smartstar_coul_device *di, u32 charged_cnt)
{
	static int chg_done_dcr_enter_time;
	int dcr_time_inc, chg_done_now_time;

	if (charged_cnt == 0)
		chg_done_dcr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);

	chg_done_now_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);

	/* get dcr cal period */
	dcr_time_inc = chg_done_now_time - chg_done_dcr_enter_time;

	if (di->chg_done_max_avg_cur_flag) {
		chg_done_dcr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		di->chg_done_max_avg_cur_flag = 0;
		coul_core_info("dcr notifier, avg cur is more than max\n");
		return;
	}

	if (dcr_time_inc > DCR_CHECK_CYCLE_S) {
		chg_done_dcr_enter_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		call_coul_blocking_notifiers(BATT_SOH_DCR, NULL);
		coul_core_info("dcr notify success, dcr_time_inc = %d\n",
			dcr_time_inc);
	}
}
#endif

#ifdef CONFIG_BATT_EIS
static void eis_freq_check(struct smartstar_coul_device *di,
	u32 charged_cnt, int *eis_freq_ntf_cnt)
{
	static int chg_done_eis_freq_time;
	int eis_freq_time_inc, chg_done_now_time;

	if (charged_cnt == 0)
		chg_done_eis_freq_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);

	chg_done_now_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);

	/* get eis_freq cal period */
	eis_freq_time_inc = chg_done_now_time - chg_done_eis_freq_time;

	if (di->chg_done_max_avg_cur_flag) {
		chg_done_eis_freq_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		di->chg_done_max_avg_cur_flag = 0;
		coul_core_info("acr/dcr/eis_freq notifier, avg cur is more than max\n");
		return;
	}

	if ((eis_freq_time_inc > EIS_FREQ_CYCLE_S) &&
		(*eis_freq_ntf_cnt <= SAME_CYC_MAX_TRIAL)) {
		chg_done_eis_freq_time = (int)(dfx_getcurtime() / NSEC_PER_SEC);
		call_coul_blocking_notifiers(BATT_EIS_FREQ, NULL);
		(*eis_freq_ntf_cnt)++;
		coul_core_info("eis freq notify success, eis_freq_time_inc = [%d]S, %dth notify\n",
			eis_freq_time_inc, *eis_freq_ntf_cnt);
	}
}
#endif

/* Remark: 1 charging done 2 Calculation period 20min  */
void coul_start_soh_check(struct smartstar_coul_device *di)
{
	static u32 charged_cnt;
	static int eis_freq_ntf_cnt;

	if (di == NULL)
		return;

	if (di->charging_state == CHARGING_STATE_CHARGE_DONE) {
#ifdef CONFIG_BATT_SOH
		acr_check(di, charged_cnt);
#endif
#ifdef CONFIG_BATT_EIS
		eis_freq_check(di, charged_cnt, &eis_freq_ntf_cnt);
#endif
#ifdef CONFIG_BATT_SOH
		dcr_check(di, charged_cnt);
#endif
		charged_cnt++;
	} else {
		charged_cnt = 0;
		eis_freq_ntf_cnt = 0;
	}
}
/* set low voltage value according low_power status, state -- normal or low power state */
void coul_core_set_low_vol_int(struct smartstar_coul_device *di, int state)
{
	int vol;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return;
	}
	di->v_low_int_value = (unsigned int)state;

	if (state == LOW_INT_STATE_RUNNING)
		vol = (int)di->v_cutoff;
	else
		vol = (int)di->v_cutoff_sleep;

	if (di->batt_temp <= LOW_INT_TEMP_THRED)
		vol = (di->v_cutoff_low_temp < vol) ? di->v_cutoff_low_temp : vol;

	di->coul_dev_ops->set_low_low_int_val(vol);
}

/* get voltage on ID pin from nv by writting in fastoot. Remark: called in module initalization */
void coul_get_battery_id_voltage(struct smartstar_coul_device *di)
{
	/* change ID get from NTC resistance by HKADC path */
	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return;
	}
	if ((int)di->boot_info.hkadc_batt_id_voltage == INVALID_BATT_ID_VOL) {
		di->batt_id_vol = 0;
		coul_core_err("%s hkadc_batt_id_voltage is INVALID\n", __func__);
	} else {
		di->batt_id_vol =
			(unsigned)(int)di->boot_info.hkadc_batt_id_voltage;
	}
	coul_core_info("%s batt_id_vol is %dmv\n", __func__, di->batt_id_vol);
}

int coul_bound_soc(int soc)
{
	soc = max(SOC_LOWER_LIMIT, soc);
	soc = min(SOC_UPPER_LIMIT, soc);
	return soc;
}

static bool is_no_clear_batt(struct smartstar_coul_device *di, const struct device_node *cfg)
{
	int i;
	int ret;
	int off;
	int array_len;
	int batt_brand_len;
	int cfg_brand_len;
	const char *cfg_brand = NULL;

	if (!cfg || !di->batt_data->batt_brand)
		return false;

	array_len = of_property_count_strings(cfg, "no_clear_batts");
	if (array_len <= 0) {
		coul_core_err("read no_clear_batts len failed\n");
		return false;
	}

	batt_brand_len = (int)strlen(di->batt_data->batt_brand);
	for (i = 0; i < array_len; i++) {
		cfg_brand = NULL;
		ret = of_property_read_string_index(cfg, "no_clear_batts",
			i, &cfg_brand);
		if (ret != 0 || !cfg_brand) {
			coul_core_err("no_clear_batts dts read failed\n");
			return false;
		}
		cfg_brand_len = (int)strlen(cfg_brand);
		if (cfg_brand_len > 0 && batt_brand_len >= cfg_brand_len) {
			off = batt_brand_len - cfg_brand_len;
			if (strncasecmp(di->batt_data->batt_brand + off, cfg_brand, cfg_brand_len) == 0) {
				coul_core_info("brand %s matched no clear batt %s\n",
					di->batt_data->batt_brand, cfg_brand);
				return true;
			}
		}
	}
	coul_core_info("not in no clear batts, clear moved data\n");
	return false;
}

void coul_clear_moved_battery_data(struct smartstar_coul_device *di, const struct device_node *cfg)
{
	int batt_sn_changed_flag = -1;

	if ((di == NULL) || (di->coul_dev_ops == NULL))
		return;
	if (!di->coul_dev_ops->is_battery_moved(di->batt_index)) {
		coul_core_info("battery not changed, chargecycles = %u\n",
			di->batt_chargecycles);
		return;
	}
	if (is_no_clear_batt(di, cfg)) {
		coul_core_info("mask battery removed\n");
		return;
	}
	di->batt_removed_flag = 1;
	coul_core_info("battery is completely pulled out and inserted once\n");

#ifdef CONFIG_HUAWEI_BATTERY_INFORMATION
	batt_sn_changed_flag = check_battery_sn_changed();
#endif
	if (batt_sn_changed_flag) {
		di->batt_chargecycles = 0;
		di->batt_limit_fcc = 0;
		di->batt_report_full_fcc_real = 0;
		di->adjusted_fcc_temp_lut = NULL;
		di->is_nv_need_save = 1;
		di->coul_dev_ops->set_nv_save_flag(di->batt_index, NV_SAVE_FAIL);
		di->coul_dev_ops->clear_last_soc_flag();
		/* clear safe record fcc */
		di->nv_info.latest_record_index = 0;
		(void)memset_s(di->nv_info.real_fcc_record,
			sizeof(di->nv_info.real_fcc_record), 0,
			sizeof(di->nv_info.real_fcc_record));
		coul_core_info("battery changed, reset chargecycles\n");
	}
}

int coul_probe_check_coul_dev_ops(struct smartstar_coul_device *di)
{
	if (coul_dev_ops_check_fail(di) != 0) {
		coul_core_err("coul_dev_ops_check_fail\n");
		return -EINVAL;
	}
	return 0;
}

int coul_check_batt_data(struct smartstar_coul_device *di)
{
	if (di == NULL)
		return -1;

	/* check battery is exist */
	if (!coul_core_is_battery_exist(di)) {
		coul_core_err("%s: no battery, just register callback\n",
			__func__);
		di->batt_data = get_battery_data(di->batt_id_vol, di->batt_indentify_fcc);
		di->batt_exist = 0;
		di->batt_removed_flag = 1;
		return -EINVAL;
	}
	di->batt_exist = 1;

	/* set battery data */
	di->batt_data = get_battery_data(di->batt_id_vol, di->batt_indentify_fcc);
	if (di->batt_data == NULL) {
		coul_core_err("%s: batt ID(0x%x) is invalid\n",
			__func__, di->batt_id_vol);
		return -1;
	}
	coul_core_info("%s: batt ID is %u\n", __func__, di->batt_id_vol);
	coul_core_debug("%s: batt_brand is %s\n",
		__func__, di->batt_data->batt_brand);
	return 0;
}

int coul_get_zero_cap_vol(struct smartstar_coul_device *di)
{
	int batt_t, zero_vol;

	if (di == NULL)
		return 0;

	batt_t = di->batt_temp / TENTH;
	/* Adjust the zero voltage based on the temperature. */
	if ((batt_t < ZERO_V_ADJ_START_T) && (batt_t > ZERO_V_ADJ_END_T))
		zero_vol = ((ZERO_V_ADJ_START_T - batt_t) *
			(ZERO_V_MIN - di->uuc_zero_vol) /
			(ZERO_V_ADJ_START_T - ZERO_V_ADJ_END_T)) +
			di->uuc_zero_vol;
	else if (batt_t <= ZERO_V_ADJ_END_T)
		zero_vol = ZERO_V_MIN;
	else
		zero_vol = di->uuc_zero_vol;

	coul_core_info("%s,batt_temp = %d,zero_vol = %d\n",
		__func__, batt_t, zero_vol);
	return zero_vol;
}

void coul_calculate_iavg_ma(struct smartstar_coul_device *di, int iavg_ua)
{
	int iavg_ma = iavg_ua / PERMILLAGE;
	int i;

	if (di == NULL) {
		coul_core_info("NULL point in [%s]\n", __func__);
		return;
	}
	di->iavg0.iavg_samples[di->iavg0.iavg_index] = iavg_ma;
	di->iavg0.iavg_index = (di->iavg0.iavg_index + 1) % IAVG_SAMPLES;
	di->iavg0.iavg_num_samples++;
	if (di->iavg0.iavg_num_samples >= IAVG_SAMPLES)
		di->iavg0.iavg_num_samples = IAVG_SAMPLES;

	iavg_ma = 0;
	for (i = 0; i < di->iavg0.iavg_num_samples; i++)
		iavg_ma += di->iavg0.iavg_samples[i];

	iavg_ma = DIV_ROUND_CLOSEST(iavg_ma, di->iavg0.iavg_num_samples);

	if (di->iavg0.iavg_num_samples > IAVG_TIME_2MIN)
		di->last_iavg_ma = -iavg_ma;
}

