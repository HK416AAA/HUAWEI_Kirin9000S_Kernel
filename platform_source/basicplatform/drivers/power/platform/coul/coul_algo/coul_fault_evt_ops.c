/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: fault event handler function for coul module
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

#include "coul_ocv_ops.h"
#include "coul_private_interface.h"
#include "coul_temp.h"
#include "coul_fault_evt_ops.h"

#define LOW_INT_VOL_SLEEP_TIME  1000
#define LOW_INT_VOL_OFFSET      10 /* mv */
#define BATTERY_VOL_LOW_ERR     2900

void coul_core_lock_init(struct smartstar_coul_device *di)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	di->g_coul_lock = wakeup_source_register(NULL, "coul_wakelock");
#else
	di->g_coul_lock = wakeup_source_register("coul_wakelock");
#endif
	if (!di->g_coul_lock) {
		coul_core_err("%s wakeup source register failed\n", __func__);
		return;
	}
}

void coul_core_lock_trash(struct smartstar_coul_device *di)
{
	wakeup_source_unregister(di->g_coul_lock);
}

/* apply coul wake_lock */
static void coul_wake_lock(struct smartstar_coul_device *di)
{
	if (!di->g_coul_lock->active) {
		__pm_stay_awake(di->g_coul_lock);
		coul_core_info("coul core wake lock\n");
	}
}

/* release coul wake_lock */
static void coul_wake_unlock(struct smartstar_coul_device *di)
{
	if (di->g_coul_lock->active) {
		__pm_relax(di->g_coul_lock);
		coul_core_info("coul core wake unlock\n");
	}
}

/* get cutoff interrupt vol mv */
static int coul_get_low_int_vol_mv(struct smartstar_coul_device *di)
{
	int vol;

	if (di == NULL)
		return 0;

	if (di->v_low_int_value == LOW_INT_STATE_RUNNING)
		vol = (int)di->v_cutoff;
	else
		vol = (int)di->v_cutoff_sleep;

	if (di->batt_temp <= LOW_INT_TEMP_THRED)
		vol = (di->v_cutoff_low_temp < vol) ? di->v_cutoff_low_temp : vol;

	coul_core_info("%s batt%d, v_cut =%u, v_sleep =%u, v_low_temp = %d, temp = %d, vol = %d\n",
		__func__, di->batt_index, di->v_cutoff, di->v_cutoff_sleep,
		di->v_cutoff_low_temp, di->batt_temp, vol);
	return vol;
}

static void judge_dsm_report(struct smartstar_coul_device *di, int delta_soc,
	int ibat_ua, int vbat_uv)
{
	char buff[DSM_BUFF_SIZE_MAX] = {0};

	if ((delta_soc > ABNORMAL_DELTA_SOC) &&
		(coul_get_temperature_stably(di, USER_COUL) >
			TEMP_OCV_ALLOW_CLEAR * TENTH)) {
		coul_core_info("delta_soc=%d, mark save ocv is invalid\n",
			delta_soc);
		/*
		 * dmd report: current information
		 * -- fcc, CC, cur_vol, cur_current, cur_temp, cur_soc, delta_soc
		 */
		snprintf(buff, (size_t)DSM_BUFF_SIZE_MAX,
			 "[LOW VOL] fcc:%dmAh, CC:%dmAh, cur_vol:%dmV, cur_current:%dmA, cur_temp:%d, cur_soc:%d, delta_soc:%d",
			 di->batt_fcc / UA_PER_MA,
			 di->coul_dev_ops->calculate_cc_uah(di->batt_index) / UA_PER_MA,
			 vbat_uv / UVOLT_PER_MVOLT, -(ibat_ua / UA_PER_MA),
			 di->batt_temp, di->batt_soc, delta_soc);
#ifdef CONFIG_HUAWEI_DSM
		coul_core_dsm_report_ocv_cali_info(di, ERROR_LOW_VOL_INT, buff);
#endif
		di->coul_dev_ops->clear_ocv(di->batt_index);
		di->last_ocv_level = INVALID_SAVE_OCV_LEVEL;
		di->coul_dev_ops->save_ocv_level(di->batt_index, di->last_ocv_level);
		di->batt_ocv_valid_to_refresh_fcc = 0;
	}
}

static void handle_low_vol_int(struct smartstar_coul_device *di,
	int ibat_ua, int vbat_uv, int cut_off_vol)
{
	int delta_soc;
	/* low_vol_int count according to dts */
	int count = (int)di->low_vol_filter_cnt;
	struct blocking_notifier_head *notifier_list = NULL;

	get_notifier_list(&notifier_list);

	/* if battery vol is too low, it's false */
	if (vbat_uv / PERMILLAGE < BATTERY_VOL_LOW_ERR)
		coul_core_err("[batt%d]Battery vol too low\n", di->batt_index);

	if ((vbat_uv / PERMILLAGE - LOW_INT_VOL_OFFSET) > cut_off_vol) {
		/* false low vol int, next suspend don't cali */
		coul_core_err("[batt%d]false low_int, in sleep\n", di->batt_index);
		return;
	}
	while (count--) {
		/* sleep 1s */
		msleep(LOW_INT_VOL_SLEEP_TIME);
		coul_core_get_battery_voltage_and_current(di, &ibat_ua,
			&vbat_uv);
		coul_core_err("[batt%d] delay 1000ms and get vbat_uv is %duv\n",
			 di->batt_index, vbat_uv);
		di->coul_dev_ops->show_key_reg();
		if ((vbat_uv / PERMILLAGE - LOW_INT_VOL_OFFSET) > cut_off_vol) {
			/* false low vol int, next suspend don't cali */
			coul_core_err("fifo0 is false, it's got in 32k clk period\n");
			return;
		}
	}
	if (cut_off_vol >= BATTERY_VOL_2_PERCENT) {
		delta_soc = di->batt_soc - SOC_TWO;
		di->batt_soc = 1;
	} else {
		delta_soc = di->batt_soc;
		di->batt_soc = 0;
	}
	/* under voltage flag */
	di->batt_under_voltage_flag = 1;
	judge_dsm_report(di, delta_soc, ibat_ua, vbat_uv);
	blocking_notifier_call_chain(notifier_list, BATTERY_LOW_SHUTDOWN, NULL);
}

static void coul_low_vol_int_handle(struct smartstar_coul_device *di)
{
	int ibat_ua = 0;
	int vbat_uv = 0;
	int cut_off_vol;

	if (di == NULL) {
		coul_core_info("NULL point in %s\n", __func__);
		return;
	}

	if (di->is_board_type != BAT_BOARD_ASIC)
		return;

	cut_off_vol = coul_get_low_int_vol_mv(di);

	coul_core_get_battery_voltage_and_current(di, &ibat_ua, &vbat_uv);
	if ((vbat_uv / PERMILLAGE - LOW_INT_VOL_OFFSET) < cut_off_vol)
		coul_core_info("IRQ: batt%d low_vol_int current_vol is %d < %d, use fifo vol!\n",
			di->batt_index, vbat_uv / PERMILLAGE, cut_off_vol);

	coul_core_err("IRQ: batt%d, low_vol_int, cur_batt_soc=%d%%, cur_voltage=%dmv, cur_current=%dma, cur_temperature=%d\n",
		di->batt_index, di->batt_soc, vbat_uv / PERMILLAGE,
		-(ibat_ua / PERMILLAGE), di->batt_temp / TENTH);

	if (!di->batt_exist) {
		coul_core_err("IRQ: COUL_VBAT_LOW_INT, no battery, error\n");
		return;
	}

	if (strstr(saved_command_line, "androidboot.swtype=factory")) {
		coul_core_err("IRQ: COUL_VBAT_LOW_INT, factory_version, do nothing\n");
		return;
	}

	handle_low_vol_int(di, ibat_ua, vbat_uv, cut_off_vol);
}

/* update coulomb start value */
static void make_cc_no_overload(struct smartstar_coul_device *di)
{
	int cc;

	if (di == NULL) {
		coul_core_info("NULL point in %s\n", __func__);
		return;
	}
	cc = di->coul_dev_ops->calculate_cc_uah(di->batt_index);
	di->coul_dev_ops->save_cc_uah(di->batt_index, cc);
}

/* handler the fault events from coul driver Parameters: work:fault workqueue */
void coul_core_fault_work(struct work_struct *work)
{
	struct smartstar_coul_device *di =
		container_of(work, struct smartstar_coul_device, fault_work);

	switch (di->coul_fault) {
	case COUL_FAULT_LOW_VOL:
		coul_core_err("%s:batt %d, low vol int\n", __func__, di->batt_index);
		di->coul_dev_ops->irq_disable(di->batt_index);
		coul_wake_lock(di);
		coul_low_vol_int_handle(di);
		coul_wake_unlock(di);
		di->coul_dev_ops->irq_enable(di->batt_index);
		di->coul_fault = COUL_FAULT_NON;
		break;
	case COUL_FAULT_CL_INT:
		coul_core_err("%s:batt %d, cc capability compare int\n", __func__, di->batt_index);
		di->coul_fault = COUL_FAULT_NON;
		break;
	case COUL_FAULT_CL_IN:
		coul_core_err("%s:batt %d, cc in over 81 int\n", __func__, di->batt_index);
		di->coul_fault = COUL_FAULT_NON;
		make_cc_no_overload(di);
		break;
	case COUL_FAULT_CL_OUT:
		coul_core_err("%s:batt %d, cc out over 81 int!\n", __func__, di->batt_index);
		di->coul_fault = COUL_FAULT_NON;
		make_cc_no_overload(di);
		break;
	default:
		coul_core_err("%s:batt %d, default deal with!\n", __func__, di->batt_index);
		break;
	}
}

