/*
 * zrhung.h
 *
 * This file is the header file for zero hung
 *
 * Copyright (c) 2017-2021 Huawei Technologies Co., Ltd.
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

#ifndef _ZRHUNG_INTERFACE_H
#define _ZRHUNG_INTERFACE_H

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/shrinker.h>
#include <linux/fs.h>

#ifdef __cplusplus
extern "C" {
#endif

enum zrhung_cat_type {
	ZRHUNG_CAT_SYS,
	ZRHUNG_CAT_APPEYE,
	ZRHUNG_CAT_EVENT,
	ZRHUNG_CAT_NUM_MAX,
	ZRHUNG_CAT_XCOLLIE
};

enum zrhung_wp_id {
	ZRHUNG_WP_NONE = ZRHUNG_CAT_SYS << 8 | 0x0000,
	ZRHUNG_WP_HUNGTASK,
	ZRHUNG_WP_SCREENON,
	ZRHUNG_WP_SCREENOFF,
	ZRHUNG_WP_SR,
	ZRHUNG_WP_LMKD,
	ZRHUNG_WP_IO,
	ZRHUNG_WP_CPU,
	ZRHUNG_WP_GC,
	ZRHUNG_WP_TP,
	ZRHUNG_WP_FENCE,
	ZRHUNG_WP_SCRNONFWK,
	ZRHUNG_WP_SERVICE,
	ZRHUNG_WP_GPU,
	ZRHUNG_WP_TRANS_WIN,
	ZRHUNG_WP_FOCUS_WIN,
	ZRHUNG_WP_LCD,
	ZRHUNG_WP_RES_LEAK,
	ZRHUNG_WP_IPC_THREAD,
	ZRHUNG_WP_IPC_OBJECT,
	ZRHUNG_WP_UFS,
	ZRHUNG_WP_INIT,
	ZRHUNG_WP_VMWATCHDOG,
	ZRHUNG_WP_APPFREEZE,
	ZRHUNG_WP_APPFRZ_WARNING,
	ZRHUNG_WP_HTSK_WARNING,
	ZRHUNG_WP_FDLEAK,
	ZRHUNG_WP_CAMERA,
	ZRHUNG_WP_UPLOAD_BIGDATA,
	ZRHUNG_WP_ION,
	ZRHUNG_WP_WATCHDOG,
	ZRHUNG_WP_TEMP1,
	ZRHUNG_WP_TEMP2,
	ZRHUNG_WP_TEMP3,
	ZRHUNG_WP_SERVICE_CHAIN,
	ZRHUNG_WP_SOFT_LOCKUP,
	ZRHUNG_WP_MAX,
	APPEYE_NONE = ZRHUNG_CAT_APPEYE << 8 | 0x0000,
	APPEYE_UIP_WARNING,
	APPEYE_UIP_FREEZE,
	APPEYE_UIP_SLOW,
	APPEYE_UIP_INPUT,
	APPEYE_MTO_FREEZE,
	APPEYE_MTO_SLOW,
	APPEYE_MTO_INPUT,
	APPEYE_BF,
	APPEYE_CL,
	APPEYE_CLA,
	APPEYE_ARN,
	APPEYE_MANR,
	APPEYE_CANR,
	APPEYE_FWB_WARNING,
	APPEYE_FWB_FREEZE,
	APPEYE_NFW,
	APPEYE_TWIN,
	APPEYE_FWE,
	APPEYE_SBF,
	APPEYE_RCV,
	APPEYE_OBS,
	APPEYE_RECOVER_RESULT,
	APPEYE_MT,
	APPEYE_UIP_RECOVER,
	APPEYE_BINDER_TIMEOUT,
	APPEYE_SBF_FREEZE,
	APPEYE_SBF_RECOVER,
	APPEYE_SUIP_WARNING,
	APPEYE_SUIP_FREEZE,
	APPEYE_SUIP_INPUT,
	APPEYE_CRASH,
	APPEYE_FWB_RECOVER,
	APPEYE_TEMP1,
	APPEYE_TEMP2,
	APPEYE_TEMP3,
	APPEYE_MAX,
	ZRHUNG_EVENT_NONE = ZRHUNG_CAT_EVENT << 8 | 0x0000,
	ZRHUNG_EVENT_LONGPRESS,
	ZRHUNG_EVENT_POWERKEY,
	ZRHUNG_EVENT_HOMEKEY,
	ZRHUNG_EVENT_BACKKEY,
	ZRHUNG_EVENT_MULTIPRESS,
	ZRHUNG_EVENT_TEMP1,
	ZRHUNG_EVENT_TEMP2,
	ZRHUNG_EVENT_TEMP3,
	ZRHUNG_XCOLLIE_NONE = ZRHUNG_CAT_XCOLLIE << 8 | 0x0000,
	XCOLLIE_FWK_SERVICE,
	ZRHUNG_XCOLLIE_MAX,
	ZRHUNG_EVENT_MAX
};

#define ZRHUNG_CMD_LEN_MAX 512
#define ZRHUNG_MSG_LEN_MAX (15 * 1024)
#define ZRHUNG_CMD_INVALID 0xFF

#define ZRHUNG_CFG_VAL_LEN_MAX 512
#define ZRHUNG_CFG_CAT_NUM_MAX 200
#define ZRHUNG_CFG_ENTRY_NUM (ZRHUNG_CAT_NUM_MAX * ZRHUNG_CFG_CAT_NUM_MAX)

#define zrhung_wpid(wp) ((wp) & 0x00FF)
#define zrhung_wpcat(wp) ((wp) >> 8 & 0x00FF)

#define zrhung_wp_to_entry(wp) (zrhung_wpcat(wp) * \
	ZRHUNG_CFG_CAT_NUM_MAX + zrhung_wpid(wp))
#define RAW_EVENT_ZRHUNG 1000

#ifdef CONFIG_DFX_ZEROHUNG
int zrhung_send_event(enum zrhung_wp_id id, const char *cmd_buf,
		      const char *buf);
int zrhung_get_config(enum zrhung_wp_id id, char *data, uint32_t maxlen);
long zrhung_ioctl(struct file *file, unsigned int cmd, uintptr_t arg);
#else
static inline int zrhung_send_event(enum zrhung_wp_id id, const char *cmd_buf,
				    const char *buf)
{
	return -EFAULT;
}

static inline int zrhung_get_config(enum zrhung_wp_id id, char *data,
				    uint32_t maxlen)
{
	return -EFAULT;
}

static inline long zrhung_ioctl(struct file *file, unsigned int cmd,
				uintptr_t arg)
{
	return -EINVAL;
}
#endif

#ifdef __cplusplus
}
#endif
#endif /* _ZRHUNG_INTERFACE_H */

