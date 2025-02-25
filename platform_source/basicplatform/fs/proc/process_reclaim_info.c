/*
 * process_reclaim_info.c
 *
 * Process reclaim information
 *
 * Copyright (c) 2016-2020 Huawei Technologies Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/of_fdt.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/pagewalk.h>
#endif
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/time64.h>
#include <internal.h>

#include <linux/sched/task.h>
#include <linux/signal.h>


static struct kmem_cache *process_reclaim_result_cache;

static int __init process_reclaim_result_cache_create(void)
{
	process_reclaim_result_cache = KMEM_CACHE(reclaim_result, 0);
	return process_reclaim_result_cache == NULL;
}

struct reclaim_result *process_reclaim_result_cache_alloc(gfp_t gfp)
{
	struct reclaim_result *result = NULL;

	if (process_reclaim_result_cache)
		result = kmem_cache_alloc(process_reclaim_result_cache, gfp);
	return result;
}

void process_reclaim_result_cache_free(struct reclaim_result *result)
{
	if (process_reclaim_result_cache)
		kmem_cache_free(process_reclaim_result_cache, result);
}

int process_reclaim_result_read(struct seq_file *m, struct pid_namespace *ns,
	struct pid *pid, struct task_struct *tsk)
{
	struct reclaim_result *result = NULL;
	unsigned int nr_reclaimed = 0;
	unsigned int nr_writedblock = 0;
	s64 elapsed = 0;

	if (tsk) {
		task_lock(tsk);
		result = tsk->proc_reclaimed_result;
		if (result) {
			nr_reclaimed = result->nr_reclaimed;
			nr_writedblock = result->nr_writedblock;
			elapsed = result->elapsed_centisecs64;
			tsk->proc_reclaimed_result = NULL;
			task_unlock(tsk);
			process_reclaim_result_cache_free(result);
		} else {
			task_unlock(tsk);
		}
	}
	seq_printf(m,
		"nr_reclaimed=%u, nr_writedblock=%u, elapsed=%lld\n",
		nr_reclaimed, nr_writedblock, elapsed);
	return 0;
}

void exit_proc_reclaim(struct task_struct *tsk)
{
	if (tsk) {
		task_lock(tsk);
		if (tsk->proc_reclaimed_result) {
			struct reclaim_result *result =
			tsk->proc_reclaimed_result;

			tsk->proc_reclaimed_result = NULL;
			task_unlock(tsk);
			process_reclaim_result_cache_free(result);
		} else {
			task_unlock(tsk);
		}
	}
}

void process_reclaim_result_write(struct task_struct *task,
	unsigned int nr_reclaimed, unsigned int nr_writedblock, s64 elapsed)
{
	struct reclaim_result *result = NULL;

	task_lock(task);
	if (!task->proc_reclaimed_result) {
		task_unlock(task);
		result  = process_reclaim_result_cache_alloc(__GFP_NOWARN |
			__GFP_NORETRY | __GFP_KSWAPD_RECLAIM);

		task_lock(task);
		if (!task->proc_reclaimed_result) {
			task->proc_reclaimed_result = result;
			result = NULL;
		}
	}
	if (task->proc_reclaimed_result) {
		task->proc_reclaimed_result->elapsed_centisecs64 =
			elapsed / NSEC_PER_USEC;
		task->proc_reclaimed_result->nr_writedblock = nr_writedblock;
		task->proc_reclaimed_result->nr_reclaimed = nr_reclaimed;
	}
	task_unlock(task);
	if (result)
		process_reclaim_result_cache_free(result);
}

bool process_reclaim_need_abort(struct mm_walk *walk)
{
	struct mm_struct *mm = NULL;

	if (!walk || !walk->private ||
		!((struct reclaim_param *)walk->private)->hiber)
		return false;

	if (reclaim_sigusr_pending(current)) {
		pr_info("Reclaim abort! case is signal\n");
		return true;
	}

	mm = walk->mm;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (mm && !list_empty(&mm->mmap_lock.wait_list)) {
#else
	if (mm && !list_empty(&mm->mmap_sem.wait_list)) {
#endif
		pr_info("Reclaim abort! case is lock race\n");
		return true;
	}

	return false;
}

static int __init process_reclaim_info_init(void)
{
	int ret = 0;

	ret = process_reclaim_result_cache_create();

	if (ret)
		pr_err("Process reclaim information module init failed!\n");
	return ret;
}
late_initcall(process_reclaim_info_init);
