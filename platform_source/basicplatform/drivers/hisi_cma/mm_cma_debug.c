/*
 * drivers/platform_drivers/hisi_cma/hisi_cma_debug.c
 *
 * Copyright(C) 2004-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "hisi_cma_debug: " fmt

#include <linux/cma.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/ftrace.h>
#include <linux/mm/mm_cma_debug.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_ap_hook.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/memcontrol.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/of_fdt.h>
#include <linux/param.h>
#include <linux/sched/debug.h>
#include <linux/sched/task.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sizes.h>
#include <linux/sysrq.h>
#ifdef CONFIG_LOG_EXCEPTION
#include <log/log_usertype.h>
#endif

#include <cma.h>
#include <internal.h>

#include "../../../kernel/workqueue_internal.h"

static int himntn_cma_trace_flag;

#define DEBUG_ON 1
#define DEBUG_OFF 0

static int cma_debug_enable = DEBUG_ON;
static DEFINE_MUTEX(cma_list_lock);
static LIST_HEAD(cma_mem_list);

#ifdef CONFIG_MM_CMA_RECORD_DEBUG
struct caller_info {
	unsigned long caller1;
	unsigned long caller2;
	unsigned long caller3;
};
#endif

struct cma_alloc_mem_info {
	unsigned long pfn;
	u64 size;
	const char *cma_name;
	char task_name[TASK_COMM_LEN];
	pid_t pid;
	struct list_head list;
#ifdef CONFIG_MM_CMA_RECORD_DEBUG
	struct caller_info *caller_info;
	ktime_t _stime;
#endif
};

#define TRACE_BUFFER_SIZE 15
#define RECORD_LOG_INFO 1
#define WORK_BUFFER_SIZE 40U
#define MAX_LOOP_TRACE_SIZE 5
#define HISI_CMA_WORK_TIMEOUT_NS   (50 * 1000 * 1000) /* 50ms */
#define MM_CMA_ALLOC_DEBUG_VERSION_MAX_TIME (10 * 1000) /* 10ms */

char *trace_log_info_maps[TAG_NUM] = {
	"undo_migrate_isolate",
	"migrate_range",
	"isolate_migrate_range",
	"recleam_clean_pages_from_list",
	"migrate_pages"
};

struct sched_record {
	u32 nivcsw;
	u32 nvcsw;
};

struct cma_work_info {
	int cpu;
	pid_t pid;
	u64 start;
	u64 delta_time;
};

struct loop_info {
	ktime_t totoal_time;
	char *log;
};

struct loop_trace_info_array {
	unsigned char loop_trace_size;
	struct loop_info info[MAX_LOOP_TRACE_SIZE];
};

struct trace_info {
	ktime_t cost_time;

	unsigned int flag;
	union {
		char *log;
		const void *func_addr;
	};

	struct sched_record sched_info;
	struct loop_trace_info_array loop_tracer;
};

struct cma_alloc_time_tracer {
	bool is_started;
	unsigned int pos;
	unsigned int work_pos;
	/*
	 * Some func maybe invoke by other task,
	 * so we should save the current's pid to charge it
	 */
	unsigned long pid;
	ktime_t trace_cost;
	struct trace_info info[TRACE_BUFFER_SIZE];
	struct cma_work_info work[WORK_BUFFER_SIZE];
};

#ifdef CONFIG_MM_CMA_TRACER_DEBUG
/* record work exec info during cma_alloc */
static struct cma_work_info drain_work[NR_CPUS];

/*
 * Due to cma_alloc in use cma_mutex to protect
 * alloc_contig_range can only invoke by one task at the same time.
 * So, we use a global record to save alloc info is suitable.
 */
struct cma_alloc_time_tracer cma_alloc_time_tracer;

static inline bool in_cma_alloc(void)
{
	return cma_alloc_time_tracer.is_started &&
	       cma_alloc_time_tracer.pid == current->pid;
}

static inline void fill_trace_cost(ktime_t once_trace)
{
	cma_alloc_time_tracer.trace_cost =
		ktime_add(cma_alloc_time_tracer.trace_cost, once_trace);
}

void cma_init_record_alloc_time(ktime_t *time)
{
	ktime_t trace_start = ktime_get();

	cma_alloc_time_tracer.trace_cost = 0;
	cma_alloc_time_tracer.pos = 0;
	cma_alloc_time_tracer.pid = current->pid;
	cma_alloc_time_tracer.work_pos = 0;
	cma_alloc_time_tracer.is_started = true;

	*time = ktime_get();

	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_stime(ktime_t *time)
{
	ktime_t trace_start;

	if (!in_cma_alloc())
		return;

	trace_start = ktime_get();
	*time = ktime_get();
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_etime_with_func(ktime_t stime, const void *func)
{
	struct trace_info *node = NULL;
	ktime_t trace_start;
	/*
	 * All info should not above the TRACE_BUFFER_SIZE - 1,
	 * so we can ensure the last record is the cma alloc total cost time
	 */
	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1)
		return;

	trace_start = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	node->cost_time = ktime_sub(ktime_get(), stime);
	node->func_addr = func;

	node->sched_info.nivcsw = current->nivcsw % 100;
	node->sched_info.nvcsw = current->nvcsw % 100;

	++cma_alloc_time_tracer.pos;
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_etime_with_log(ktime_t stime, enum LOG_TRACE_TAG flag)
{
	struct trace_info *node = NULL;
	ktime_t trace_start;
	/*
	 * All info should not above the TRACE_BUFFER_SIZE - 1,
	 * so we can ensure the last record is the cma alloc total cost time
	 */
	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1 ||
	    flag >= TAG_NUM)
		return;

	trace_start = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	node->cost_time = ktime_sub(ktime_get(), stime);
	node->flag = RECORD_LOG_INFO;

	node->sched_info.nivcsw = current->nivcsw % 100;
	node->sched_info.nvcsw = current->nvcsw % 100;

	node->log = trace_log_info_maps[flag];
	/* Rest the loop pos point, avoid print when no record */
	node->loop_tracer.loop_trace_size = 0;
	++cma_alloc_time_tracer.pos;
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_stime_with_log_loop(ktime_t *stime,
					  enum LOOP_TRACE_POS loop_trace_count)
{
	struct trace_info *node = NULL;
	struct loop_info *loop_info = NULL;
	ktime_t trace_start;
	unsigned int i = 0;

	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1)
		return;

	if (loop_trace_count > MAX_LOOP_TRACE_SIZE) {
		pr_err("invalid input loop save size, max allowed is %d\n",
		       MAX_LOOP_TRACE_SIZE);
		return;
	}

	trace_start = ktime_get();

	*stime = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	node->loop_tracer.loop_trace_size = loop_trace_count;
	loop_info = node->loop_tracer.info;
	for (; i < loop_trace_count; ++i) {
		loop_info[i].totoal_time = 0;
		loop_info[i].log = NULL;
	}
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_stime_trace_loop(ktime_t *loop_stime)
{
	ktime_t trace_start;

	if (!in_cma_alloc())
		return;

	trace_start = ktime_get();
	*loop_stime = ktime_get();
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_etime_trace_loop(ktime_t loop_stime,
				       enum LOOP_TRACE_POS pos,
				       enum LOG_TRACE_TAG flag)
{
	struct trace_info *node = NULL;
	struct loop_trace_info_array *loop_info_array = NULL;
	struct loop_info *loop_info = NULL;
	ktime_t trace_start;

	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1)
		return;

	trace_start = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	loop_info_array = &node->loop_tracer;
	if (unlikely(pos >= loop_info_array->loop_trace_size)) {
		pr_err("plz check the input pos, total allowed size=%u\n",
		       loop_info_array->loop_trace_size);
		fill_trace_cost(ktime_sub(ktime_get(), trace_start));
		return;
	}

	loop_info = &loop_info_array->info[pos];
	if (!loop_info->log)
		loop_info->log = trace_log_info_maps[flag];
	loop_info->totoal_time = ktime_add(loop_info->totoal_time,
					   ktime_sub(ktime_get(), loop_stime));
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_record_alloc_etime_with_log_loop(ktime_t stime,
					  enum LOG_TRACE_TAG flag)
{
	struct trace_info *node = NULL;
	ktime_t trace_start;
	/*
	 * All info should not above the TRACE_BUFFER_SIZE - 1,
	 * so we can ensure the last record is the cma alloc total cost time
	 */
	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1 ||
	    flag >= TAG_NUM)
		return;

	trace_start = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	node->cost_time = ktime_sub(ktime_get(), stime);
	node->flag = RECORD_LOG_INFO;

	node->sched_info.nivcsw = current->nivcsw % 100;
	node->sched_info.nvcsw = current->nvcsw % 100;

	node->log = trace_log_info_maps[flag];
	++cma_alloc_time_tracer.pos;
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_end_alloc_time_record(ktime_t stime, const void *func)
{
	struct trace_info *node = NULL;
	ktime_t trace_start;

	if (!in_cma_alloc() ||
	    cma_alloc_time_tracer.pos >= TRACE_BUFFER_SIZE - 1)
		return;

	trace_start = ktime_get();

	node = &cma_alloc_time_tracer.info[cma_alloc_time_tracer.pos];
	node->cost_time = ktime_sub(ktime_get(), stime);
	node->func_addr = func;

	node->sched_info.nivcsw = current->nivcsw % 100;
	node->sched_info.nvcsw = current->nvcsw % 100;

	++cma_alloc_time_tracer.pos;
	cma_alloc_time_tracer.is_started = false;
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_work_record_start(int cpu)
{
	struct cma_work_info *info = NULL;
	ktime_t trace_start;

	if (cpu < 0 || cpu >= NR_CPUS)
		return;

	/* not an worker or not in cma alloc */
	if (!cma_alloc_time_tracer.is_started ||
	    !(current->flags & PF_WQ_WORKER))
		return;

	trace_start = ktime_get();

	info = &drain_work[cpu];
	info->cpu = cpu;
	info->start = dfx_getcurtime();
	info->pid = current->pid;

	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

void cma_work_record_exec_time(cpumask_t *cpus)
{
	struct cma_work_info *info = NULL;
	unsigned int cur_pos;
	int cpu;
	int cpu_late = -1;
	u64 start_early = 0;
	u64 start_late = 0;
	ktime_t trace_start;

	if (!cpus || !in_cma_alloc())
		return;

	trace_start = ktime_get();
	for_each_cpu(cpu, cpus) {
		info = &drain_work[cpu];

		if (start_early == 0 || start_late == 0) {
			start_early = info->start;
			start_late = info->start;
			cpu_late = cpu;
		} else if (info->start < start_early) {
			start_early = info->start;
		} else if (info->start > start_late) {
			start_late = info->start;
			cpu_late = cpu;
		}
	}

	cur_pos = cma_alloc_time_tracer.work_pos;
	if (cur_pos >= WORK_BUFFER_SIZE || cpu_late < 0 ||
	    start_late - start_early < HISI_CMA_WORK_TIMEOUT_NS) {
		fill_trace_cost(ktime_sub(ktime_get(), trace_start));
		return;
	}

	info = &drain_work[cpu_late];
	info->delta_time = start_late - start_early;

	memcpy(&cma_alloc_time_tracer.work[cur_pos],
	       &drain_work[cpu_late],
	       sizeof(struct cma_work_info));
	++cma_alloc_time_tracer.work_pos;

	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
}

static void show_loop_record(struct trace_info *trace_info)
{
	struct loop_trace_info_array *loop_alloc_info =
		&trace_info->loop_tracer;
	struct loop_info *loop_info = loop_alloc_info->info;
	int i = 0;

	pr_info("in path (%s) all loop trace info as follow:\n",
		trace_info->log);
	for (; i < loop_alloc_info->loop_trace_size; ++i)
		pr_info("loop in %s totoal cost %lld us\n", loop_info[i].log,
			ktime_to_us(loop_info[i].totoal_time));
}

static void show_time_record_info(struct trace_info *info)
{
	if (info->flag == RECORD_LOG_INFO) {
		pr_info("(%s) cost %lld us vcsw %u ivcsw %u\n", info->log,
			ktime_to_us(info->cost_time), info->sched_info.nvcsw,
			info->sched_info.nivcsw);
		if (info->loop_tracer.loop_trace_size != 0)
			show_loop_record(info);
	} else {
		pr_info("[%ps] cost %lld us vcsw %u ivcsw %u\n",
			info->func_addr, ktime_to_us(info->cost_time),
			info->sched_info.nvcsw, info->sched_info.nivcsw);
	}
}

static void show_work_record_info(void)
{
	unsigned int i;
	struct cma_work_info *info = NULL;
	unsigned int max =
		min(cma_alloc_time_tracer.work_pos, WORK_BUFFER_SIZE);

	for (i = 0; i < max; ++i) {
		info = &cma_alloc_time_tracer.work[i];
		pr_info("CPU%d: pid %u ts %llu delta_time %llu ns\n",
			info->cpu, info->pid, info->start, info->delta_time);
		worker_recent_search(info->cpu, info->start, info->delta_time);
	}
}

static void __show_record_alloc_time_info(void)
{
	unsigned int i = 0;
	unsigned int max = cma_alloc_time_tracer.pos;
	struct trace_info *node = NULL;
	ktime_t trace_start = ktime_get();

	pr_info("================\n");
	for (; i < max; ++i) {
		node = &cma_alloc_time_tracer.info[i];
		show_time_record_info(node);
	}
	show_work_record_info();
	pr_info("================\n");
	fill_trace_cost(ktime_sub(ktime_get(), trace_start));
	pr_info("the cma trace framework total cost time=%lld us\n",
		ktime_to_us(cma_alloc_time_tracer.trace_cost));
}
#else
static void __show_record_alloc_time_info(void) { }
void cma_init_record_alloc_time(ktime_t *time) { }
void cma_record_alloc_stime(ktime_t *time) { }
void cma_record_alloc_etime_with_func(ktime_t stime, const void *func) { }
void cma_record_alloc_etime_with_log(ktime_t stime, enum LOG_TRACE_TAG flag) { }
void cma_record_alloc_stime_with_log_loop(ktime_t *stime,
	enum LOOP_TRACE_POS loop_trace_count) { }
void cma_record_alloc_stime_trace_loop(ktime_t *loop_stime) { }
void cma_record_alloc_etime_trace_loop(ktime_t loop_stime,
	enum LOOP_TRACE_POS pos, enum LOG_TRACE_TAG flag) { }
void cma_record_alloc_etime_with_log_loop(ktime_t stime,
	enum LOG_TRACE_TAG flag) { }
void cma_end_alloc_time_record(ktime_t stime, const void *func) { }
void cma_work_record_start(int cpu) { }
void cma_work_record_exec_time(cpumask_t *cpus) { }
#endif /* CONFIG_MM_CMA_TRACER_DEBUG */

#ifdef CONFIG_MM_CMA_RECORD_DEBUG

void record_cma_caller_info(void)
{
	struct cma_alloc_mem_info *pos = NULL;
	pid_t pid = task_pid_nr(current->group_leader);

	/* Due to new cma_debug_node is add into tail, so reverse search is more faster */
	mutex_lock(&cma_list_lock);
	list_for_each_entry_reverse (pos, &cma_mem_list, list) {
		if (pid == pos->pid && !pos->caller_info)
			goto get_info;
	}
	mutex_unlock(&cma_list_lock);

	return;

get_info:
	/* get the function's GrandParent, to save the caller's info */
	pos->caller_info = kzalloc(sizeof(*pos->caller_info), GFP_KERNEL);
	if (!pos->caller_info) {
		mutex_unlock(&cma_list_lock);
		return;
	}

	pos->caller_info->caller1 = CALLER_ADDR1;
	pos->caller_info->caller2 = CALLER_ADDR2;
	pos->caller_info->caller3 = CALLER_ADDR3;
	pos->_stime = ktime_get();
	mutex_unlock(&cma_list_lock);
}

static inline void __dump_caller_info(ktime_t _stime, struct caller_info *info)
{
	pr_err("run time(%lld)ms, caller trace[<%ps -> %ps -> %ps>]\n",
	       ktime_ms_delta(ktime_get(), _stime), (void *)info->caller3,
	       (void *)info->caller2, (void *)info->caller1);
}
#endif

static void __attribute__((unused)) __dump_cma_mem_info(void)
{
	struct list_head *entry = NULL;
	struct cma_alloc_mem_info *cma_alloc_info = NULL;

	if (cma_debug_enable == DEBUG_OFF)
		return;

	mutex_lock(&cma_list_lock);
	list_for_each(entry, &cma_mem_list) {
		cma_alloc_info = list_entry(entry,
			struct cma_alloc_mem_info, list);
		pr_err("%s[%s-%u](%s,pfn %#lx ,size %#llx)\n",
		       __func__, cma_alloc_info->task_name, cma_alloc_info->pid,
		       cma_alloc_info->cma_name, cma_alloc_info->pfn,
		       cma_alloc_info->size);
#ifdef CONFIG_MM_CMA_RECORD_DEBUG
		if (cma_alloc_info->caller_info)
			__dump_caller_info(cma_alloc_info->_stime,
					   cma_alloc_info->caller_info);
#endif
	}
	mutex_unlock(&cma_list_lock);
}

static void __attribute__((unused)) __record_cma_alloc_info(struct cma *cma,
		unsigned long pfn, size_t count)
{
	struct task_struct *task = NULL;
	struct cma_alloc_mem_info *cma_alloc_info = NULL;

	if (cma_debug_enable == DEBUG_OFF)
		return;

	if (!cma || !cma->count || !count || !pfn_valid(pfn))
		return;

	cma_alloc_info = kzalloc(sizeof(struct cma_alloc_mem_info), GFP_KERNEL);
	if (!cma_alloc_info) {
		pr_err("Failed to allocate cma alloc info mem\n");
		return;
	}

	task = current->group_leader;
	//lint -esym(514,*)
	get_task_comm(cma_alloc_info->task_name, task);
	//lint +esym(514,*)
	cma_alloc_info->pid = task_pid_nr(task);
	cma_alloc_info->cma_name = cma->name;
	cma_alloc_info->pfn = pfn;
	cma_alloc_info->size = count * PAGE_SIZE;

	mutex_lock(&cma_list_lock);
	list_add_tail(&cma_alloc_info->list, &cma_mem_list);
	mutex_unlock(&cma_list_lock);
}

static void __attribute__((unused)) __record_cma_release_info(struct cma *cma,
		unsigned long pfn, size_t count)
{
	struct cma_alloc_mem_info *cma_alloc_info = NULL;
	struct list_head *pos = NULL, *next = NULL;

	if (cma_debug_enable == DEBUG_OFF)
		return;

	if (!cma || !cma->count
		|| !count || !pfn_valid(pfn))
		return;

	mutex_lock(&cma_list_lock);
	if (list_empty(&cma_mem_list)) {
		mutex_unlock(&cma_list_lock);
		return;
	}

	list_for_each_safe(pos, next, &cma_mem_list) {
		cma_alloc_info = list_entry(pos,
					struct cma_alloc_mem_info, list);
		if (pfn == cma_alloc_info->pfn
			&& count == cma_alloc_info->size / PAGE_SIZE) {
			list_del(pos);
#ifdef CONFIG_MM_CMA_RECORD_DEBUG
			if (cma_alloc_info->caller_info)
				kfree(cma_alloc_info->caller_info);
#endif
			kfree(cma_alloc_info);
		}
	}
	mutex_unlock(&cma_list_lock);
}

//lint -esym(1564,*)
module_param_named(cma_debug_enable, cma_debug_enable, int, 0640);
//lint +esym(1564,*)

#ifdef CONFIG_MM_CMA_RECORD_DEBUG

void show_record_alloc_time_info(ktime_t _etime, ktime_t _stime)
{
	if (ktime_us_delta(_etime, _stime) >
		 MM_CMA_ALLOC_DEBUG_VERSION_MAX_TIME)
		__show_record_alloc_time_info();
}

void record_cma_alloc_info(struct cma *cma, unsigned long pfn, size_t count)
{
	__record_cma_alloc_info(cma, pfn, count);
}

void record_cma_release_info(struct cma *cma, unsigned long pfn, size_t count)
{
	__record_cma_release_info(cma, pfn, count);
}

void dump_cma_mem_info(void)
{
	__dump_cma_mem_info();
}
#else

/*
 * When CONFIG_MM_CMA_RECORD_DEBUG close, means the variant is user.
 * When it's beta version, we show all time info. But when file system is not ready,
 * the get_logusertype_flag() will return 0 even it's beta version,
 * so we will miss the record show which cost not above 1s
 * and before file system prepared.
 * Or else, only when total cost time above 1s, show record.
 */
void show_record_alloc_time_info(ktime_t _etime, ktime_t _stime)
{
#ifdef CONFIG_LOG_EXCEPTION
	unsigned int status = get_logusertype_flag();
	if (status == BETA_USER || status == OVERSEA_USER) {
		if(ktime_us_delta(_etime, _stime) > MM_CMA_ALLOC_DEBUG_VERSION_MAX_TIME)
		       __show_record_alloc_time_info();
		return;
	}
#endif
	if (ktime_us_delta(_etime, _stime) > HISI_CMA_ALLOC_MAX_LATENCY_US)
		__show_record_alloc_time_info();
}

void record_cma_alloc_info(struct cma *cma, unsigned long pfn, size_t count)
{
#ifdef CONFIG_LOG_EXCEPTION
	unsigned int status = get_logusertype_flag();
	if (status == BETA_USER || status == OVERSEA_USER)
		__record_cma_alloc_info(cma, pfn, count);
#endif
}

void record_cma_release_info(struct cma *cma, unsigned long pfn, size_t count)
{
#ifdef CONFIG_LOG_EXCEPTION
	unsigned int status = get_logusertype_flag();
	if (status == BETA_USER || status == OVERSEA_USER)
		__record_cma_release_info(cma, pfn, count);
#endif
}

void dump_cma_mem_info(void)
{
#ifdef CONFIG_LOG_EXCEPTION
	unsigned int status = get_logusertype_flag();
	if (status == BETA_USER || status == OVERSEA_USER)
		__dump_cma_mem_info();
#endif
}
#endif

void set_himntn_cma_trace_flag(void)
{
	himntn_cma_trace_flag = 1;
}

int get_himntn_cma_trace_flag(void)
{
	return himntn_cma_trace_flag;
}
EXPORT_SYMBOL(get_himntn_cma_trace_flag);

unsigned int cma_bitmap_used(struct cma *cma)
{
	unsigned int used;

	used = (unsigned int)bitmap_weight(cma->bitmap,
		(unsigned int)cma_bitmap_maxno(cma));

	return used << cma->order_per_bit;
}

unsigned long int cma_bitmap_maxchunk(struct cma *cma)
{
	unsigned long maxchunk = 0;
	unsigned long start;
	unsigned long end = 0;
	unsigned long bitmap_maxno = cma_bitmap_maxno(cma);

	for (;;) {
		start = find_next_zero_bit(cma->bitmap, bitmap_maxno, end);
		if (start >= cma->count)
			break;
		end = find_next_bit(cma->bitmap, bitmap_maxno, start);
		maxchunk = max(end - start, maxchunk);
	}

	return (maxchunk << cma->order_per_bit);
}

void dump_cma_page_flags(unsigned long pfn)
{
	struct page *page = NULL;
	struct mem_cgroup *memcg = NULL;

	if (!pfn_valid(pfn))
		return;

	page = pfn_to_page(pfn);
	memcg = page_memcg(page);

	pr_info("[%s] PFN %lu Block %lu Flags %#lx(%pGp)\
		page_count(page) %d page_mapcount(page) %d",
		__func__,
		pfn,
		pfn >> pageblock_order,
		page->flags,
		&page->flags,
		page_count(page),
		PageSlab(page) ? 0 : page_mapcount(page));

#ifdef CONFIG_HYPERHOLD
	if (memcg)
		pr_info("memcg_id %d, name %s",
			mem_cgroup_id(memcg),
			memcg->name);
#endif
}

void dump_cma_page(struct cma *cma, size_t count,
			unsigned long mask, unsigned long offset,
			unsigned long bitmap_maxno, unsigned long bitmap_count)
{
	unsigned int used = 0;
	unsigned long maxchunk = 0;
	unsigned long bitmap_no;
	unsigned long start = 0;
	struct page *page = NULL;
	unsigned long pfn, pfn_end;

	if (!cma || !cma->count || !count || get_himntn_cma_trace_flag())
		return;

	mutex_lock(&cma->lock);
	used = cma_bitmap_used(cma);
	maxchunk = cma_bitmap_maxchunk(cma);
	pr_info("total %lu KB mask 0x%lx used %u KB\
		maxchunk %lu KB alloc %lu KB\n",
			cma->count << (PAGE_SHIFT - 10), mask,
			used << (PAGE_SHIFT - 10),
			maxchunk << (PAGE_SHIFT - 10),
			count << (PAGE_SHIFT - 10));

	for (;;) {
		bitmap_no = bitmap_find_next_zero_area_off(cma->bitmap,
			bitmap_maxno, start, bitmap_count, mask, offset);
		if (bitmap_no >= bitmap_maxno)
			break;

		pfn = cma->base_pfn + (bitmap_no << cma->order_per_bit);
		pfn_end = pfn + count;
		while (pfn < pfn_end) {
			if (!pfn_valid_within(pfn)) {
				pfn++;
				continue;
			}
			page = pfn_to_page(pfn);
			if (PageBuddy(page)) {
				pfn += 1 << buddy_order(page);
			} else {
				dump_cma_page_flags(pfn);
				break;
			}
		}

		/* try again with a bit different memory target */
		start = bitmap_no + mask + 1;
	}
	mutex_unlock(&cma->lock);
}

#if (defined(CONFIG_MM_CMA_RECORD_DEBUG)) && (defined(CONFIG_CMA_DEBUGFS))
static int hisi_cma_info_show(struct seq_file *s, void *d)
{
	struct cma_alloc_mem_info *pos = NULL;
	ktime_t _etime = ktime_get();

	mutex_lock(&cma_list_lock);
	list_for_each_entry (pos, &cma_mem_list, list) {
		if (!pos->caller_info)
			continue;

		seq_printf(s,
			"(%s, size %#llx, run time(%lld)ms, caller trace[%ps -> %ps -> %ps])\n",
			pos->cma_name, pos->size,
			ktime_ms_delta(_etime, pos->_stime),
			(void *)(uintptr_t)pos->caller_info->caller3,
			(void *)(uintptr_t)pos->caller_info->caller2,
			(void *)(uintptr_t)pos->caller_info->caller1);
	}
	mutex_unlock(&cma_list_lock);

	return 0;
}

static int hisi_cma_detect_alloc_open(struct inode *inode, struct file *file)
{
	return single_open(file, hisi_cma_info_show, inode->i_private);
}

static const struct file_operations hisi_cma_debug_detect_fops = {
	.open = hisi_cma_detect_alloc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void hisi_cma_debugfs_init(void)
{
	struct dentry *debug_file = NULL;
	struct dentry *d = NULL;

	d = debugfs_create_dir("hisi_cma", NULL);
	if (IS_ERR_OR_NULL(d)) {
		pr_err("failed to create the hisi cma debug dir\n");
		return;
	}

	debug_file = debugfs_create_file("cma_info", 0440, d, NULL,
					 &hisi_cma_debug_detect_fops);
	if (IS_ERR_OR_NULL(debug_file)) {
		pr_err("failed to create detect debug file in dir\n");
		return;
	}
}
#endif
