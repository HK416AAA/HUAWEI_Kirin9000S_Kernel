/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022.
 * All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 */

#include <linux/regulator/consumer.h>
#include "hvgr_defs.h"
#include "hvgr_platform_api.h"
#include "dvfs/hvgr_pm_dvfs.h"
#include "hvgr_external_base.h"
#include "hvgr_dm_api.h"

static int hvgr_regulator_enable(struct regulator *regulator)
{
	return regulator_enable(regulator);
}

static int hvgr_regulator_disable(struct regulator *regulator)
{
	return regulator_disable(regulator);
}

static int hvgr_gpufreq_callback_register(struct hvgr_device *gdev)
{
#ifdef CONFIG_GPUFREQ_INDEPDENT
	return hvgr_pm_gpufreq_init(gdev);
#else
	return 0;
#endif
}

static bool hvgr_is_guest(void)
{
	return false;
}

static unsigned long hvgr_get_gpu_id(void)
{
	return HVGR_PF_P0;
}

static int hvgr_base_platform_reset(void)
{
	return 0;
}

static int hvgr_base_platform_reset_register(platform_reset_callback callback, void *date)
{
	return 0;
}

static bool hvgr_pm_is_always_on(struct hvgr_device * const gdev)
{
	return false;
}

static struct hvgr_platform_subsys_ops subsys_ops = {
	.regulator_enable = hvgr_regulator_enable,
	.regulator_disable = hvgr_regulator_disable,
	.gpufreq_callback_register = hvgr_gpufreq_callback_register,
	.is_ace_guest = hvgr_is_guest,
	.pm_is_always_on = hvgr_pm_is_always_on,
	.get_gpu_id = hvgr_get_gpu_id,
	.platform_reset = hvgr_base_platform_reset,
	.platform_reset_register = hvgr_base_platform_reset_register
};

struct hvgr_platform_subsys_ops *hvgr_subsys_ops_get(void)
{
	return &subsys_ops;
}

