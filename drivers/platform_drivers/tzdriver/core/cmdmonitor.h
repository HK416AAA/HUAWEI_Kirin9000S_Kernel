/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2022. All rights reserved.
 * Description: cmdmonitor function declaration
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef CMD_MONITOR_H
#define CMD_MONITOR_H

#include "tzdebug.h"
#include "teek_ns_client.h"
#include "smc_smp.h"
#include <linux/version.h>

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
#define TASK_COMM_LEN 16
#endif

enum {
	TYPE_CRASH_TEE = 1,
	TYPE_CRASH_TA = 2,
	TYPE_KILLED_TA = 3,
};

/*
 * when cmd execute more than 25s in tee,
 * it will be terminated when CA is killed
 */
#define CMD_MAX_EXECUTE_TIME 10U
#define S_TO_MS 1000
#define S_TO_US 1000000

struct time_spec {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	struct timespec ts;
#else
	struct timespec64 ts;
#endif
};

struct cmd_monitor {
	struct list_head list;
	struct time_spec sendtime;
	struct time_spec lasttime;
	int32_t timer_index;
	int count;
	bool returned;
	bool is_reported;
	pid_t pid;
	pid_t tid;
	char pname[TASK_COMM_LEN];
	char tname[TASK_COMM_LEN];
	unsigned int lastcmdid;
	long long timetotal;
	int agent_call_count;
};

struct cmd_monitor *cmd_monitor_log(const struct tc_ns_smc_cmd *cmd);
void cmd_monitor_reset_context(void);
void cmd_monitor_logend(struct cmd_monitor *item);
void init_cmd_monitor(void);
void free_cmd_monitor(void);
void do_cmd_need_archivelog(void);
bool is_thread_reported(pid_t tid);
void tzdebug_archivelog(void);
void cmd_monitor_ta_crash(int32_t type, const uint8_t *ta_uuid, uint32_t uuid_len);
void memstat_report(void);
void tzdebug_memstat(void);
void get_time_spec(struct time_spec *time);

#endif
