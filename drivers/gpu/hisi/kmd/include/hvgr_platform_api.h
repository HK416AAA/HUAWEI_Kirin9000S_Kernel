/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2022-2022. All rights reserved.
 */

#ifndef HVGR_PLATFORM_API_H
#define HVGR_PLATFORM_API_H

#include <linux/types.h>
#include <linux/platform_device.h>

#include "hvgr_defs.h"

enum {
	HVGR_PF_P0 = 0,
	HVGR_PF_P1,
	HVGR_PF_PALL
};

enum dmd_nv_mark {
	DMD_NV_UNMARK = 0,
	DMD_NV_MARK
};

/*
 * hvgr_platform_map_reg_addr - map ctrl process.
 *
 * @param pdev: platform pointer.
 *
 * @param gdev: hvgr pointer.
 */
int hvgr_platform_map_reg_addr(struct platform_device *pdev, struct hvgr_device *gdev);

/*
 * hvgr_platform_smmu_bypass - smu bypass process for cpro.
 *
 * @param gdev: hvgr pointer.
 * @param partition: p0 or p1 or pall
 */
uint64_t hvgr_platform_smmu_bypass(struct hvgr_device *gdev, u32 partition);

/*
 * hvgr_platform_sec_cfg - set sec config process for cpro.
 *
 * @param gdev: hvgr pointer.
 * @param partition: p0 or p1 or pall
 */
uint64_t hvgr_platform_sec_cfg(struct hvgr_device *gdev, u32 partition);

/*
 * hvgr_platform_reset - GPU hard reset process.
 *
 * @param gdev: hvgr pointer.
 */
void hvgr_platform_reset(struct hvgr_device *gdev);

/*
 * hvgr_platform_timestamp_cbit_config - GPU timestamp_cbit config for cpro.
 *
 * @param gdev: hvgr pointer.
 */
void hvgr_platform_timestamp_cbit_config(struct hvgr_device *gdev);

/*
 * hvgr_sc_config_remap_register - GPU sc remap process for cpro.
 *
 * @param gdev: hvgr pointer.
 */
void hvgr_sc_config_remap_register(struct hvgr_device *gdev);

/*
 * hvgr_sc_config_remap_register_early - GPU sc remap process.
 *
 * @param gdev: hvgr pointer.
 */
void hvgr_sc_config_remap_register_early(struct hvgr_device *gdev);

/*
 * hvgr_platform_enable_icg_clk - enable icg clk process.
 *
 * @param gdev: hvgr pointer.
 */
void hvgr_platform_enable_icg_clk(struct hvgr_device *gdev);

/*
 * hvgr_platform_protected_cfg - in or out TZPC.
 *
 * @param gdev: hvgr pointer.
 *
 * @param mode: 0 : in TZPC;
 *              1 : out TZPC;
 */
uint64_t hvgr_platform_protected_cfg(struct hvgr_device *gdev, u32 mode);

/*
 * hvgr_platform_set_mode - set vir mode.
 *
 * @param gdev: hvgr pointer.
 *
 */
uint64_t hvgr_platform_set_mode(struct hvgr_device *gdev);

/*
 * hvgr_platform_set_qos - set gpu qos.
 *
 * @param gdev: hvgr pointer.
 *
 */
uint64_t hvgr_platform_set_qos(struct hvgr_device *gdev);

/*
 * hvgr_platform_register_reset - register reset api to hvgr_fe.
 *
 * @param gdev: hvgr pointer.
 *
 */
int hvgr_platform_register_reset(struct hvgr_device *gdev);

/*
 * hvgr_platform_reset_guest - reset partition1
 *
 * @param data: hvgr pointer.
 *
 */
u32 hvgr_platform_reset_guest(void *data);

/*
 * hvgr_platform_smmu_set_sid - set sid for gpu summ
 *
 * @param gdev: hvgr pointer.
 *
 */
uint64_t hvgr_platform_smmu_set_sid(struct hvgr_device *gdev);

/*
 * hvgr_platform_smmu_tcu_on - on on tcu for gpu smmu
 *
 * @param gdev: hvgr pointer.
 *
 */
int hvgr_platform_smmu_tcu_on(struct hvgr_device *gdev);

/*
 * hvgr_platform_smmu_tcu_off - tcu off for gpu smmu
 *
 * @param gdev: hvgr pointer.
 *
 */
int hvgr_platform_smmu_tcu_off(struct hvgr_device *gdev);

/*
 * hvgr_platform_smmu_tbu_connect - smmu tbu connect
 *
 * @param gdev: hvgr pointer.
 *
 */
uint64_t hvgr_platform_smmu_tbu_connect(struct hvgr_device *gdev);

/*
 * hvgr_platform_smmu_set_sid - smmu tbu disconnect
 *
 * @param gdev: hvgr pointer.
 *
 */
uint64_t hvgr_platform_smmu_tbu_disconnect(struct hvgr_device *gdev);

/*
 * hvgr_dmd_msg_check - dmd msg check
 *
 * @param gdev: hvgr pointer.
 *
 */
void hvgr_dmd_msg_check(struct hvgr_device *gdev);

/*
 * hvgr_get_sh_dmd_id - get sh dmd id
 */
unsigned int hvgr_get_sh_dmd_id(void);

/*
 * hvgr_platform_sh_runtime
 *
 * @param gdev: hvgr pointer.
 * @param addr: data addr.
 * @param size: data size.
 */
uint64_t hvgr_platform_sh_runtime(struct hvgr_device *gdev, uint64_t addr, uint64_t size);

/*
 * hvgr_set_sid - set sid
 *
 * @param ctx: hvgr pointer.
 * @param sid: sid.
 *
 */
void hvgr_set_sid(struct hvgr_ctx *ctx, uint32_t sid);

/*
 * hvgr_config_ctx_gaf - config gaf
 *
 * @param ctx: hvgr pointer.
 * @param flag: flag.
 *
 */
void hvgr_config_ctx_gaf(struct hvgr_ctx *ctx, uint32_t flag);
uint32_t hvgr_dmd_read_gaf(struct hvgr_device *gdev);

#endif
