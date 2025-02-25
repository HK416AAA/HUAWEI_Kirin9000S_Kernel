/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <bsp_dump.h>
#include <bsp_slice.h>
#include <bsp_om_enum.h>
#include <bsp_diag.h>
#include <mdrv_diag.h>
#include <securec.h>
#include "vdev.h"

#define THIS_MODU mod_vcom

#define MCI_DUMP_BUFFER_SIZE 0x8000
#define MCI_DUMP_CHAN_SIZE 0x200
#define MCI_DUMP_CHAN_NUM (MCI_DUMP_BUFFER_SIZE / MCI_DUMP_CHAN_SIZE)
#define MCI_DUMP_ITEM_SIZE 0x10
#define MCI_DUMP_ITEM_NUM (MCI_DUMP_CHAN_SIZE / MCI_DUMP_ITEM_SIZE)

#define MCI_HIDS_MSG_ID 0x9f383361
#define MCI_HIDS_DATA_SIZE sizeof(struct mci_hids_data)
#define MCI_HIDS_ITEM_SIZE sizeof(struct mci_hids_item)

void mci_hids_report(struct vcom_debug_stax *debug_ctx)
{
    int ret;
    unsigned char *hids_data = NULL;
    unsigned int hids_size;

    hids_data = kmalloc(MCI_HIDS_DATA_SIZE, GFP_KERNEL);
    if (hids_data == NULL) {
        VCOM_PRINT_ERROR("alloc mem fail\n");
        return;
    }

    spin_lock(&debug_ctx->hids_ctx.hids_lock);
    if (debug_ctx->hids_ctx.hids_size == 0) {
        spin_unlock(&debug_ctx->hids_ctx.hids_lock);
        kfree(hids_data);
        return;
    }

    hids_size = debug_ctx->hids_ctx.hids_size;
    ret = memcpy_s(hids_data, MCI_HIDS_DATA_SIZE, debug_ctx->hids_ctx.hids_data, debug_ctx->hids_ctx.hids_size);
    if (ret) {
        spin_unlock(&debug_ctx->hids_ctx.hids_lock);
        VCOM_PRINT_ERROR("memcpy fail\n");
        kfree(hids_data);
        return;
    }

    debug_ctx->hids_ctx.hids_size = 0;
    spin_unlock(&debug_ctx->hids_ctx.hids_lock);

    ret = bsp_diag_trans_report(MCI_HIDS_MSG_ID, DIAG_DRV_HDS_PID, hids_data, hids_size);
    if (ret != BSP_OK) {
        debug_ctx->hids_ctx.report_fail++;
    } else {
        debug_ctx->hids_ctx.report_succ++;
    }
    kfree(hids_data);
}

void mci_item_record(void *dst_buff, unsigned int dst_size, const void *src_buff, unsigned int src_size, enum vcom_buff_type type)
{
    int ret;

    memset_s(dst_buff, dst_size, 0, dst_size);
    src_size = src_size > dst_size ? dst_size : src_size;
    if (type == VCOM_USER_TYPE) {
        ret = copy_from_user(dst_buff, src_buff, src_size);
    } else if (type == VCOM_KERNEL_TYPE) {
        ret = memcpy_s(dst_buff, dst_size, src_buff, src_size);
    } else {
        VCOM_PRINT_ERROR("type: %u is error\n", type);
        return;
    }

    if (ret) {
        VCOM_PRINT_ERROR("buffer memcpy fail\n");
        return;
    }
}

void mci_hids_record(struct vcom_hd *hd, struct mci_record_info *record_info, enum vcom_buff_type type)
{
    struct vcom_debug_stax *debug_ctx = vcom_get_debug_stax();
    struct mci_hids_item *item = NULL;

    if (debug_ctx->hids_ctx.hids_data == NULL || record_info->size == 0) {
        return;
    }

    spin_lock(&debug_ctx->hids_ctx.hids_lock);
    if (debug_ctx->hids_ctx.hids_size == MCI_HIDS_DATA_SIZE) {
        spin_unlock(&debug_ctx->hids_ctx.hids_lock);
        mci_hids_report(debug_ctx);
        spin_lock(&debug_ctx->hids_ctx.hids_lock);
    }

    item  = (struct mci_hids_item *)((uintptr_t)debug_ctx->hids_ctx.hids_data + debug_ctx->hids_ctx.hids_size);
    item->channel_id = hd->dev_id;
    item->length = record_info->size;
    item->status = record_info->status;
    item->time = record_info->time;
    mci_item_record(item->msg, sizeof(item->msg), record_info->buffer, record_info->size, type);
    debug_ctx->hids_ctx.hids_size += MCI_HIDS_ITEM_SIZE;
    spin_unlock(&debug_ctx->hids_ctx.hids_lock);
}

void mci_dump_record(struct vcom_hd *hd, struct mci_record_info *record_info, enum vcom_buff_type type)
{
    struct vcom_debug_stax *debug_ctx = vcom_get_debug_stax();
    unsigned char *channel_buffer = NULL;
    struct vcom_dump_item *item = NULL;
    unsigned int dump_index;

    if (debug_ctx->dump_ctx.dump_buffer == NULL || hd->dev_id >= MCI_DUMP_CHAN_NUM || hd->dump_enable == 0) {
        return;
    }

    spin_lock(&debug_ctx->dump_ctx.dump_lock);
    dump_index = hd->dump_index;
    hd->dump_index = (hd->dump_index + 1) % MCI_DUMP_ITEM_NUM;
    spin_unlock(&debug_ctx->dump_ctx.dump_lock);

    channel_buffer = (unsigned char*)((uintptr_t)debug_ctx->dump_ctx.dump_buffer + hd->dev_id * MCI_DUMP_CHAN_SIZE);
    item = (struct vcom_dump_item *)(channel_buffer + dump_index * MCI_DUMP_ITEM_SIZE);
    item->length = record_info->size;
    item->status = record_info->status;
    item->time = record_info->time;
    mci_item_record(item->msg, sizeof(item->msg), record_info->buffer, record_info->size, type);
}

void mci_debug_record(struct vcom_hd *hd, const void *buff, unsigned int size, enum vcom_buff_type type, unsigned short status)
{
    struct mci_record_info record_info= {0};

    record_info.buffer = buff;
    record_info.size = size;
    record_info.time = bsp_get_slice_value();
    record_info.status = status;
    mci_dump_record(hd, &record_info, type);
}

void mci_dump_init(struct vcom_debug_stax *debug_ctx)
{
    spin_lock_init(&debug_ctx->dump_ctx.dump_lock);
    debug_ctx->dump_ctx.dump_buffer = bsp_dump_register_field(DUMP_MODEMAP_VCOM, "MCI", MCI_DUMP_BUFFER_SIZE, 0);
    if (debug_ctx->dump_ctx.dump_buffer == NULL) {
        VCOM_PRINT_ERROR("dump buffer alloc fail\n");
    }
}

int mci_hids_init(struct vcom_debug_stax *debug_ctx)
{
    debug_ctx->hids_ctx.hids_size = 0;
    debug_ctx->hids_ctx.hids_data = kzalloc(MCI_HIDS_DATA_SIZE, GFP_KERNEL);
    if (debug_ctx->hids_ctx.hids_data == NULL) {
        VCOM_PRINT_ERROR("hids buffer alloc fail\n");
        return -1;
    }
    spin_lock_init(&debug_ctx->hids_ctx.hids_lock);
    return 0;
}

int mci_debug_init(void)
{
    struct vcom_debug_stax *debug_ctx = vcom_get_debug_stax();

    mci_dump_init(debug_ctx);
    return mci_hids_init(debug_ctx);
}
