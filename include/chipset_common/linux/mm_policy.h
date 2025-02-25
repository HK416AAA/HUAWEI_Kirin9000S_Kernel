/*
 * mm_policy.h
 *
 * Description: provide the huawei policy of memory.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
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
#ifndef MM_POLICY_H_INCLUDED
#define MM_POLICY_H_INCLUDED

#include <linux/gfp.h>

#ifdef CONFIG_HW_MM_POLICY
#include <linux/sched.h>
#ifdef CONFIG_HW_VIP_THREAD
#include <chipset_common/hwcfs/hwcfs_common.h>
#endif

#define MM_POLICY_ALLOC_HARDER	(0x1)
#define MM_POLICY_KVMALLOC	(0x2)
#define MM_POLICY_DISPLAY_CACHE	(0x4)

extern unsigned int get_hw_mm_policy(void);
extern u16 *get_display_buffer(u32 size, bool force);
extern u16 *put_display_buffer(u16 *buf, bool force);
extern unsigned int hw_mm_policy;

#define INIT_MM_POLICY_VM_NODE \
	{ \
		.procname	= "hw_mm_policy", \
		.data		= &hw_mm_policy, \
		.maxlen		= sizeof(unsigned int), \
		.mode		= S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, \
		.proc_handler	= proc_douintvec, \
	}

#ifdef CONFIG_HW_VIP_THREAD
static inline bool vip_care_allocstall(void)
{
	return current->static_vip || atomic64_read(&current->dynamic_vip);
}
#else
static inline bool vip_care_allocstall(void)
{
	return false;
}
#endif
static inline bool mm_policy_vip_alloc_harder(gfp_t gfp)
{
	return (gfp & __GFP_DIRECT_RECLAIM) &&
	    likely(get_hw_mm_policy() & MM_POLICY_ALLOC_HARDER) &&
	    unlikely(vip_care_allocstall());
}

static inline bool mm_policy_vip_kvmalloc_nodelay(gfp_t gfp)
{
	return (gfp & __GFP_DIRECT_RECLAIM) &&
	    likely(get_hw_mm_policy() & MM_POLICY_KVMALLOC) &&
	    unlikely(rt_task(current) || vip_care_allocstall());
}

static inline bool mm_policy_display_cache_enabled(void)
{
	return get_hw_mm_policy() & MM_POLICY_DISPLAY_CACHE;
}
#else
static inline bool mm_policy_vip_alloc_harder(gfp_t gfp)
{
	return false;
}

static inline bool mm_policy_vip_kvmalloc_nodelay(gfp_t gfp)
{
	return false;
}

static inline bool mm_policy_display_cache_enabled(void)
{
	return false;
}

static inline u16 *put_display_buffer(u16 *buf, bool force)
{
	return buf;
}
#endif
#endif /* MM_POLICY_H_INCLUDED */
