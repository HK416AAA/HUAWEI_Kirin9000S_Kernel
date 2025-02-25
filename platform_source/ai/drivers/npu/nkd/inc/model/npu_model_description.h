/*
 * npu_model_description.h
 *
 * about model related specification
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
#ifndef __NPU_MODEL_DESCRIPTION_H
#define __NPU_MODEL_DESCRIPTION_H

#include <linux/types.h>
#include "npu_ddr_map.h"

#define NPU_HWTS_SQ_SLOT_SIZE         64
#define NPU_HWTS_CQ_SLOT_SIZE         16

// dont consider the case where the pool size is not the same
#define NPU_MAX_HWTS_SQ_POOL_NUM      4
#define NPU_MAX_LONG_HWTS_SQ_POOL_NUM 1
#define NPU_MAX_HWTS_CQ_POOL_NUM      1

#define NPU_MAX_HWTS_EVENT_ID         1024
#define NPU_MAX_HWTS_NOTIFY_ID        1024

#define NPU_MAX_MODEL_INFO_NUM       (NPU_MAX_MODEL_ID)
#define NPU_MAX_STREAM_INFO_NUM      (NPU_MAX_NON_SINK_STREAM_ID)
#define NPU_MAX_SINK_STREAM_INFO_NUM (NPU_MAX_SINK_STREAM_ID)

#ifndef CONFIG_NPU_HWTS
#define NPU_MAX_HWTS_SQ_NUM 384
#define NPU_MAX_HWTS_CQ_NUM 30
#endif

#define NPU_MAX_SQ_INFO_NUM          (NPU_MAX_SQ_NUM)
#define NPU_MAX_CQ_INFO_NUM          (NPU_MAX_CQ_NUM)
#define NPU_MAX_HWTS_SQ_INFO_NUM     (NPU_MAX_HWTS_SQ_NUM)
#define NPU_MAX_HWTS_CQ_INFO_NUM     (NPU_MAX_HWTS_CQ_NUM)

#define NPU_MODEL_STREAM_NUM  2
#define NPU_MODEL_EVENT_NUM   128

#define NPU_MAX_DFX_SQ_DEPTH  256
#define NPU_MAX_DFX_CQ_DEPTH  256

typedef enum npu_execute_enable_info {
	NPU_MODEL_EXECUTE_DISENABLE = 0,
	NPU_MODEL_EXECUTE_ENABLE,
} npu_execute_enable_info_t;

typedef enum npu_compute_type {
	NPU_COMPUTE_TYPE_NN,
	NPU_COMPUTE_TYPE_CV,
	NPU_COMPUTE_TYPE_MAX
} npu_compute_type_t;

typedef struct npu_model_desc {
	u32 compute_type;
	u16 model_id;
	u16 stream_cnt;
	u16 stream_id[NPU_MODEL_STREAM_NUM];
	u16 stream_tasks[NPU_MODEL_STREAM_NUM];
	void *stream_addr[NPU_MODEL_STREAM_NUM];
	u64 aiv_csw_buff_addr[NPU_MODEL_STREAM_NUM]; // aiv slow context switch buffer base addr
	u32 execute_pid;
	u32 priority;
	u32 uid;
} npu_model_desc_t;

typedef struct npu_model_desc_sub {
	u32 priority;
	u32 execute_pid;
	u32 profiling;
	u32 execute_times;
	u32 reboot_times;
	u32 execute_enable;
	u32 uid;
} npu_model_desc_sub_t;

typedef struct npu_model_desc_info {
	// acpu-->tscpu
	uint16_t stream_id[NPU_MODEL_STREAM_NUM];
	uint8_t reserved0[60];
	uint8_t  event_info[NPU_MODEL_EVENT_NUM];
	uint32_t model_id;
	uint32_t stream_cnt;
	uint64_t aiv_csw_buff_addr[NPU_MODEL_STREAM_NUM]; // aiv slow context switch buffer base addr
	uint16_t compute_type;
	uint8_t reserve0[102]; // 64 bytes cache line align
} npu_model_desc_info_t;

struct npu_hwts_sq_info {
	u32 head; // head sqe
	u32 tail; // tail sqe
	u32 credit; // resv num
	u32 index; // sq index
	u32 length; // 1024/8192
	u32 rsd0;
	u64 hwts_sq_sub;
	u64 iova_addr;
	u8 rsd1[24];
};

struct npu_hwts_cq_info {
	u32 head;
	u32 tail;
	u64 hwts_cq_sub;
	u64 iova_addr;
	u8 rsd[48];
};

/* The info memory of reserved memory order: 1.SQ_INFO, 2.CQ_INFO, 3.STREAM_INFO */
/* 4.HWTS_SQ_INFO, 5.HWTS_CQ_INFO, 6.MODEL_INFO 7.PROFILINGL_INFO 8.TS_STAUS_INFO */

/* reserved memory info size */
#define NPU_HWTS_SQ_INFO_SIZE	sizeof(struct npu_hwts_sq_info)    // 64
#define NPU_HWTS_CQ_INFO_SIZE	sizeof(struct npu_hwts_cq_info)    // 16
#define NPU_MODEL_INFO_SIZE	sizeof(struct npu_model_desc_info)    // 1536

#define NPU_HWTS_SQ_INFO_OCCUPY_SIZE \
	(NPU_HWTS_SQ_INFO_SIZE * NPU_MAX_HWTS_SQ_NUM)
#define NPU_HWTS_CQ_INFO_OCCUPY_SIZE \
	(NPU_HWTS_CQ_INFO_SIZE * NPU_MAX_HWTS_CQ_NUM)
#define NPU_MODEL_INFO_OCCUPY_SIZE   \
	(NPU_MODEL_INFO_SIZE * NPU_MAX_MODEL_INFO_NUM)
#define NPU_PROFILINGL_INFO_OCCUPY_SIZE  (PROF_HEAD_MANAGER_SIZE)
#define NPU_MAILBOX_OCCUPY_SIZE 256

/* reserved memory info offset */
#define NPU_HWTS_SQ_INFO_OFFSET (NPU_SQ_INFO_OCCUPY_SIZE + \
	NPU_CQ_INFO_OCCUPY_SIZE + NPU_STREAM_INFO_OCCUPY_SIZE)
#define NPU_HWTS_CQ_INFO_OFFSET \
	(NPU_HWTS_SQ_INFO_OFFSET + NPU_HWTS_SQ_INFO_OCCUPY_SIZE)
#define NPU_MODEL_INFO_OFFSET \
	(NPU_HWTS_CQ_INFO_OFFSET + NPU_HWTS_CQ_INFO_OCCUPY_SIZE)

/* reserved memory data size */
#define NPU_SQ_OCCUPY_SIZE \
	(NPU_MAX_SQ_NUM * NPU_MAX_SQ_DEPTH * NPU_SQ_SLOT_SIZE)
#define NPU_CQ_OCCUPY_SIZE \
	(NPU_MAX_CQ_NUM * NPU_MAX_CQ_DEPTH * NPU_CQ_SLOT_SIZE)

/* reserved memory DFX data size */
#define NPU_DFX_SQ_OCCUPY_SIZE    \
	(NPU_MAX_DFX_SQ_NUM * NPU_MAX_SQ_DEPTH * NPU_SQ_SLOT_SIZE)
#define NPU_DFX_CQ_OCCUPY_SIZE    \
	(NPU_MAX_DFX_CQ_NUM * NPU_MAX_CQ_DEPTH * NPU_CQ_SLOT_SIZE)
#define NPU_DFX_SQ_INFO_OCCUPY_SIZE    \
	(NPU_MAX_DFX_SQ_NUM * NPU_SQ_INFO_SIZE)
#define NPU_DFX_CQ_INFO_OCCUPY_SIZE    \
	(NPU_MAX_DFX_CQ_NUM * NPU_CQ_INFO_SIZE)
#define NPU_DFX_OCCUPY_SIZE    \
	((NPU_DFX_SQ_OCCUPY_SIZE) + (NPU_DFX_CQ_OCCUPY_SIZE) + \
		(NPU_DFX_SQ_INFO_OCCUPY_SIZE) + (NPU_DFX_CQ_INFO_OCCUPY_SIZE))

#define NPU_TS_LOG_OCCUPY_SIZE 0x80000  // 512k
#define NPU_BLACKBOX_OCCUPY_SIZE 0x40000 // 256k

/* sysdma related */
#define NPU_SDMA_CHANNEL_SHIFT 8
#define NPU_HWTS_SDMA_CHANNEL_NUM 4 /* 0-3:HWTS, 4-7:reserved */
#define NPU_HWTS_CFG_BASE (SOC_ACPU_hwts_BASE_ADDR)
#define NPU_HWTS_SDMA_CQ_CFG_OFFSET 0xA0000
/* Each 4KB is reserved for each SDMA CQ to write back to HWTS
 * in order to notify a completion status, The size is allowed
 * to 16B in this region, as there's no actual nManager registers
 */
#define NPU_HWTS_SDMA_CQ_OFFSET 0x2000
#define SDMA_CHN_QOS 3 /* 4 level: 3>2>1>0 */
#define SDMA_SQ_SIZE 10 /* sdma sq len: 2^SDMA_SQ_SIZE max len 65536-1 */
#define SDMA_CQ_SIZE 9 /* sdma cq len: 2^SDMA_CQ_SIZE, max len 512-1 */
#define SDMA_CQ_ENTRIES (1 << SDMA_CQ_SIZE)
#define SDMA_SQ_ENTRIES (1 << SDMA_SQ_SIZE)
#define SDMA_SQE_SIZE 32
#define SMMU_BYPASS_SID 63

#endif
