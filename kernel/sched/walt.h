/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __WALT_H
#define __WALT_H

struct rq;
struct rq_flags;

enum {
	TASK_DC,
	CPU_DC,
	DISCOUNT_MAX_NR,
};

#ifdef CONFIG_SCHED_WALT

#define WINDOW_STATS_RECENT		0
#define WINDOW_STATS_MAX		1
#define WINDOW_STATS_MAX_RECENT_AVG	2
#define WINDOW_STATS_AVG		3
#define WINDOW_STATS_INVALID_POLICY	4

#ifdef CONFIG_SCHED_WALT_DYNAMIC_WINDOW_SIZE
#ifdef CONFIG_HZ_300
/*
 * Tick interval becomes to 3333333 due to
 * rounding error when HZ=300.
 */
#define DEFAULT_SCHED_RAVG_WINDOW (3333333 * 6)
#else
/* Default window size (in ns) = 20ms */
#define DEFAULT_SCHED_RAVG_WINDOW 20000000
#endif

/* Max window size (in ns) = 1s */
#define MAX_SCHED_RAVG_WINDOW 1000000000
#define NR_WINDOWS_PER_SEC (NSEC_PER_SEC / DEFAULT_SCHED_RAVG_WINDOW)
extern unsigned int sysctl_walt_ravg_window_nr_ticks;
#endif

void walt_update_task_ravg(struct task_struct *p, struct rq *rq, int event,
		u64 wallclock, u64 irqtime);
void walt_inc_cumulative_runnable_avg(struct rq *rq, struct task_struct *p);
void walt_dec_cumulative_runnable_avg(struct rq *rq, struct task_struct *p);
void walt_fixup_cumulative_runnable_avg(struct rq *rq, struct task_struct *p,
					u64 new_task_load);

void walt_fixup_busy_time(struct task_struct *p, int new_cpu);
void walt_init_new_task_load(struct task_struct *p);
void walt_mark_task_starting(struct task_struct *p);
void walt_set_window_start(struct rq *rq, struct rq_flags *rf);
#ifdef CONFIG_CPU_ISOLATION_OPT
void walt_migrate_sync_cpu(int cpu, int new_cpu);
#else
void walt_migrate_sync_cpu(int cpu);
#endif
u64 walt_ktime_clock(void);
void walt_account_irqtime(int cpu, struct task_struct *curr, u64 delta,
			  u64 wallclock);

u64 walt_irqload(int cpu);
int walt_cpu_high_irqload(int cpu);
int walt_cpu_overload_irqload(int cpu);

void walt_try_to_wake_up(struct task_struct *p);
extern void reset_task_stats(struct task_struct *p);
void walt_reset_new_task_load(struct task_struct *p);

#define NEW_TASK_WINDOWS 5
static inline bool is_new_task(struct task_struct *p)
{
	return p->ravg.active_windows < NEW_TASK_WINDOWS;
}

extern unsigned int sysctl_sched_use_walt_cpu_util;
extern unsigned int sysctl_sched_use_walt_task_util;
extern unsigned int sysctl_sched_walt_cpu_high_irqload;
extern unsigned int sysctl_sched_walt_cpu_overload_irqload;
extern unsigned int sysctl_sched_walt_init_task_load_pct;
#ifdef CONFIG_SCHED_WALT_DYNAMIC_WINDOW_SIZE
extern void sched_window_nr_ticks_change(void);
#endif
#else /* !CONFIG_SCHED_WALT */

static inline void walt_update_task_ravg(struct task_struct *p, struct rq *rq,
		int event, u64 wallclock, u64 irqtime) { }
static inline void walt_inc_cumulative_runnable_avg(struct rq *rq, struct task_struct *p) { }
static inline void walt_dec_cumulative_runnable_avg(struct rq *rq, struct task_struct *p) { }
static inline void walt_fixup_cumulative_runnable_avg(struct rq *rq,
						      struct task_struct *p,
						      u64 new_task_load) { }
static inline void walt_fixup_busy_time(struct task_struct *p, int new_cpu) { }
static inline void walt_init_new_task_load(struct task_struct *p) { }
static inline void walt_mark_task_starting(struct task_struct *p) { }
static inline void walt_set_window_start(struct rq *rq, struct rq_flags *rf) { }
#ifdef CONFIG_CPU_ISOLATION_OPT
static inline void walt_migrate_sync_cpu(int cpu, int new_cpu) { }
#else
static inline void walt_migrate_sync_cpu(int cpu) { }
#endif
static inline u64 walt_ktime_clock(void) { return 0; }

static inline u64 walt_irqload(int cpu) { return 0; }
#define walt_cpu_high_irqload(cpu) false
#define walt_cpu_overload_irqload(cpu) false

#define walt_try_to_wake_up(a) {}

#define is_new_task(p) false

#endif /* CONFIG_SCHED_WALT */

#ifdef CONFIG_SCHED_TOP_TASK
#define DEFAULT_TOP_TASK_HIST_SIZE		RAVG_HIST_SIZE_MAX
#define DEFAULT_TOP_TASK_STATS_POLICY		WINDOW_STATS_RECENT
#define DEFAULT_TOP_TASK_STATS_EMPTY_WINDOW	false

#define NUM_TRACKED_WINDOWS	2
#define NUM_LOAD_INDICES	64
#define SCHED_LOAD_GRANULE (walt_ravg_window / NUM_LOAD_INDICES)
#define ZERO_LOAD_INDEX (NUM_LOAD_INDICES - 1)

unsigned long top_task_util(struct rq *rq);
void top_task_exit(struct task_struct *p, struct rq *rq);
void top_task_rq_init(struct rq *rq);

struct top_task_entry {
	u8 count;
	u8 preferidle_count;
};
#else /* !CONFIG_SCHED_TOP_TASK */
static inline unsigned long top_task_util(struct rq *rq) { return 0; }
static inline void top_task_exit(struct task_struct *p, struct rq *rq) {}
#endif /* CONFIG_SCHED_TOP_TASK */

#ifdef CONFIG_SCHED_WALT_WINDOW_SIZE_TUNABLE
extern unsigned int walt_ravg_window;
extern bool walt_disabled;
#else
extern const unsigned int walt_ravg_window;
extern const bool walt_disabled;
#endif

#ifdef CONFIG_SCHED_PRED_LOAD
bool use_pred_load(int cpu);
unsigned long predict_util(struct rq *rq);
unsigned long task_pred_util(struct task_struct *p);
unsigned long max_pred_ls(struct rq *rq);
unsigned long cpu_util_pred_min(struct rq *rq);
extern unsigned int predl_jump_load;
extern unsigned int predl_do_predict;
extern unsigned int predl_window_size;
extern unsigned int predl_enable;
#else
static inline bool use_pred_load(int cpu) { return false; }
static inline unsigned long predict_util(struct rq *rq) { return 0; }
static inline unsigned long task_pred_util(struct task_struct *p) { return 0; }
static inline unsigned long max_pred_ls(struct rq *rq) { return 0; }
static inline unsigned long cpu_util_pred_min(struct rq *rq) { return 0; }
#endif

#if defined(CONFIG_CFS_BANDWIDTH) && defined(CONFIG_SCHED_WALT)
void walt_inc_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p);
void walt_dec_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p);
#else
static inline void walt_inc_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p) { }
static inline void walt_dec_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p) { }
#endif

#ifdef CONFIG_MIGRATION_NOTIFY
#define DEFAULT_FREQ_INC_NOTIFY (200 * 1000)
#define DEFAULT_FREQ_DEC_NOTIFY (200 * 1000)
#endif

#endif
