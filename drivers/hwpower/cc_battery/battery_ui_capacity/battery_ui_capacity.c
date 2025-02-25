// SPDX-License-Identifier: GPL-2.0
/*
 * battery_ui_capacity.c
 *
 * huawei battery ui capacity
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

#include "battery_ui_capacity.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <chipset_common/hwpower/battery/battery_ocv.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_supply_interface.h>
#include <chipset_common/hwpower/battery/battery_capacity_public.h>
#include "../battery_model/battery_model.h"
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <huawei_platform/power/battery_voltage.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/common_module/power_delay.h>

#define HWLOG_TAG bat_ui_capacity
HWLOG_REGIST();

static int g_bat_ui_save_soc_flag;
static struct bat_ui_capacity_device *g_bat_ui_capacity_dev;
static const struct bat_ui_vbat_para g_bat_ui_fake_vbat_level[] = {
	{ 3500,  2 },
	{ 3550,  10 },
	{ 3600,  20 },
	{ 3700,  30 },
	{ 3800,  40 },
	{ 3850,  50 },
	{ 3900,  60 },
	{ 4000,  70 },
	{ 4250,  85 },
};

static int bat_ui_capacity_filter(int soc)
{
	if (soc < BUC_CAPACITY_EMPTY)
		return BUC_CAPACITY_EMPTY;
	else if (soc > BUC_CAPACITY_FULL)
		return BUC_CAPACITY_FULL;
	else
		return soc;
}

int bat_ui_capacity(void)
{
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di)
		return coul_interface_get_battery_capacity(bat_core_get_coul_type());

	return bat_fake_cap_filter(di->ui_capacity);
}

int bat_ui_raw_capacity(void)
{
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;
	int cap = coul_interface_get_battery_capacity(bat_core_get_coul_type());

	if (!di)
		return cap;

	return bat_fake_cap_filter(cap);
}

static int bat_ui_capacity_calc_from_voltage(struct bat_ui_capacity_device *di)
{
	int i;
	int vbat_size = ARRAY_SIZE(g_bat_ui_fake_vbat_level);
	int bat_volt = di->bat_volt;

	if (bat_volt <= BUC_VBAT_MIN * hw_battery_get_series_num())
		return BUC_CAPACITY_EMPTY;

	bat_volt -= di->bat_cur * BUC_CONVERT_VOLTAGE / BUC_CONVERT_CURRENT;

	if (bat_volt >= g_bat_ui_fake_vbat_level[vbat_size - 1].volt * hw_battery_get_series_num())
		return BUC_CAPACITY_FULL;

	for (i = 0; i < vbat_size; i++) {
		if (bat_volt < g_bat_ui_fake_vbat_level[i].volt * hw_battery_get_series_num())
			return g_bat_ui_fake_vbat_level[i].soc;
	}

	return BUC_DEFAULT_CAPACITY;
}

static int bat_ui_capacity_pulling_filter(struct bat_ui_capacity_device *di,
	int curr_capacity)
{
	int index;

	if (!di->bat_exist) {
		curr_capacity = bat_ui_capacity_calc_from_voltage(di);
		hwlog_info("cal curr_capacity=%d from voltage\n", curr_capacity);
		return curr_capacity;
	}

	index = di->capacity_filter_count % di->filter_len;

	di->capacity_sum -= di->capacity_filter[index];
	di->capacity_filter[index] = curr_capacity;
	di->capacity_sum += di->capacity_filter[index];

	if (++di->capacity_filter_count >= di->filter_len)
		di->capacity_filter_count = 0;

	return di->capacity_sum / di->filter_len;
}

int bat_ui_capacity_get_filter_sum(int base)
{
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	hwlog_info("get filter sum=%d\n", di->capacity_sum);

	return (di->capacity_sum * base) / di->filter_len;
}

static void bat_ui_capacity_reset_capacity_fifo(struct bat_ui_capacity_device *di,
	int curr_capacity)
{
	unsigned int i;

	if (curr_capacity < 0)
		return;

	di->capacity_sum = 0;

	for (i = 0; i < di->filter_len; i++) {
		di->capacity_filter[i] = curr_capacity;
		di->capacity_sum += curr_capacity;
	}
}

void bat_ui_capacity_sync_filter(int rep_soc, int round_soc, int base)
{
	int new_soc;
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di) {
		hwlog_err("di is null\n");
		return;
	}

	if (!base) {
		hwlog_err("base is 0\n");
		return;
	}

	/* step1: reset capacity fifo */
	bat_ui_capacity_reset_capacity_fifo(di, round_soc);
	new_soc = (rep_soc * di->filter_len / base) - round_soc * (di->filter_len - 1);
	new_soc = bat_ui_capacity_filter(new_soc);
	bat_ui_capacity_pulling_filter(di, new_soc);

	hwlog_info("sync filter prev_soc=%d,new_soc=%d,round_soc=%d\n",
		di->prev_soc, new_soc, round_soc);

	/* step2: capacity changed (example: 86% to 87%) */
	if (di->prev_soc != round_soc) {
		di->prev_soc = round_soc;
		di->ui_capacity = round_soc;
		di->ui_prev_capacity = round_soc;
		power_supply_sync_changed("Battery");
		power_supply_sync_changed("battery");
	}
}

void bat_ui_capacity_cancle_work(void)
{
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di)
		return;

	cancel_delayed_work_sync(&di->update_work);
}

void bat_ui_capacity_restart_work(void)
{
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di)
		return;

	mod_delayed_work(system_power_efficient_wq, &di->update_work,
		msecs_to_jiffies(di->interval_normal));
}

static void bat_ui_capacity_force_full_timer(struct bat_ui_capacity_device *di,
	int *curr_capacity)
{
	if (*curr_capacity > di->chg_force_full_soc_thld) {
		di->chg_force_full_count++;
		if (di->chg_force_full_count >= di->chg_force_full_wait_time) {
			hwlog_info("wait out full time curr_capacity=%d\n", *curr_capacity);
			di->chg_force_full_count = di->chg_force_full_wait_time;
			*curr_capacity = BUC_CAPACITY_FULL;
		}
	} else {
		di->chg_force_full_count = 0;
	}
}

static int bat_ui_capacity_correct(struct bat_ui_capacity_device *di,
	int curr_cap)
{
	int i;

	for (i = 0; i < BUC_SOC_CALIBRATION_PARA_LEVEL; i++) {
		if ((curr_cap < di->vth_soc_calibration_data[i].soc) &&
			(di->bat_volt >= di->vth_soc_calibration_data[i].volt)) {
			hwlog_info("correct capacity: bat_vol=%d,cap=%d,lock_cap=%d\n",
				di->bat_volt, curr_cap, di->vth_soc_calibration_data[i].soc);
			return di->vth_soc_calibration_data[i].soc;
		}
	}
	return curr_cap;
}

static int bat_ui_capacity_vth_correct_soc(struct bat_ui_capacity_device *di,
	int curr_capacity)
{
	if (!di->vth_correct_en)
		return curr_capacity;

	if ((di->charge_status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		(di->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING))
		curr_capacity = bat_ui_capacity_correct(di, curr_capacity);

	return curr_capacity;
}

void bat_ui_capacity_set_work_interval(struct bat_ui_capacity_device *di)
{
	if (!di->monitoring_interval ||
		(di->charge_status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		(di->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING))
		di->monitoring_interval = di->interval_normal;

	if ((di->bat_temp < BUC_LOWTEMP_THRESHOLD) &&
		(di->monitoring_interval > di->interval_lowtemp))
		di->monitoring_interval = di->interval_lowtemp;

	if (di->charge_status == POWER_SUPPLY_STATUS_CHARGING)
		di->monitoring_interval = di->interval_charging;
}

static int bat_ui_capacity_handle(struct bat_ui_capacity_device *di,
	int *curr_capacity)
{
	int cap;

	if (!di->bat_exist) {
		*curr_capacity = bat_ui_capacity_calc_from_voltage(di);
		hwlog_info("use demon capacity=%d from voltage\n", *curr_capacity);
	} else {
		cap = coul_interface_get_battery_capacity(bat_core_get_coul_type());
		/* uisoc = soc/soc_at_term - ui_cap_zero_offset */
		if (di->soc_at_term)
			*curr_capacity = DIV_ROUND_CLOSEST((cap * BUC_CAPACITY_AMPLIFY),
				di->soc_at_term) - di->ui_cap_zero_offset;
		else
			*curr_capacity = cap;

		if (*curr_capacity > BUC_CAPACITY_FULL)
			*curr_capacity = BUC_CAPACITY_FULL;
		if (*curr_capacity < 0)
			*curr_capacity = 0;
		hwlog_info("capacity=%d from coul %d, zoom:%d\n",
			*curr_capacity, cap, di->soc_at_term);
	}

	if (!di->bat_exist && power_cmdline_is_factory_mode() &&
		(*curr_capacity < BUC_LOW_CAPACITY)) {
		di->ui_capacity = BUC_LOW_CAPACITY;
		di->ui_prev_capacity = BUC_LOW_CAPACITY;
		hwlog_info("battery not exist in factory mode, force low capacity to %d\n",
			di->ui_capacity);
		return -EPERM;
	}

	if (di->ui_capacity == -1) {
		di->ui_capacity = *curr_capacity;
		di->ui_prev_capacity = *curr_capacity;
		hwlog_info("first init ui_capacity=%d\n", di->ui_capacity);
		return -EPERM;
	}

	if (abs(di->ui_prev_capacity - *curr_capacity) >= BUC_SOC_JUMP_THRESHOLD)
		hwlog_info("prev_capacity=%d, curr_capacity=%d, bat_volt=%d\n",
			di->ui_prev_capacity, *curr_capacity, di->bat_volt);

	return 0;
}

static void bat_ui_capacity_update_charge_status(struct bat_ui_capacity_device *di,
	int status)
{
	int cur_status = status;

	if ((status == POWER_SUPPLY_STATUS_CHARGING) &&
		(di->ui_capacity == BUC_CAPACITY_FULL))
		cur_status = POWER_SUPPLY_STATUS_FULL;

	di->charge_status = cur_status;
}

static int bat_ui_capacity_event_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	int status;
	struct bat_ui_capacity_device *di = g_bat_ui_capacity_dev;

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_CHARGING_START:
		status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_NE_CHARGING_STOP:
		status = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
	case POWER_NE_CHARGING_SUSPEND:
		status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	default:
		return NOTIFY_OK;
	}

	di->soc_monitor_cnt = 0;
	di->soc_monitor_flag = BUC_STATUS_START;
	hwlog_info("receive event=%lu,status=%d\n", event, status);
	mutex_lock(&di->update_lock);
	bat_ui_capacity_update_charge_status(di, status);
	mutex_unlock(&di->update_lock);

	return NOTIFY_OK;
}

static int bat_ui_capacity_charge_status_check(struct bat_ui_capacity_device *di,
	int *curr_capacity)
{
	switch (di->charge_status) {
	case POWER_SUPPLY_STATUS_CHARGING:
		bat_ui_capacity_force_full_timer(di, curr_capacity);
		break;
	case POWER_SUPPLY_STATUS_FULL:
		if (di->bat_volt >= (di->bat_max_volt - BUC_RECHG_PROTECT_VOLT_THRESHOLD)) {
			*curr_capacity = BUC_CAPACITY_FULL;
			hwlog_info("force curr_capacity=100\n");
		}
		di->chg_force_full_count = 0;
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if (di->ui_prev_capacity <= *curr_capacity) {
			hwlog_err("not charging pre=%d < curr_capacity=%d\n",
				di->ui_prev_capacity, *curr_capacity);
			return -EPERM;
		}
		di->chg_force_full_count = 0;
		break;
	default:
		hwlog_err("other charge status %d\n", di->charge_status);
		break;
	}

	return 0;
}

static int bat_ui_capacity_exception_check(struct bat_ui_capacity_device *di,
	int curr_capacity)
{
	if (di->ui_prev_capacity == curr_capacity)
		return -EPERM;

	if ((di->charge_status == POWER_SUPPLY_STATUS_DISCHARGING) ||
		(di->charge_status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		if (di->ui_prev_capacity < curr_capacity)
			return -EPERM;
	}

	if ((di->charge_status == POWER_SUPPLY_STATUS_CHARGING) &&
		((di->bat_cur >= BUC_CURRENT_THRESHOLD) ||
		power_platform_in_dc_charging_stage())) {
		if (di->ui_prev_capacity > curr_capacity)
			return -EPERM;
	}

	return 0;
}

static int bat_ui_capacity_get_shutdown_vth(struct bat_ui_capacity_device *di)
{
	int ret;
	int voltage = 0;

	ret = battery_ocv_get_ocv_by_cap(BATTERY_OCV_ONE_PERCENT_CAPACITY,
		&voltage);
	if (ret)
		return di->shutdown_vth;

	return voltage;
}

static int bat_ui_capacity_correct_shutdown_soc_by_vth(struct bat_ui_capacity_device *di, int cap)
{
	int voltage;
	int count;
	int shutdown_vth;

	if (!di->bat_exist || !di->correct_shutdown_soc_en)
		return cap;

	shutdown_vth = bat_ui_capacity_get_shutdown_vth(di);
	if ((di->charge_status != POWER_SUPPLY_STATUS_DISCHARGING) &&
		(di->charge_status != POWER_SUPPLY_STATUS_NOT_CHARGING))
		di->shutdown_flag = false;
	/* correct soc by voltage in [0,50) degree centigrade */
	if ((di->bat_temp >= 0) && (di->bat_temp < 500) &&
		(cap > di->shutdown_cap) &&
		(cap <= (di->shutdown_cap + di->shutdown_gap)) &&
		(di->bat_volt <= shutdown_vth)) {
		if (di->shutdown_flag)
			return di->shutdown_cap;

		count = 3; /* 3 times for filter detection */
		while (count > 0) {
			voltage = coul_interface_get_battery_voltage(bat_core_get_coul_type());
			if (voltage > shutdown_vth) {
				hwlog_err("correct fail: vol=%d, count=%d\n", voltage, count);
				break;
			}
			count--;
			power_msleep(DT_MSLEEP_1S, 0, NULL);
		}
		if (!count) {
			hwlog_info("correct cap from %d to %d\n", cap, di->shutdown_cap);
			di->shutdown_flag = true;
			return di->shutdown_cap;
		}
	}

	return cap;
}

static bool bat_ui_capacity_is_changed(struct bat_ui_capacity_device *di)
{
	int ret;
	int curr_capacity = 0;
	int low_temp_capacity_record;

	ret = bat_ui_capacity_handle(di, &curr_capacity);
	if (ret)
		return true;

	if ((curr_capacity < BUC_LOW_CAPACITY_THRESHOLD) &&
		(di->charge_status != POWER_SUPPLY_STATUS_CHARGING) &&
		(!di->disable_pre_vol_check) &&
		(di->bat_volt >= BUC_VBAT_MIN * hw_battery_get_series_num())) {
		hwlog_info("low capacity: battery_vol=%d,curr_capacity=%d\n",
			di->bat_volt, curr_capacity);
		return false;
	}

	curr_capacity = bat_ui_capacity_correct_shutdown_soc_by_vth(di, curr_capacity);
	ret = bat_ui_capacity_charge_status_check(di, &curr_capacity);
	if (ret)
		return false;

	curr_capacity = bat_ui_capacity_vth_correct_soc(di, curr_capacity);
	low_temp_capacity_record = curr_capacity;
	curr_capacity = bat_ui_capacity_pulling_filter(di, curr_capacity);
	if ((di->bat_temp < BUC_LOWTEMP_THRESHOLD) &&
		((curr_capacity - low_temp_capacity_record) > 1)) {
		hwlog_info("curr_capacity=%d, low_temp_capacity_record=%d\n",
			curr_capacity, low_temp_capacity_record);
		curr_capacity -= 1;
	}

	if (bat_ui_capacity_exception_check(di, curr_capacity)) {
		hwlog_err("battery ui capacity exception\n");
		return false;
	}

	di->ui_capacity = curr_capacity;
	di->ui_prev_capacity = curr_capacity;
	mutex_lock(&di->update_lock);
	bat_ui_capacity_update_charge_status(di, di->charge_status);
	mutex_unlock(&di->update_lock);

	return true;
}

static void bat_ui_capacity_update_info(struct bat_ui_capacity_device *di)
{
	int type = bat_core_get_coul_type();

	di->bat_exist = coul_interface_is_battery_exist(type);
	di->bat_volt = coul_interface_get_battery_voltage(type);
	di->bat_cur = coul_interface_get_battery_current(type);
	di->bat_temp = coul_interface_get_battery_temperature(type);
}

static bool bat_ui_check_soc_vary(int soc_changed_abs, int last_record_soc,
	bool temp_stablity, struct bat_ui_capacity_device *di)
{
	if ((soc_changed_abs >= di->soc_monitor_limit) &&
		((last_record_soc > BUC_SOC_VARY_LOW) &&
		(last_record_soc < BUC_SOC_VARY_HIGH)) &&
		(temp_stablity == TRUE))
		return TRUE;
	return FALSE;
}

static int bat_ui_battery_soc_vary_flag(bool monitor_flag, int *delta_soc,
	struct bat_ui_capacity_device *di)
{
	int ret;
	int soc_changed_abs;
	int current_record_soc;
	int soc_changed;
	static int last_record_soc;
	bool temp_stablity = FALSE;

	/* Start up or resume, refresh record soc and return invalid data */
	if (di->soc_monitor_flag != BUC_STATUS_RUNNING) {
		last_record_soc = di->ui_capacity;
		di->soc_monitor_flag = BUC_STATUS_RUNNING;
		return -EPERM;
	}

	if ((di->bat_temp > BUC_SOC_MONITOR_TEMP_MIN) &&
		(di->bat_temp < BUC_SOC_MONITOR_TEMP_MAX))
		temp_stablity = TRUE;

	if (monitor_flag) {
		current_record_soc = di->ui_capacity;
		soc_changed = current_record_soc - last_record_soc;
		if (soc_changed < 0)
			soc_changed_abs = -soc_changed;
		else
			soc_changed_abs = soc_changed;
		last_record_soc = current_record_soc;
		/* if needed, report soc error */
		if (bat_ui_check_soc_vary(soc_changed_abs,
			last_record_soc, temp_stablity, di)) {
			*delta_soc = soc_changed;
			hwlog_err("soc vary fast! soc_changed is %d\n", soc_changed);
			ret = 0;
		} else {
			ret = -EPERM;
		}
	} else {
		if (temp_stablity == TRUE)
			ret = 0;
		else
			ret = -EPERM;
	}
	return ret;
}

static void bat_ui_check_soc_vary_err(struct bat_ui_capacity_device *di)
{
#ifndef CONFIG_HLTHERM_RUNTEST
	static int data_invalid;
	int delta_soc = 0;
	int r_soc = 0;
	static char tmp_start_buf[BUC_DSM_BUF_SIZE] = { 0 };
	char tmp_end_buf[BUC_DSM_BUF_SIZE] = { 0 };
	const char *gauge_name = coul_interface_get_coul_model(bat_core_get_coul_type());

	if (!di) {
		hwlog_err("di is null\n");
		return;
	}

	data_invalid |= bat_ui_battery_soc_vary_flag(FALSE, &delta_soc, di);
	if (di->soc_monitor_cnt % (BUC_SOC_MONITOR_INTERVAL / di->monitoring_interval) == 0) {
		data_invalid |= bat_ui_battery_soc_vary_flag(TRUE, &delta_soc, di);
		if (!data_invalid) {
			r_soc = coul_interface_get_battery_capacity(bat_core_get_coul_type());
			snprintf(tmp_end_buf, BUC_DSM_BUF_SIZE,
				"end: %d,%d,%d,%d,%d delta_soc=%d,soc_monitor_limit=%d\n",
				di->ui_capacity, r_soc, di->bat_volt, di->bat_cur, di->bat_temp,
				delta_soc, di->soc_monitor_limit);
			strcat(tmp_start_buf, tmp_end_buf);
			power_dsm_report_dmd(POWER_DSM_BATTERY,
				POWER_DSM_ERROR_BATT_SOC_CHANGE_FAST, tmp_start_buf);
		}
		di->soc_monitor_cnt = 0; /* reset the counter */
		data_invalid = 0;
	}
	if (!di->soc_monitor_cnt) {
		r_soc = coul_interface_get_battery_capacity(bat_core_get_coul_type());
		memset(tmp_start_buf, 0, sizeof(tmp_start_buf));
		snprintf(tmp_start_buf, BUC_DSM_BUF_SIZE,
			"%s, soc r_soc vol cur temp start:%d,%d,%d,%d,%d ",
			gauge_name, di->ui_capacity, r_soc, di->bat_volt, di->bat_cur, di->bat_temp);
	}
	di->soc_monitor_cnt++;
#endif
}

static void bat_ui_capacity_work(struct work_struct *work)
{
	struct bat_ui_capacity_device *di = container_of(work,
		struct bat_ui_capacity_device, update_work.work);

	__pm_wakeup_event(di->wakelock, jiffies_to_msecs(HZ));
	bat_ui_capacity_update_info(di);
	hwlog_info("update: e=%d,v=%d,t=%d,c=%d,s=%d,cap=%d,pcap=%d,cnt=%d\n",
		di->bat_exist, di->bat_volt, di->bat_temp,
		di->bat_cur, di->charge_status, di->ui_capacity,
		di->ui_prev_capacity, di->chg_force_full_count);

	if (bat_ui_capacity_is_changed(di)) {
		power_supply_sync_changed("Battery");
		power_supply_sync_changed("battery");
		power_event_bnc_notify(POWER_BNT_BAT_UI_CAPACITY,
			POWER_NE_BAT_UI_CAP_CHAGNED, &di->ui_capacity);
		hwlog_info("ui capacity change to %d\n", di->ui_capacity);
	}
	bat_ui_check_soc_vary_err(di);
	power_wakeup_unlock(di->wakelock, false);

	bat_ui_capacity_set_work_interval(di);
	queue_delayed_work(system_power_efficient_wq, &di->update_work,
		msecs_to_jiffies(di->monitoring_interval));
}

static void bat_ui_capacity_data_init(struct bat_ui_capacity_device *di)
{
	di->monitoring_interval = di->interval_normal;
	di->charge_status = POWER_SUPPLY_STATUS_DISCHARGING;
	di->prev_soc = 0;
	di->capacity_filter_count = 0;
	di->disable_pre_vol_check = 0;
	di->chg_force_full_count = 0;
	di->chg_force_full_soc_thld = BUC_CHG_FORCE_FULL_SOC_THRESHOLD;
	/* the unit of BUC_CHG_FORCE_FULL_TIME is minute */
	di->chg_force_full_wait_time = BUC_CHG_FORCE_FULL_TIME * 60 * 1000 /
		di->interval_charging;
	di->wait_cnt = 0;
	di->soc_monitor_flag = BUC_STATUS_START;
}

static int bat_ui_capacity_calc_from_last(int cap_jump_th)
{
	int cur_cap = coul_interface_get_battery_capacity(bat_core_get_coul_type());
	int last_cap = coul_interface_get_battery_last_capacity(bat_core_get_coul_type());

	hwlog_info("cur_cap=%d, last_cap=%d\n", cur_cap, last_cap);

	if (cur_cap < 0)
		return 0;

	if ((last_cap < 0) || (abs(cur_cap - last_cap) >= cap_jump_th))
		return cur_cap;

	return last_cap;
}

static void bat_ui_capacity_data_sec_init(struct bat_ui_capacity_device *di)
{
	int capacity;

#ifdef CONFIG_HLTHERM_RUNTEST
	bat_ui_capacity_update_info(di);
	capacity = bat_ui_capacity_calc_from_voltage(di);
#else
	capacity = bat_ui_capacity_calc_from_last(di->cap_jump_th);
#endif /* CONFIG_HLTHERM_RUNTEST */

	di->bat_max_volt = bat_model_get_vbat_max();
	di->ui_capacity = capacity;
	di->ui_prev_capacity = capacity;
	bat_ui_capacity_reset_capacity_fifo(di, capacity);
	hwlog_info("capacity_filter = %d\n", capacity);
}

static void bat_ui_wait_ready_work(struct work_struct *work)
{
	struct bat_ui_capacity_device *di = container_of(work,
		struct bat_ui_capacity_device, wait_work.work);

	if (!coul_interface_is_coul_ready(bat_core_get_coul_type())) {
		di->wait_cnt++;
		if (di->wait_cnt < (di->wait_max_time / BUC_WAIT_INTERVAL)) {
			di->ui_capacity = coul_interface_get_battery_capacity(bat_core_get_coul_type());
			if (di->wait_cnt % BUC_WAIT_MAGIC_10 == BUC_WAIT_MAGIC_1)
				hwlog_info("waiting battery gauge ic ready\n");
			queue_delayed_work(system_power_efficient_wq,
				&di->wait_work, msecs_to_jiffies(BUC_WAIT_INTERVAL));
			return;
		} else {
			hwlog_info("wait battery gauge ic timeout\n");
		}
	}

	bat_ui_capacity_data_sec_init(di);
	queue_delayed_work(system_power_efficient_wq, &di->update_work, 0);
}

static void bat_ui_shutdown_correct_dts(struct bat_ui_capacity_device *di)
{
	struct device_node *np = di->dev->of_node;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"correct_shutdown_soc_en", &di->correct_shutdown_soc_en, 0);
	if (!di->correct_shutdown_soc_en)
		return;
	/* correct soc from shutdown_cap(1%) + shutdown_gap(2%) to shutdown_cap(1%) when voltage is below 3380 mV */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"shutdown_cap", &di->shutdown_cap, 1);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"shutdown_gap", &di->shutdown_gap, 2);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"shutdown_vth", &di->shutdown_vth, 3380);
}

static void bat_ui_vth_correct_dts(struct bat_ui_capacity_device *di)
{
	struct device_node *np = di->dev->of_node;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"vth_correct_en", &di->vth_correct_en, 0);

	if (!di->vth_correct_en)
		return;
	/* 2: element count of bat_ui_soc_calibration_para */
	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"vth_correct_para",
		(u32 *)&di->vth_soc_calibration_data[0],
		BUC_SOC_CALIBRATION_PARA_LEVEL * 2))
		di->vth_correct_en = 0;
}

static void bat_ui_interval_dts(struct bat_ui_capacity_device *di)
{
	struct device_node *np = di->dev->of_node;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"monitoring_interval_charging",
		&di->interval_charging, BUC_WORK_INTERVAL_CHARGING);
	/* 1:correct abnormal value of itv_chg */
	if (di->interval_charging < 1)
		di->interval_charging = BUC_WORK_INTERVAL_CHARGING;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"monitoring_interval_normal",
		&di->interval_normal, BUC_WORK_INTERVAL_NORMAL);
	/* 2:correct abnormal value of itv_norm */
	if (di->interval_normal < 1)
		di->interval_normal = BUC_WORK_INTERVAL_NORMAL;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"monitoring_interval_lowtemp",
		&di->interval_lowtemp, BUC_WORK_INTERVAL_LOWTEMP);
	/* 3:correct abnormal value of itv_lowtemp */
	if (di->interval_lowtemp < 1)
		di->interval_lowtemp = BUC_WORK_INTERVAL_LOWTEMP;
}

static void bat_ui_capacity_dts_init(struct bat_ui_capacity_device *di)
{
	struct device_node *np = di->dev->of_node;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"soc_at_term", &di->soc_at_term, BUC_CAPACITY_DIVISOR);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"soc_monitor_limit", &di->soc_monitor_limit, BUC_DEFAULT_SOC_MONITOR_LIMIT);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"cap_jump_th", &di->cap_jump_th, BUC_CAP_JUMP_TH);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ui_cap_zero_offset", &di->ui_cap_zero_offset, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"filter_len", &di->filter_len, BUC_WINDOW_LEN);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"wait_max_time", &di->wait_max_time, BUC_WAIT_MAX_TIME);
	if ((di->filter_len < 1) || (di->filter_len > BUC_WINDOW_LEN)) /* 1:minimum value of filter_len */
		di->filter_len = BUC_WINDOW_LEN;

	bat_ui_interval_dts(di);
	bat_ui_vth_correct_dts(di);
	bat_ui_shutdown_correct_dts(di);
}

static int bat_ui_capacity_probe(struct platform_device *pdev)
{
	int ret;
	struct bat_ui_capacity_device *di = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	g_bat_ui_capacity_dev = di;

	di->dev = &pdev->dev;
	bat_ui_capacity_dts_init(di);
	bat_ui_capacity_data_init(di);
	mutex_init(&di->update_lock);
	di->event_nb.notifier_call = bat_ui_capacity_event_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_CHARGING, &di->event_nb);
	if (ret)
		goto fail_free_mem;

	di->wakelock = power_wakeup_source_register(di->dev, "bat_ui_capacity");
	if (!di->wakelock) {
		ret = -ENOMEM;
		goto fail_free_mem;
	}

	INIT_DELAYED_WORK(&di->wait_work, bat_ui_wait_ready_work);
	INIT_DELAYED_WORK(&di->update_work, bat_ui_capacity_work);
	platform_set_drvdata(pdev, di);
	queue_delayed_work(system_power_efficient_wq, &di->wait_work, 0);

	return 0;

fail_free_mem:
	kfree(di);
	g_bat_ui_capacity_dev = NULL;
	return ret;
}

static int bat_ui_capacity_remove(struct platform_device *pdev)
{
	struct bat_ui_capacity_device *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	power_wakeup_source_unregister(di->wakelock);
	platform_set_drvdata(pdev, NULL);
	cancel_delayed_work(&di->update_work);
	power_event_bnc_unregister(POWER_BNT_CHARGING, &di->event_nb);
	mutex_destroy(&di->update_lock);
	kfree(di);
	g_bat_ui_capacity_dev = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int bat_ui_capacity_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	struct bat_ui_capacity_device *di = platform_get_drvdata(pdev);

	if (!di)
		return 0;

	cancel_delayed_work(&di->update_work);
	return 0;
}

static int bat_ui_capacity_resume(struct platform_device *pdev)
{
	struct bat_ui_capacity_device *di = platform_get_drvdata(pdev);

	if (!di)
		return 0;

	di->soc_monitor_flag = BUC_STATUS_WAKEUP;
	queue_delayed_work(system_power_efficient_wq, &di->update_work, 0);
	return 0;
}
#endif /* CONFIG_PM */

static void bat_ui_set_shutdown_capacity(void)
{
	int ret;
	int val;

	if (g_bat_ui_save_soc_flag)
		return;

	val = bat_ui_capacity();
	ret = coul_interface_set_battery_last_capacity(bat_core_get_coul_type(), val);
	if (!ret)
		g_bat_ui_save_soc_flag = 1;
}

static void bat_ui_capacity_shutdown(struct platform_device *pdev)
{
	bat_ui_set_shutdown_capacity();
}

static int bat_ui_reboot_shutdown_callback(struct notifier_block *self,
	unsigned long event, void *data)
{
	bat_ui_set_shutdown_capacity();
	return NOTIFY_OK;
}

static struct notifier_block reboot_shutdown_notifier = {
	.notifier_call = bat_ui_reboot_shutdown_callback,
};

static const struct of_device_id bat_ui_capacity_match_table[] = {
	{
		.compatible = "huawei,battery_ui_capacity",
		.data = NULL,
	},
	{},
};

static struct platform_driver bat_ui_capacity_driver = {
	.probe = bat_ui_capacity_probe,
	.remove = bat_ui_capacity_remove,
#ifdef CONFIG_PM
	.suspend = bat_ui_capacity_suspend,
	.resume = bat_ui_capacity_resume,
#endif /* CONFIG_PM */
	.shutdown = bat_ui_capacity_shutdown,
	.driver = {
		.name = "huawei,battery_ui_capacity",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bat_ui_capacity_match_table),
	},
};

static int __init bat_ui_capacity_init(void)
{
	register_reboot_notifier(&reboot_shutdown_notifier);
	return platform_driver_register(&bat_ui_capacity_driver);
}

static void __exit bat_ui_capacity_exit(void)
{
	platform_driver_unregister(&bat_ui_capacity_driver);
	unregister_reboot_notifier(&reboot_shutdown_notifier);
}

late_initcall(bat_ui_capacity_init);
module_exit(bat_ui_capacity_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("battery ui capacity driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
