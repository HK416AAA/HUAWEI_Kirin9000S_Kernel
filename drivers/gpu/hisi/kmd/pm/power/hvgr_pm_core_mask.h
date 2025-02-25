/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022. All rights reserved.
 * This file is the dvfs implementation of the PM module.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 */
#ifndef HVGR_PM_CORE_MASK_H
#define HVGR_PM_CORE_MASK_H

#include "hvgr_pm_defs.h"

ssize_t hvgr_pm_set_core_mask_inner(struct hvgr_device *gdev,
	const char *buf, size_t count);
void hvgr_core_mask_init(struct hvgr_device *gdev);
#endif
