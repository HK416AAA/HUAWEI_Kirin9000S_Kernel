/*
 * lpcpu_cpufreq_dt.h
 *
 * lpcpu cpufreq device tree head file
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#ifndef _LPCPU_CPUFREQ_DT_H
#define _LPCPU_CPUFREQ_DT_H

#include <linux/cpufreq.h>
#include <linux/version.h>
#ifdef CONFIG_LPCPU_HW_VOTE_CPU_FREQ
#include <platform_include/cee/linux/hw_vote.h>
#endif
#ifdef CONFIG_FREQ_STATS_COUNTING_IDLE
#include <linux/cpumask.h>
#include <linux/of_device.h>
#endif


#ifdef CONFIG_LPCPU_CPUFREQ_DT
struct opp_table *lpcpu_cpufreq_set_supported_hw(struct device *cpu_dev);
void lpcpu_cpufreq_put_supported_hw(struct opp_table *opp_table);
int lpcpu_get_cpu_version(unsigned int *cpu_version);

void lpcpu_cpufreq_get_suspend_freq(struct cpufreq_policy *policy);
int lpcpu_cpufreq_init(void);

#ifdef CONFIG_LPCPU_HW_VOTE_CPU_FREQ

#ifdef CONFIG_LPCPU_L2_DYNAMIC_RETENTION
void l2_dynamic_retention_ctrl(struct cpufreq_policy *policy,
			       unsigned int freq);
#endif

struct hvdev *lpcpu_cpufreq_hv_init(struct device *cpu_dev);
void lpcpu_cpufreq_hv_exit(struct hvdev *cpu_hvdev, unsigned int cpu);
int lpcpu_cpufreq_set(struct hvdev *cpu_hvdev, unsigned int freq);
unsigned int lpcpu_cpufreq_get(unsigned int cpu);
void lpcpu_cpufreq_policy_cur_init(struct hvdev *cpu_hvdev,
				   struct cpufreq_policy *policy);
#endif

#endif
#endif /* _LPCPU_CPUFREQ_DT_H */
