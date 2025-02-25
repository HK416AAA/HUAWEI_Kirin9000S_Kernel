/* SPDX-License-Identifier: GPL-2.0 */
/*
 * battery_iscd.h
 *
 * driver adapter for iscd.
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#ifndef _BATTERY_ISCD_H_
#define _BATTERY_ISCD_H_

#include <chipset_common/hwpower/battery/battery_soh.h>

#define ISCD_BASIC_INFO_MAX_LEN        32
#define ISCD_OCV_UPDATE_EVENT           1
#define ISCD_START_CHARGE_EVENT         2
#define ISCD_STOP_CHARGE_EVENT          3
#define ISCD_FATAL_ISC_DMD_LINE_SIZE   42
#define ISCD_MAX_FATAL_ISC_NUM         20
#define ISCD_MAX_FATAL_ISC_DMD_NUM \
	(((ISCD_MAX_FATAL_ISC_NUM + 1) * (ISCD_FATAL_ISC_DMD_LINE_SIZE)) + 1)

struct iscd_ocv_data {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	time64_t sample_time_sec;
#else
	time_t sample_time_sec;
#endif
	int ocv_volt_uv; /* uV */
	int ocv_soc_uah; /* uAh */
	s64 cc_value; /* uAh */
	int tbatt;
	int pc;
	int ocv_level;
};

struct iscd_dmd_data {
	char buff[ISCD_MAX_FATAL_ISC_DMD_NUM];
	int err_no;
};

struct iscd_imonitor_data {
	unsigned int fcc;
	unsigned int bat_cyc; /* Battery charging cycles */
	unsigned int q_max; /* battery QMAX */
	char bat_man[ISCD_BASIC_INFO_MAX_LEN]; /* battery manufactor id */
};

#endif /* _BATTERY_ISCD_H_ */
