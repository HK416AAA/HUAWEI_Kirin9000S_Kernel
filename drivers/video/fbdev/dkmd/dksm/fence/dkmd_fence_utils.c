/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_log.h"

#include <linux/file.h>
#include <linux/sync_file.h>
#include <linux/fs.h>

#include "dkmd_fence_utils.h"

int32_t dkmd_fence_get_fence_fd(struct dma_fence *fence)
{
	int32_t fd;
	struct sync_file *sync_file = NULL;

	if (unlikely(!fence)) {
		dpu_pr_err("fence is null\n");
		return -1;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		dpu_pr_err("fail to get unused fd\n");
		return fd;
	}

	sync_file = sync_file_create(fence);
	if (!sync_file) {
		put_unused_fd(fd);
		dpu_pr_err("failed to create sync file\n");
		return -ENOMEM;
	}

	fd_install(fd, sync_file->file);

	return fd;
}

int32_t dkmd_fence_signal_fence(int32_t fence_fd)
{
	struct dma_fence *fence = NULL;

	if (fence_fd < 0)
		return 0;

	fence = sync_file_get_fence(fence_fd);
	if (!fence) {
		dpu_pr_err("fence_fd=%d sync_file_get_fence failed!\n", fence_fd);
		return -EINVAL;
	}

	if (test_bit(DMA_FENCE_FLAG_SIGNALED_BIT, &fence->flags))
		return 0;

	if (dma_fence_signal_timestamp(fence, ktime_get()) != 0)
		dpu_pr_err("fence_fd %d signal timestamp fail", fence_fd);

	dma_fence_put(fence);

	dpu_pr_debug("signal fence fd %d", fence_fd);
	return 0;
}

