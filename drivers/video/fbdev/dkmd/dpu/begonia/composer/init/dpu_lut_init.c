/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <dpu/soc_dpu_define.h>
#include <dpu/dpu_mitm.h>
#include <dkmd_cmdlist.h>
#include "dpu_comp_mgr.h"
#include <dpu/dpu_scl_lut.h>
#include "dpu_lut_init.h"
#include "dpu_comp_init.h"
#include "dpu_cmdlist.h"
#include "dkmd_object.h"
#include "dpu_config_utils.h"
#include "cmdlist_interface.h"

static struct dpu_ov_mitm* g_ov_mitm_coef_array[] = {
	&g_mitm_srgb2p3, &g_mitm_bt7092p3,
};

static void dpu_scl_write_coefs(uint32_t *cmd_item_base, uint32_t *scf_lut_tap, uint32_t tap_size)
{
	uint32_t i;

	dpu_check_and_no_retval(!cmd_item_base, err, "item base is null!");
	for (i = 0; i < (tap_size / sizeof(uint32_t)); i++)
		*(cmd_item_base + i) = scf_lut_tap[i];
}

static int32_t dpu_scf_lut_cmdlist_config(struct dpu_composer *dpu_comp,
	struct scf_lut_tap_table *tap_tlb, uint32_t scf_lut_base)
{
	/* 1. alloc cmdlist node */
	uint32_t cmdlist_id = cmdlist_create_user_client(DPU_SCENE_INITAIL, DATA_TRANSPORT_TYPE,
		scf_lut_base + tap_tlb->offset, tap_tlb->tap_size);

	/* 2. config cmdlist node */
	uint32_t *payload_addr = (uint32_t *)cmdlist_get_payload_addr(DPU_SCENE_INITAIL, cmdlist_id);
	if (unlikely(!payload_addr)) {
		cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
		cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);
		dpu_pr_err("payload_addr is null");
		return -1;
	}
	dpu_scl_write_coefs(payload_addr, tap_tlb->scf_lut_tap, tap_tlb->tap_size);

	/* 3. add comlist node to init list header tail */
	cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
	cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);

	return 0;
}

static int32_t dpu_scf_lut_init(struct dpu_composer *dpu_comp)
{
	uint32_t i, j;
	uint32_t length = 0;
	uint32_t *scf_lut_addr_tlb = NULL;

	scf_lut_addr_tlb = dpu_config_get_scf_lut_addr_tlb(&length);
	if (!scf_lut_addr_tlb)
		return -1;

	for (i = 0; i < length; i++) {
		for (j = 0; j < ARRAY_SIZE(g_scf_lut_tap_tlb); j++) {
			if (dpu_scf_lut_cmdlist_config(dpu_comp, &g_scf_lut_tap_tlb[j], scf_lut_addr_tlb[i]))
				return -1;
		}
	}
	return 0;
}

static void dpu_arsr_write_coefs(uint32_t *cmd_item_base, uint32_t *p, int32_t hv)
{
	uint32_t i;
	uint32_t arry_len = ARRAY_SIZE(g_arsr_coef_tap6);

	dpu_check_and_no_retval(!cmd_item_base, err, "item base is null!");

	for (i = 0; i < arry_len; i++) {
		*(cmd_item_base + i) = p[i];
		if (hv == HORIZONTAL)
			*(cmd_item_base + i + ARSR_LUT_H_OFFSET) = p[i];
	}
}

static int32_t dpu_arsr_lut_cmdlist_config(struct dpu_composer *dpu_comp,
	struct arsr_lut_tap_table *tap_tlb, uint32_t arsr_lut_base)
{
	/* 1. alloc cmdlist node */
	uint32_t cmdlist_id = cmdlist_create_user_client(DPU_SCENE_INITAIL, DATA_TRANSPORT_TYPE,
		arsr_lut_base + tap_tlb->offset, tap_tlb->tap_size);

	/* 2. config cmdlist node */
	uint32_t *payload_addr = (uint32_t *)cmdlist_get_payload_addr(DPU_SCENE_INITAIL, cmdlist_id);
	if (unlikely(!payload_addr)) {
		cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
		cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);
		dpu_pr_err("payload_addr is null");
		return -1;
	}
	dpu_arsr_write_coefs(payload_addr, tap_tlb->arsr_lut_tap, tap_tlb->direction);

	/* 3. add comlist node to init list header tail */
	cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
	cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);
	return 0;
}

static int32_t dpu_arsr_lut_init(struct dpu_composer *dpu_comp)
{
	int32_t i, j;
	int32_t length = 0;
	uint32_t *arsr_lut_addr_tlb = NULL;

	arsr_lut_addr_tlb = dpu_config_get_arsr_lut_addr_tlb(&length);
	if (!arsr_lut_addr_tlb)
		return -1;

	for (i = 0; i < length; i++) {
		for (j = 0; j < ARRAY_SIZE(g_arsr_lut_tap_tlb); j++) {
			if (dpu_arsr_lut_cmdlist_config(dpu_comp, &g_arsr_lut_tap_tlb[j], arsr_lut_addr_tlb[i]))
				return -1;
		}
	}
	return 0;
}

static void dpu_ov_mitm_write_coefs(char *cmd_item_base, struct dpu_ov_mitm *p)
{
	errno_t err_ret;

	err_ret = memcpy_s((struct dpu_ov_mitm*)cmd_item_base, sizeof(struct dpu_ov_mitm), p, sizeof(struct dpu_ov_mitm));
	if (err_ret != EOK)
		dpu_pr_err("memcpy_s failed");
}

static int32_t dpu_ov_mitm_coef_init(struct dpu_composer *dpu_comp)
{
	uint32_t i;

	/* 1. alloc cmdlist node */
	uint32_t cmdlist_id = cmdlist_create_user_client(DPU_SCENE_INITAIL, DATA_TRANSPORT_TYPE,
		DPU_RCH_OV_MITM_CTRL_ADDR(DPU_RCH_OV_OFFSET, 0), MITM_COEF_OFFSET * ARRAY_SIZE(g_ov_mitm_coef_array));

	/* 2. config cmdlist node */
	char *payload_addr = (char *)cmdlist_get_payload_addr(DPU_SCENE_INITAIL, cmdlist_id);
	if (unlikely(!payload_addr)) {
		cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
		cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);
		dpu_pr_err("payload_addr is null");
		return -1;
	}
	for (i = 0; i < ARRAY_SIZE(g_ov_mitm_coef_array); i++)
		dpu_ov_mitm_write_coefs(payload_addr + MITM_COEF_OFFSET * i, g_ov_mitm_coef_array[i]);

	/* 3. add comlist node to init list header tail */
	cmdlist_flush_client(DPU_SCENE_INITAIL, cmdlist_id);
	cmdlist_append_client(DPU_SCENE_INITAIL, dpu_comp->init_scene_cmdlist, cmdlist_id);

	return 0;
}

void dpu_hdr_lut_init(struct dpu_composer *dpu_comp)
{
}

int32_t dpu_lut_init(struct dpu_composer *dpu_comp)
{
	if (dpu_comp == NULL)
		return -1;

	if (dpu_scf_lut_init(dpu_comp))
		return -1;

	if (dpu_arsr_lut_init(dpu_comp))
		return -1;

	if (dpu_ov_mitm_coef_init(dpu_comp))
		return -1;

	dpu_hdr_lut_init(dpu_comp);

	return 0;
}
