// SPDX-License-Identifier: GPL-2.0
/*
 *  linux/drivers/thermal/cpufreq_cooling.c
 *
 *  Copyright (C) 2012	Samsung Electronics Co., Ltd(http://www.samsung.com)
 *
 *  Copyright (C) 2012-2018 Linaro Limited.
 *
 *  Authors:	Amit Daniel <amit.kachhap@linaro.org>
 *		Viresh Kumar <viresh.kumar@linaro.org>
 *
 */
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpu_cooling.h>
#include <linux/energy_model.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/idr.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/thermal.h>

#include <securec.h>
#include <trace/events/thermal.h>
#ifdef CONFIG_IPA_THERMAL
#include <platform_include/cee/linux/lpm_thermal.h>
#include <trace/events/ipa_thermal_trace.h>
#include <trace/events/thermal_power_allocator.h>
#endif
#include <platform_include/cee/linux/time_in_state.h>

/*
 * Cooling state <-> CPUFreq frequency
 *
 * Cooling states are translated to frequencies throughout this driver and this
 * is the relation between them.
 *
 * Highest cooling state corresponds to lowest possible frequency.
 *
 * i.e.
 *	level 0 --> 1st Max Freq
 *	level 1 --> 2nd Max Freq
 *	...
 */
#ifndef CONFIG_IPA_THERMAL


/**
 * struct time_in_idle - Idle time stats
 * @time: previous reading of the absolute time that this cpu was idle
 * @timestamp: wall time of the last invocation of get_cpu_idle_time_us()
 */
struct time_in_idle {
	u64 time;
	u64 timestamp;
};

/**
 * struct cpufreq_cooling_device - data for cooling device with cpufreq
 * @id: unique integer value corresponding to each cpufreq_cooling_device
 *	registered.
 * @last_load: load measured by the latest call to cpufreq_get_requested_power()
 * @cpufreq_state: integer value representing the current state of cpufreq
 *	cooling	devices.
 * @max_level: maximum cooling level. One less than total number of valid
 *	cpufreq frequencies.
 * @em: Reference on the Energy Model of the device
 * @cdev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @policy: cpufreq policy.
 * @node: list_head to link all cpufreq_cooling_device together.
 * @idle_time: idle time stats
 * @qos_req: PM QoS contraint to apply
 *
 * This structure is required for keeping information of each registered
 * cpufreq_cooling_device.
 */
struct cpufreq_cooling_device {
	int id;
	u32 last_load;
	unsigned int cpufreq_state;
	unsigned int max_level;
	struct em_perf_domain *em;
	struct cpufreq_policy *policy;
	struct list_head node;
	struct time_in_idle *idle_time;
	struct freq_qos_request qos_req;
};
#endif

#ifdef CONFIG_IPA_THERMAL
DEFINE_IDA(cpufreq_ida);
DEFINE_MUTEX(cooling_list_lock);
LIST_HEAD(cpufreq_cdev_list);
DEFINE_MUTEX(cooling_cpufreq_lock);
#else
static DEFINE_IDA(cpufreq_ida);
static DEFINE_MUTEX(cooling_list_lock);
static LIST_HEAD(cpufreq_cdev_list);
#endif
#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
/**
 * get_level: Find the level for a particular frequency
 * @cpufreq_cdev: cpufreq_cdev for which the property is required
 * @freq: Frequency
 *
 * Return: level corresponding to the frequency.
 */
#ifdef CONFIG_IPA_THERMAL
static unsigned long get_level(struct cpufreq_cooling_device *cpufreq_cdev,
			       unsigned int freq)
{
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	unsigned long level;

	for (level = 1; level <= cpufreq_cdev->max_level; level++)
		if (freq > freq_table[level].frequency)
			break;

	return level - 1;
}
static u32 cpu_freq_to_power(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 freq)
{
	int i;
	struct freq_table *freq_table = cpufreq_cdev->freq_table;

	for (i = 1; i <= cpufreq_cdev->max_level; i++)
		if (freq > freq_table[i].frequency)
			break;

	return freq_table[i - 1].power;
}

static u32 cpu_power_to_freq(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 power)
{
	int i;
	struct freq_table *freq_table = cpufreq_cdev->freq_table;

	for (i = 0; i < cpufreq_cdev->max_level; i++)
		if (power >= freq_table[i].power)
			break;

	return freq_table[i].frequency;
}
#else
static unsigned long get_level(struct cpufreq_cooling_device *cpufreq_cdev,
			       unsigned int freq)
{
	int i;

	for (i = cpufreq_cdev->max_level - 1; i >= 0; i--) {
		if (freq > cpufreq_cdev->em->table[i].frequency)
			break;
	}

	return cpufreq_cdev->max_level - i - 1;
}

static u32 cpu_freq_to_power(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 freq)
{
	int i;

	for (i = cpufreq_cdev->max_level - 1; i >= 0; i--) {
		if (freq > cpufreq_cdev->em->table[i].frequency)
			break;
	}

	return cpufreq_cdev->em->table[i + 1].power;
}

static u32 cpu_power_to_freq(struct cpufreq_cooling_device *cpufreq_cdev,
			     u32 power)
{
	int i;

	for (i = cpufreq_cdev->max_level; i > 0; i--) {
		if (power >= cpufreq_cdev->em->table[i].power)
			break;
	}

	return cpufreq_cdev->em->table[i].frequency;
}
#endif
/**
 * get_load() - get load for a cpu since last updated
 * @cpufreq_cdev:	&struct cpufreq_cooling_device for this cpu
 * @cpu:	cpu number
 * @cpu_idx:	index of the cpu in time_in_idle*
 *
 * Return: The average load of cpu @cpu in percentage since this
 * function was last called.
 */
static u32 get_load(struct cpufreq_cooling_device *cpufreq_cdev, int cpu,
		    int cpu_idx)
{
	u32 load;
	u64 now, now_idle, delta_time, delta_idle;
	struct time_in_idle *idle_time = &cpufreq_cdev->idle_time[cpu_idx];

	now_idle = get_cpu_idle_time(cpu, &now, 0);
	delta_idle = now_idle - idle_time->time;
	delta_time = now - idle_time->timestamp;

#ifdef CONFIG_SCHED_SMT
	if (cpumask_weight(cpu_smt_mask(cpu)) == 2) {
		u64 total_busy[2], delta_busy[2];

		lpcpu_time_in_smt_get(cpu, total_busy, 2);
		delta_busy[0] = total_busy[0] - cpufreq_cdev->busy_time[cpu_idx].mt_busy_time;
		delta_busy[1] = total_busy[1] - cpufreq_cdev->busy_time[cpu_idx].st_busy_time;
		if (delta_busy[0] + delta_busy[1] > delta_time)
			delta_time = delta_busy[0] + delta_busy[1];
		delta_idle = delta_time - (delta_busy[0] * 65 / 100) - (delta_busy[1] * 50 / 100);
		cpufreq_cdev->busy_time[cpu_idx].mt_busy_time = total_busy[0];
		cpufreq_cdev->busy_time[cpu_idx].st_busy_time = total_busy[1];
	}
#endif

	if (delta_time <= delta_idle)
		load = 0;
	else
		load = div64_u64(100 * (delta_time - delta_idle), delta_time);

	idle_time->time = now_idle;
	idle_time->timestamp = now;

	return load;
}

/**
 * get_dynamic_power() - calculate the dynamic power
 * @cpufreq_cdev:	&cpufreq_cooling_device for this cdev
 * @freq:	current frequency
 *
 * Return: the dynamic power consumed by the cpus described by
 * @cpufreq_cdev.
 */
static u32 get_dynamic_power(struct cpufreq_cooling_device *cpufreq_cdev,
			     unsigned long freq)
{
	u32 raw_cpu_power;

#ifdef CONFIG_ITS_IPA
	if (check_its_enabled())
		return cpu_get_dynamic_power(cpufreq_cdev);
#endif

	raw_cpu_power = cpu_freq_to_power(cpufreq_cdev, freq);
	return (raw_cpu_power * cpufreq_cdev->last_load) / 100;
}

/**
 * cpufreq_get_requested_power() - get the current power
 * @cdev:	&thermal_cooling_device pointer
 * @power:	pointer in which to store the resulting power
 *
 * Calculate the current power consumption of the cpus in milliwatts
 * and store it in @power.  This function should actually calculate
 * the requested power, but it's hard to get the frequency that
 * cpufreq would have assigned if there were no thermal limits.
 * Instead, we calculate the current power on the assumption that the
 * immediate future will look like the immediate past.
 *
 * We use the current frequency and the average load since this
 * function was last called.  In reality, there could have been
 * multiple opps since this function was last called and that affects
 * the load calculation.  While it's not perfectly accurate, this
 * simplification is good enough and works.  REVISIT this, as more
 * complex code may be needed if experiments show that it's not
 * accurate enough.
 *
 * Return: 0 on success, -E* if getting the static power failed.
 */
static int cpufreq_get_requested_power(struct thermal_cooling_device *cdev,
				       u32 *power)
{
	unsigned long freq;
	int i = 0, cpu;
	u32 total_load = 0;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	u32 *load_cpu = NULL;
	u32 dynamic_power;
#ifdef CONFIG_IPA_THERMAL
	u32 static_power;
	u32 max_load = 0;
	int ret;
#endif

	freq = cpufreq_quick_get(policy->cpu);
#ifdef CONFIG_IPA_THERMAL
	/* policy->cur equals 0, means the policy data of this cpu was NULL,
	*  return early to avoid find voltage of freq(0) in the opp
	*/
	if (!freq) {
		*power = 0;
		return 0;
	}
#endif

#ifdef CONFIG_IPA_THERMAL
	if (1) { /*lint !e774*/
#else
	if (trace_thermal_power_cpu_get_power_enabled()) {
#endif
		u32 ncpus = cpumask_weight(policy->related_cpus);

		load_cpu = kcalloc(ncpus, sizeof(*load_cpu), GFP_KERNEL);
	}

	for_each_cpu(cpu, policy->related_cpus) {
		u32 load;

		if (cpu_online(cpu))
			load = get_load(cpufreq_cdev, cpu, i);
		else
			load = 0;

		total_load += load;
#ifdef CONFIG_IPA_THERMAL
		if (load > max_load)
			max_load = load;
#endif
		if (load_cpu)
			load_cpu[i] = load;

		i++;
	}

	cpufreq_cdev->last_load = total_load;

	dynamic_power = get_dynamic_power(cpufreq_cdev, freq);
#ifdef CONFIG_IPA_THERMAL
	ret = cpu_get_static_power(cpufreq_cdev, freq, &static_power);
	if (ret != 0) {
		kfree(load_cpu);
		return ret;
	}
	*power = static_power + dynamic_power;
#else
	*power = dynamic_power;
#endif

#ifdef CONFIG_ITS_IPA
	cpufreq_cdev->normalized_powerdata = normalize_its_power(cpufreq_cdev,
								 dynamic_power,
								 freq);
#endif
	if (load_cpu) {
		trace_thermal_power_cpu_get_power(policy->related_cpus, freq,
						  load_cpu, i, *power);

		kfree(load_cpu);
	}
#ifdef CONFIG_IPA_THERMAL
	cdev->current_freq = freq;
	if (load_cpu != NULL)
		cdev->current_load = max_load;
#endif

	return 0;
}

/**
 * cpufreq_state2power() - convert a cpu cdev state to power consumed
 * @cdev:	&thermal_cooling_device pointer
 * @state:	cooling device state to be converted
 * @power:	pointer in which to store the resulting power
 *
 * Convert cooling device state @state into power consumption in
 * milliwatts assuming 100% load.  Store the calculated power in
 * @power.
 *
 * Return: 0 on success, -EINVAL if the cooling device state could not
 * be converted into a frequency or other -E* if there was an error
 * when calculating the static power.
 */
static int cpufreq_state2power(struct thermal_cooling_device *cdev,
			       unsigned long state, u32 *power)
{
	unsigned int freq, num_cpus, idx;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
#ifdef CONFIG_IPA_THERMAL
	int ret;
	u32 static_power;
#endif

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

	num_cpus = cpumask_weight(cpufreq_cdev->policy->cpus);

	idx = cpufreq_cdev->max_level - state;
#ifdef CONFIG_IPA_THERMAL
	freq = cpufreq_cdev->freq_table[state].frequency;
	ret = cpu_get_static_power(cpufreq_cdev, freq, &static_power);
	if (ret != 0)
		return ret;

	*power = static_power + cpu_freq_to_power(cpufreq_cdev, freq) * num_cpus;
#else
	freq = cpufreq_cdev->em->table[idx].frequency;
	*power = cpu_freq_to_power(cpufreq_cdev, freq) * num_cpus;
#endif

	return 0;
}

/**
 * cpufreq_power2state() - convert power to a cooling device state
 * @cdev:	&thermal_cooling_device pointer
 * @power:	power in milliwatts to be converted
 * @state:	pointer in which to store the resulting state
 *
 * Calculate a cooling device state for the cpus described by @cdev
 * that would allow them to consume at most @power mW and store it in
 * @state.  Note that this calculation depends on external factors
 * such as the cpu load or the current static power.  Calling this
 * function with the same power as input can yield different cooling
 * device states depending on those external factors.
 *
 * Return: 0 on success, -ENODEV if no cpus are online or -EINVAL if
 * the calculated frequency could not be converted to a valid state.
 * The latter should not happen unless the frequencies available to
 * cpufreq have changed since the initialization of the cpu cooling
 * device.
 */
static int cpufreq_power2state(struct thermal_cooling_device *cdev,
			       u32 power, unsigned long *state)
{
#ifdef CONFIG_IPA_THERMAL
	return cpu_power_to_state(cdev, power, state);
#else
	unsigned int target_freq;
	u32 last_load, normalised_power;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;

	last_load = cpufreq_cdev->last_load ?: 1;
	normalised_power = (power * 100) / last_load;
	target_freq = cpu_power_to_freq(cpufreq_cdev, normalised_power);

	*state = get_level(cpufreq_cdev, target_freq);
	trace_thermal_power_cpu_limit(policy->related_cpus, target_freq, *state,
				      power);
#endif

	return 0;
}

static inline bool em_is_sane(struct cpufreq_cooling_device *cpufreq_cdev,
			      struct em_perf_domain *em) {
	struct cpufreq_policy *policy;
	unsigned int nr_levels;

	if (!em)
		return false;

	policy = cpufreq_cdev->policy;
	if (!cpumask_equal(policy->related_cpus, em_span_cpus(em))) {
		pr_err("The span of pd %*pbl is misaligned with cpufreq policy %*pbl\n",
			cpumask_pr_args(em_span_cpus(em)),
			cpumask_pr_args(policy->related_cpus));
		return false;
	}

	nr_levels = cpufreq_cdev->max_level + 1;
	if (em_pd_nr_perf_states(em) != nr_levels) {
		pr_err("The number of performance states in pd %*pbl (%u) doesn't match the number of cooling levels (%u)\n",
			cpumask_pr_args(em_span_cpus(em)),
			em_pd_nr_perf_states(em), nr_levels);
		return false;
	}

	return true;
}
#endif /* CONFIG_THERMAL_GOV_POWER_ALLOCATOR */

#ifdef CONFIG_IPA_THERMAL
unsigned int get_state_freq(struct cpufreq_cooling_device *cpufreq_cdev,
                            unsigned long state)
#else
static unsigned int get_state_freq(struct cpufreq_cooling_device *cpufreq_cdev,
				   unsigned long state)
#endif
{
	struct cpufreq_policy *policy;
	unsigned long idx;

#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
	/* Use the Energy Model table if available */
	if (cpufreq_cdev->em) {
		idx = cpufreq_cdev->max_level - state;
		return cpufreq_cdev->em->table[idx].frequency;
	}
#endif

	/* Otherwise, fallback on the CPUFreq table */
	policy = cpufreq_cdev->policy;
	if (policy->freq_table_sorted == CPUFREQ_TABLE_SORTED_ASCENDING)
		idx = cpufreq_cdev->max_level - state;
	else
		idx = state;

	return policy->freq_table[idx].frequency;
}

/* cpufreq cooling device callback functions are defined below */

/**
 * cpufreq_get_max_state - callback function to get the max cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the max cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * max cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;

	*state = cpufreq_cdev->max_level;
	return 0;
}

/**
 * cpufreq_get_cur_state - callback function to get the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: fill this variable with the current cooling state.
 *
 * Callback for the thermal cooling device to return the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;

	*state = cpufreq_cdev->cpufreq_state;

	return 0;
}

/**
 * cpufreq_set_cur_state - callback function to set the current cooling state.
 * @cdev: thermal cooling device pointer.
 * @state: set this variable to the current cooling state.
 *
 * Callback for the thermal cooling device to change the cpufreq
 * current cooling state.
 *
 * Return: 0 on success, an error code otherwise.
 */
static int cpufreq_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
#ifndef CONFIG_IPA_THERMAL
	struct cpumask *cpus;
	unsigned int frequency;
	unsigned long max_capacity, capacity;
#endif
	int ret;

	/* Request state should be less than max_level */
	if (state > cpufreq_cdev->max_level)
		return -EINVAL;

#ifdef CONFIG_IPA_THERMAL
	cpu_cooling_get_limit_state(cpufreq_cdev, state);
	ret = 0;
#else
	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_cdev->cpufreq_state == state)
		return 0;

	frequency = get_state_freq(cpufreq_cdev, state);

	ret = freq_qos_update_request(&cpufreq_cdev->qos_req, frequency);
	if (ret >= 0) {
		cpufreq_cdev->cpufreq_state = state;
		cpus = cpufreq_cdev->policy->cpus;
		max_capacity = arch_scale_cpu_capacity(cpumask_first(cpus));
		capacity = frequency * max_capacity;
		capacity /= cpufreq_cdev->policy->cpuinfo.max_freq;
		arch_set_thermal_pressure(cpus, max_capacity - capacity);
		ret = 0;
	}
#endif
	return ret;
}

/* Bind cpufreq callbacks to thermal cooling device ops */

static struct thermal_cooling_device_ops cpufreq_cooling_ops = {
	.get_max_state		= cpufreq_get_max_state,
	.get_cur_state		= cpufreq_get_cur_state,
	.set_cur_state		= cpufreq_set_cur_state,
};

static unsigned int find_next_max(struct cpufreq_frequency_table *table,
				  unsigned int prev_max)
{
	struct cpufreq_frequency_table *pos;
	unsigned int max = 0;

	cpufreq_for_each_valid_entry(pos, table) {
		if (pos->frequency > max && pos->frequency < prev_max)
			max = pos->frequency;
	}
	return max;
}

/**
 * __cpufreq_cooling_register - helper function to create cpufreq cooling device
 * @np: a valid struct device_node to the cooling device device tree node
 * @policy: cpufreq policy
 * Normally this should be same as cpufreq policy->related_cpus.
 * @em: Energy Model of the cpufreq policy
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. It also gives the opportunity to link the cooling device
 * with a device tree node, in order to bind it via the thermal DT code.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
static struct thermal_cooling_device *
__cpufreq_cooling_register(struct device_node *np,
			struct cpufreq_policy *policy,
			struct em_perf_domain *em)
{
	struct thermal_cooling_device *cdev;
	struct cpufreq_cooling_device *cpufreq_cdev;
	char dev_name[THERMAL_NAME_LENGTH];
	unsigned int i, num_cpus;
	struct device *dev;
	int ret, freq;
	struct thermal_cooling_device_ops *cooling_ops;

	dev = get_cpu_device(policy->cpu);
	if (unlikely(!dev)) {
		pr_warn("No cpu device for cpu %d\n", policy->cpu);
		return ERR_PTR(-ENODEV);
	}

	if (IS_ERR_OR_NULL(policy)) {
		pr_err("%s: cpufreq policy isn't valid: %p\n", __func__, policy);
		return ERR_PTR(-EINVAL);
	}

	i = cpufreq_table_count_valid_entries(policy);
	if (!i) {
		pr_debug("%s: CPUFreq table not found or has no valid entries\n",
			 __func__);
		return ERR_PTR(-ENODEV);
	}

	cpufreq_cdev = kzalloc(sizeof(*cpufreq_cdev), GFP_KERNEL);
	if (!cpufreq_cdev)
		return ERR_PTR(-ENOMEM);

	cpufreq_cdev->policy = policy;
	num_cpus = cpumask_weight(policy->related_cpus);
	cpufreq_cdev->idle_time = kcalloc(num_cpus,
					 sizeof(*cpufreq_cdev->idle_time),
					 GFP_KERNEL);
	if (!cpufreq_cdev->idle_time) {
		cdev = ERR_PTR(-ENOMEM);
		goto free_cdev;
	}

#ifdef CONFIG_SCHED_SMT
	cpufreq_cdev->busy_time = kcalloc(num_cpus,
					 sizeof(*cpufreq_cdev->busy_time),
					 GFP_KERNEL);
	if (!cpufreq_cdev->busy_time) {
		cdev = ERR_PTR(-ENOMEM);
		goto free_idle_time;
	}
#endif

	/* max_level is an index, not a counter */
	cpufreq_cdev->max_level = i - 1;
#ifdef CONFIG_IPA_THERMAL
	cpufreq_cdev->freq_table = kmalloc_array(i,
					sizeof(*cpufreq_cdev->freq_table),
					GFP_KERNEL);
	if (!cpufreq_cdev->freq_table) {
		cdev = ERR_PTR(-ENOMEM);
		goto free_idle_time;
	}
#endif
	ret = ida_simple_get(&cpufreq_ida, 0, 0, GFP_KERNEL);
	if (ret < 0) {
		cdev = ERR_PTR(ret);
		goto free_idle_time;
	}
	cpufreq_cdev->id = ret;

	snprintf(dev_name, sizeof(dev_name), "thermal-cpufreq-%d",
		 cpufreq_cdev->id);
#ifdef CONFIG_IPA_THERMAL
	/* Fill freq-table in descending order of frequencies */
	for (i = 0, freq = -1; i <= cpufreq_cdev->max_level; i++) { /*lint !e570*/
		freq = find_next_max(policy->freq_table, freq);
		cpufreq_cdev->freq_table[i].frequency = freq;

		/* Warn for duplicate entries */
		if (!freq)
			pr_warn("%s: table has duplicate entries\n", __func__);
		else
			pr_debug("%s: freq:%u KHz\n", __func__, freq);
	}
#endif
	cooling_ops = &cpufreq_cooling_ops;

#ifdef CONFIG_THERMAL_GOV_POWER_ALLOCATOR
#ifdef CONFIG_IPA_THERMAL
	if (1) {
#else
	if (em_is_sane(cpufreq_cdev, em)) {
#endif
		cpufreq_cdev->em = em;
		cooling_ops->get_requested_power = cpufreq_get_requested_power;
		cooling_ops->state2power = cpufreq_state2power;
		cooling_ops->power2state = cpufreq_power2state;
	} else
#endif
	if (policy->freq_table_sorted == CPUFREQ_TABLE_UNSORTED) {
		pr_err("%s: unsorted frequency tables are not supported\n",
		       __func__);
		cdev = ERR_PTR(-EINVAL);
		goto remove_ida;
	}

	ret = freq_qos_add_request(&policy->constraints,
				   &cpufreq_cdev->qos_req, FREQ_QOS_MAX,
				   get_state_freq(cpufreq_cdev, 0));
	if (ret < 0) {
		pr_err("%s: Failed to add freq constraint (%d)\n", __func__,
		       ret);
		cdev = ERR_PTR(ret);
		goto remove_ida;
	}

	cdev = thermal_of_cooling_device_register(np, dev_name, cpufreq_cdev,
						  cooling_ops);
	if (IS_ERR(cdev))
		goto remove_qos_req;

	mutex_lock(&cooling_list_lock);
	list_add(&cpufreq_cdev->node, &cpufreq_cdev_list);
	mutex_unlock(&cooling_list_lock);

	return cdev;

remove_qos_req:
	freq_qos_remove_request(&cpufreq_cdev->qos_req);
remove_ida:
	ida_simple_remove(&cpufreq_ida, cpufreq_cdev->id);
free_idle_time:
#ifdef CONFIG_SCHED_SMT
	if (cpufreq_cdev->busy_time)
		kfree(cpufreq_cdev->busy_time);
#endif
	kfree(cpufreq_cdev->idle_time);
free_cdev:
	kfree(cpufreq_cdev);
	return cdev;
}

/**
 * cpufreq_cooling_register - function to create cpufreq cooling device.
 * @policy: cpufreq policy
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * on failure, it returns a corresponding ERR_PTR().
 */
struct thermal_cooling_device *
cpufreq_cooling_register(struct cpufreq_policy *policy)
{
	return __cpufreq_cooling_register(NULL, policy, NULL);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_register);

/**
 * of_cpufreq_cooling_register - function to create cpufreq cooling device.
 * @policy: cpufreq policy
 *
 * This interface function registers the cpufreq cooling device with the name
 * "thermal-cpufreq-%x". This api can support multiple instances of cpufreq
 * cooling devices. Using this API, the cpufreq cooling device will be
 * linked to the device tree node provided.
 *
 * Using this function, the cooling device will implement the power
 * extensions by using a simple cpu power model.  The cpus must have
 * registered their OPPs using the OPP library.
 *
 * It also takes into account, if property present in policy CPU node, the
 * static power consumed by the cpu.
 *
 * Return: a valid struct thermal_cooling_device pointer on success,
 * and NULL on failure.
 */
struct thermal_cooling_device *
of_cpufreq_cooling_register(struct cpufreq_policy *policy)
{
#ifdef CONFIG_IPA_THERMAL
	struct device_node *np = NULL;
	char node[16] = {0};
	u32 cluster;
	int ret;
#else
	struct device_node *np = of_get_cpu_node(policy->cpu, NULL);
#endif
	struct thermal_cooling_device *cdev = NULL;

#ifdef CONFIG_IPA_THERMAL
	cluster = topology_physical_package_id(policy->cpu);
	ret = snprintf_s(node, sizeof(node), (sizeof(node) - 1),
			"cluster%d", cluster);
	if (ret < 0) {
		pr_err("snprintf_s error: %s\n", node);
		return ERR_PTR(-ENODEV);
	}

	np = of_find_node_by_name(NULL, node);
	if (np == NULL) {
		pr_err("Node not found: %s\n", node);
		return ERR_PTR(-ENODEV);
	}
#else
	if (!np) {
		pr_err("cpufreq_cooling: OF node not available for cpu%d\n",
		       policy->cpu);
		return NULL;
	}
#endif
	if (of_find_property(np, "#cooling-cells", NULL)) {
		struct em_perf_domain *em = em_cpu_get(policy->cpu);

		cdev = __cpufreq_cooling_register(np, policy, em);
		if (IS_ERR(cdev)) {
			pr_err("cpufreq_cooling: cpu%d failed to register as cooling device: %ld\n",
			       policy->cpu, PTR_ERR(cdev));
			cdev = NULL;
		}
	}

	of_node_put(np);
	return cdev;
}
EXPORT_SYMBOL_GPL(of_cpufreq_cooling_register);

/**
 * cpufreq_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 *
 * This interface function unregisters the "thermal-cpufreq-%x" cooling device.
 */
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	struct cpufreq_cooling_device *cpufreq_cdev;

	if (!cdev)
		return;

	cpufreq_cdev = cdev->devdata;

	mutex_lock(&cooling_list_lock);
	list_del(&cpufreq_cdev->node);
	mutex_unlock(&cooling_list_lock);

	thermal_cooling_device_unregister(cdev);
	freq_qos_remove_request(&cpufreq_cdev->qos_req);
	ida_simple_remove(&cpufreq_ida, cpufreq_cdev->id);
	kfree(cpufreq_cdev->idle_time);
	kfree(cpufreq_cdev);
}
EXPORT_SYMBOL_GPL(cpufreq_cooling_unregister);
