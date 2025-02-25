/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 */

#ifndef HVGR_DATAN_H
#define HVGR_DATAN_H

#include <linux/list.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>

#ifdef HVGR_DATAN_JOB_TIMELINE
#include "jobtimeline/hvgr_job_timeline.h"

struct hvgr_datan_timline_buf;
#endif

enum hvgr_datan_ct_state {
	HVGR_DATAN_CT_STATE_CLOSED = 0,
	HVGR_DATAN_CT_STATE_IDLE,
	HVGR_DATAN_CT_STATE_DUMPING,
	HVGR_DATAN_CT_STATE_FAULT
};

struct hvgr_datan_ct_dev {
	/*
	 * The ct_lock for operation perf counter register
	 */
	spinlock_t ct_lock;
	struct hvgr_ctx *ctx;
	u64 dump_addr;
	bool int_done;
	wait_queue_head_t wait;
	int triggered;
	bool ct_config;
	bool ct_closed;
	enum hvgr_datan_ct_state state;
};

#ifdef CONFIG_HVGR_DFX_DEBUG_BUS_DUMP
#include "hvgr_regmap_debug_bus.h"

/*
 * @brief debug bus struct
 */
struct hvgr_debugbus_dev {
	/* Debugbus workqueue */
	struct workqueue_struct *gpu_debugbus_wait_wq;
	/* Debugbus work struct */
	struct work_struct gpu_debugbus_wait_work;
	/* Debugbus data space */
	char *debugbus_buf;
	int32_t debugbus_cnt;

	struct mutex debug_bus_mutex;
	spinlock_t hwaccess_lock;
	struct hvgr_datan_dev *global_datan_dev;
};
#endif

#ifdef CONFIG_HVGR_DFX_DATAN
#define DATAN_PATH_MAX_LEN (256)

struct datan_jobtrace_ctx {
	struct mutex lock;
	/* Enable or Disable JobTrace */
	bool flag;
	char outpath[DATAN_PATH_MAX_LEN];
	int32_t softq_id;
	int32_t entry_size;
	atomic_t counter;
};
#endif

struct hvgr_datan_dev {
#ifdef CONFIG_HVGR_DFX_DEBUG_BUS_DUMP
	struct hvgr_debugbus_dev datan_bus_dev;
#endif
	struct hvgr_datan_ct_dev datan_ct_dev;
	struct mutex dump_dir_lock;
#ifdef HVGR_DATAN_JOB_TIMELINE
	struct hvgr_datan_timline_buf timeline_buf;
#endif
};

struct hvgr_datan_ctx {
#ifdef CONFIG_HVGR_DFX_DATAN
	struct datan_jobtrace_ctx jobtrace_ctx;
#else
	int pad;
#endif
};

#endif /* HVGR_DATAN_H END */

