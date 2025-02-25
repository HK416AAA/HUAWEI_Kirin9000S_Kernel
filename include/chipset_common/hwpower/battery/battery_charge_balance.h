/* SPDX-License-Identifier: GPL-2.0 */
/*
 * battery_charge_balance.h
 *
 * huawei battery charge balance driver interface
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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

#ifndef _BATTERY_CHARGE_BALANCE_H_
#define _BATTERY_CHARGE_BALANCE_H_

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>

#define BAT_MAIN                0
#define BAT_AUX                 1
#define BAT_BALANCE_COUNT       2
#define BAL_TEMP_TAB_LEN        10
#define BAL_CCCV_TAB_LEN        10
#define BAL_UNBAL_TAB_LEN       2

enum battery_connect_mode {
	BAT_PARALLEL_MODE = 0,
	BAT_SERIAL_MODE,
};

struct bat_cccv_node {
	int vol;
	int cur;
};

struct bat_chg_balance_info {
	int temp;
	int vol;
	int cur;
};

struct bat_chg_balance_cur {
	int total_cur;
	struct bat_cccv_node cccv[BAT_BALANCE_COUNT];
};

struct bat_temp_cccv {
	int temp;
	int len;
	struct bat_cccv_node cccv_tab[BAL_CCCV_TAB_LEN];
};

struct bat_param {
	u32 weight;
	int len;
	struct bat_temp_cccv temp_tab[BAL_TEMP_TAB_LEN];
};

struct bat_chg_bal_device {
	struct device *dev;
	struct bat_param param_tab[BAT_BALANCE_COUNT];
	int unbalance_th[BAL_UNBAL_TAB_LEN];
	int req_cur[BAT_BALANCE_COUNT];
	int bal_cur;
	int ratio_err_cnt;
	int detect_cycle;
};

#ifdef CONFIG_HUAWEI_BATTERY_CHARGE_BALANCE
int bat_chg_balance_get_cur_info(struct bat_chg_balance_info *info,
	u32 info_len, struct bat_chg_balance_cur *result, int mode);
int bat_chg_bal_parallel_get_cur_info(struct bat_chg_bal_device *di,
	struct bat_chg_balance_info *info, u32 info_len,
	struct bat_cccv_node *cccv_node, u32 node_len,
	struct bat_chg_balance_cur *result);
int bat_chg_bal_serial_get_cur_info(struct bat_chg_bal_device *di,
	struct bat_chg_balance_info *info, u32 info_len,
	struct bat_cccv_node *cccv_node, u32 node_len,
	struct bat_chg_balance_cur *result);
#else
static int bat_chg_balance_get_cur_info(struct bat_chg_balance_info *info,
	u32 info_len, struct bat_chg_balance_cur *result, int mode)
{
	return -ENODEV;
}

static int bat_chg_bal_parallel_get_cur_info(struct bat_chg_bal_device *di,
	struct bat_chg_balance_info *info, u32 info_len,
	struct bat_cccv_node *cccv_node, u32 node_len,
	struct bat_chg_balance_cur *result)
{
	return -ENODEV;
}

static int bat_chg_bal_serial_get_cur_info(struct bat_chg_bal_device *di,
	struct bat_chg_balance_info *info, u32 info_len,
	struct bat_cccv_node *cccv_node, u32 node_len,
	struct bat_chg_balance_cur *result)
{
	return -ENODEV;
}
#endif /* CONFIG_HUAWEI_BATTERY_CHARGE_BALANCE */

#endif /* _BATTERY_CHARGE_BALANCE_H_ */
