/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: gear ctrl implement
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <securec.h>
#include <platform_include/basicplatform/linux/hck/ufs/hck_scsi_ufs_gear_ctrl.h>
#include "ufshcd.h"
#include "hufs_plat.h"
#include "ufs_gear_ctrl.h"
#include "scsi_gear_ctrl.h"
#include "dsm_ufs.h"
#include "ufshcd_extend.h"

#define break_if_fail_else_add_ret(ret, count) \
	do {				       \
		if ((ret) < 0)		       \
			return 0;	       \
		else			       \
			(count) += (ret);      \
	} while (0)

#define UFS_CTRL_GEAR_VALUE_GEAR3 3
#define UFS_CTRL_GEAR_VALUE_GEAR4 4
#define UFS_CTRL_GEAR_VALUE_LANE1 1
#define UFS_CTRL_GEAR_VALUE_LANE2 2

static ufs_ctrl_gear_type_t g_gear_type = UFS_CTRL_GEAR_TYPE_GEAR;

struct ufs_gear_ctrl_info {
	ufs_ctrl_gear_type_t type;
	ufs_ctrl_gear_level_t level;
	uint8_t gear;
	uint8_t lane;
};

static struct ufs_gear_ctrl_info g_gear_info[] = {
	{ .type = UFS_CTRL_GEAR_TYPE_LANE,
	  .level = UFS_CTRL_GEAR_LEVEL_0,
	  .gear = UFS_CTRL_GEAR_VALUE_GEAR4,
	  .lane = UFS_CTRL_GEAR_VALUE_LANE1 },
	{ .type = UFS_CTRL_GEAR_TYPE_LANE,
	  .level = UFS_CTRL_GEAR_LEVEL_1,
	  .gear = UFS_CTRL_GEAR_VALUE_GEAR4,
	  .lane = UFS_CTRL_GEAR_VALUE_LANE2 },
	{ .type = UFS_CTRL_GEAR_TYPE_GEAR,
	  .level = UFS_CTRL_GEAR_LEVEL_0,
	  .gear = UFS_CTRL_GEAR_VALUE_GEAR3,
	  .lane = UFS_CTRL_GEAR_VALUE_LANE2 },
	{ .type = UFS_CTRL_GEAR_TYPE_GEAR,
	  .level = UFS_CTRL_GEAR_LEVEL_1,
	  .gear = UFS_CTRL_GEAR_VALUE_GEAR4,
	  .lane = UFS_CTRL_GEAR_VALUE_LANE2 },
};

static int ufshcd_gear_ctrl_fill_value_by_info(struct ufs_gear_ctrl_info *info)
{
	uint32_t i;

	if (!info || info->level > UFS_CTRL_GEAR_LEVEL_MAX || info->type > UFS_CTRL_GEAR_TYPE_MAX)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(g_gear_info); i++) {
		if (g_gear_info[i].type == info->type && g_gear_info[i].level == info->level) {
			info->gear = g_gear_info[i].gear;
			info->lane = g_gear_info[i].lane;
			return 0;
		}
	}
	return -EINVAL;
}

static int ufshcd_gear_ctrl_find_level_by_type_and_value(struct ufs_gear_ctrl_info *info)
{
	uint32_t i;

	if (!info || info->type > UFS_CTRL_GEAR_TYPE_MAX)
		return -EINVAL;

	for (i = 0; i < ARRAY_SIZE(g_gear_info); i++) {
		if (g_gear_info[i].type == info->type && g_gear_info[i].gear == info->gear &&
		    g_gear_info[i].lane == info->lane) {
			info->level = g_gear_info[i].level;
			return 0;
		}
	}
	return -EINVAL;
}

static int ufshcd_swith_gear_param_get(ufs_ctrl_gear_level_t gear_level, struct ufs_pa_layer_attr *pwr_mode)
{
	struct ufs_gear_ctrl_info info;
	int ret;

	info.type = g_gear_type;
	info.level = gear_level;
	ret = ufshcd_gear_ctrl_fill_value_by_info(&info);
	if (ret) {
		pr_err("%s, unsupport gear levle. type %d level %d\n", g_gear_type, gear_level);
		return ret;
	}

	pwr_mode->gear_rx = info.gear;
	pwr_mode->gear_tx = info.gear;
	pwr_mode->lane_rx = info.lane;
	pwr_mode->lane_tx = info.lane;

	return 0;
}

static int ufshcd_dev_read_lrb_use_rate(struct scsi_device *sdev, unsigned long *lrb_use_rate)
{
	struct ufs_hba *hba = NULL;
	unsigned long num = 0;
	unsigned long total;

	if (!sdev || !sdev->host)
		return -EINVAL;
	hba = shost_priv(sdev->host);
	if (unlikely(!hba))
		return -EINVAL;

	num = hweight64(hba->lrb_in_use);
	total = (unsigned long)hba->nutrs;
	if (num >= total)
		*lrb_use_rate = 100;
	else
		*lrb_use_rate = (num * 100) / total;

	return 0;
}

#ifdef CONFIG_HUAWEI_UFS_DSM
static void ufshcd_dss_dmd_update(struct ufs_hba *hba, u32 set_gear, int sub_err, int *sched_ret)
{
	unsigned long flags;

	spin_lock_irqsave(hba->host->host_lock, flags);
	*sched_ret = dsm_ufs_update_ufs_dynamic_switch_info(hba, g_gear_type, set_gear, sub_err,
							    DSM_UFS_DYNAMIC_SWITCH_SPEED_ERR);
	spin_unlock_irqrestore(hba->host->host_lock, flags);
	return;
}
static void ufshcd_dss_dmd_schedule(struct ufs_hba *hba, int sched_ret)
{
	if (sched_ret == DSM_UFS_DSS_SCHEDUAL)
		schedule_ufs_dsm_work(hba);
}
#else
static inline void ufshcd_dss_dmd_update(struct ufs_hba *hba, u32 set_gear, int sub_err, int *sched_ret)
{
}
static inline void ufshcd_dss_dmd_schedule(struct ufs_hba *hba, int sched_ret)
{
}
#endif

static bool ufshcd_change_gear_result_process(struct ufs_hba *hba, u32 set_gear, int result)
{
	struct ufs_gear_ctrl_info info;
	static uint8_t continues_reset_count = 0;
	int ret;
	bool need_hreset = false;
	int code;
	int sched_ret = DSM_UFS_DSS_ERR;

	if (!result) {
		code = (set_gear == UFS_CTRL_GEAR_LEVEL_MAX) ? UFS_DSS_INFO_SET_HIGH : UFS_DSS_INFO_SET_LOW;
		/* dmd1: report switch count(usually update count, only when reach time expired or count expired) */
		ufshcd_dss_dmd_update(hba, set_gear, code, &sched_ret);
		if (set_gear == UFS_CTRL_GEAR_LEVEL_MAX)
			continues_reset_count = 0;
		goto out;
	}

	info.type = g_gear_type;
	info.gear = hba->pwr_info.gear_rx;
	info.lane = hba->pwr_info.lane_rx;
	/* dmd2: report switch failure */
	ufshcd_dss_dmd_update(hba, set_gear, UFS_DSS_SWITCH_ERR, &sched_ret);
	dev_err(hba->dev, "%s: Failed to switch gear %d, err = %d\n", __func__, set_gear, result);
	dev_err(hba->dev, "gear:\t\t%d\t%d\nlane:\t\t%d\t%d\npwr mode:\t%d\t%d\n", hba->pwr_info.gear_rx,
		hba->pwr_info.gear_tx, hba->pwr_info.lane_rx, hba->pwr_info.lane_tx, hba->pwr_info.pwr_rx,
		hba->pwr_info.pwr_tx);
	ret = ufshcd_gear_ctrl_find_level_by_type_and_value(&info);
	if (ret || info.level != UFS_CTRL_GEAR_LEVEL_MAX) {
		/* Case1: unrecognize gear ctrl level. it means unexpected error occurred in power mode change.
		   Case2: current gear ctrl level not equal level max. it means "switch to high" failed.
		   In upper 2 cases, there need a hardware reset. */
		need_hreset = true;
		/* dmd3: report hardware reset event. */
		ufshcd_dss_dmd_update(hba, set_gear, UFS_DSS_RESET, &sched_ret);
		dev_err(hba->dev, "%s: reset occur. get failed code %d, cur level %d\n", __func__, ret, info.level);
		continues_reset_count++;
		/* continues rest threshold now is 3. */
		if (continues_reset_count >= 3) {
			/* dmd4: report ufs dynamic switch speed feature to be closed. */
			ufshcd_dss_dmd_update(hba, set_gear, UFS_DSS_FEATURE_CLOSE, &sched_ret);
			dev_err(hba->dev, "%s: feature closed occur.\n", __func__);
			mas_blk_disable_gear_ctrl(true);
		}
	}
out:
	ufshcd_dss_dmd_schedule(hba, sched_ret);
	return need_hreset;
}

static int ufshcd_change_gear(struct ufs_hba *hba, u32 set_gear)
{
	int ret;
	errno_t errno;
	struct ufs_pa_layer_attr pwr_info;
	unsigned long flags;
	bool need_hreset = false;

	errno = memcpy_s(&pwr_info, sizeof(struct ufs_pa_layer_attr), &hba->pwr_info, sizeof(struct ufs_pa_layer_attr));
	if (errno != EOK) {
		dev_err(hba->dev, "%s: Failed to get ori pwr para\n", __func__);
		ret = -ENOMEM;
		goto out;
	}
	ret = ufshcd_swith_gear_param_get(set_gear, &pwr_info);
	if (ret) {
		if (ret == -ECANCELED) {
			ret = 0;
			dev_err(hba->dev, "%s: No need to redo gear switch\n", __func__);
		} else {
			dev_err(hba->dev, "%s: Failed to get pwr para\n", __func__);
		}
		goto out;
	}

	spin_lock_irqsave(hba->host->host_lock, flags);
	if (hba->ufshcd_state != UFSHCD_STATE_OPERATIONAL || hba->force_reset)
		goto unlock_hostlock;
	if (ufshcd_eh_in_progress(hba))
		goto unlock_hostlock;
	spin_unlock_irqrestore(hba->host->host_lock, flags);
	mutex_lock(&hba->eh_mutex);
	if (!hba->ufs_in_err_handle) {
		down(&hba->host_sem);
		down_write(&hba->clk_scaling_lock);
		ret = ufshcd_config_pwr_mode(hba, &pwr_info);
		up_write(&hba->clk_scaling_lock);
		need_hreset = ufshcd_change_gear_result_process(hba, set_gear, ret);
		up(&hba->host_sem);
	}
	mutex_unlock(&hba->eh_mutex);
	goto out;
unlock_hostlock:
	spin_unlock_irqrestore(hba->host->host_lock, flags);
out:
	if (need_hreset)
		ufshcd_host_force_reset_sync(hba);
	return ret;
}

#define UFS_GEAR_SWITCH_LEVEL_NOT_SUPPORT_PRINT_INTERVAL_MASK 0xFFF
static int ufshcd_gear_level_get(struct scsi_device *sdev, u32 *get_gear)
{
	int ret;
	struct ufs_hba *hba = NULL;
	struct ufs_gear_ctrl_info info;
	static uint32_t unsupport_print_cnt = 0;

	if (unlikely(!sdev || !get_gear))
		return -EINVAL;

	hba = shost_priv(sdev->host);
	if (unlikely(!hba))
		return -EINVAL;

	info.type = g_gear_type;
	info.gear = hba->pwr_info.gear_rx;
	info.lane = hba->pwr_info.lane_rx;
	ret = ufshcd_gear_ctrl_find_level_by_type_and_value(&info);
	if (ret) {
		if (!(unsupport_print_cnt &
		      UFS_GEAR_SWITCH_LEVEL_NOT_SUPPORT_PRINT_INTERVAL_MASK))
			pr_err("%s, unrecoginze gear type and value. type %d, gear %d, lane %d, %d\n",
			       __func__, info.type, info.gear, info.lane,
			       g_gear_type);
		unsupport_print_cnt++;
		return -EPERM;
	}

	unsupport_print_cnt = 0;
	*get_gear = info.level;

	return 0;
}

static inline int ufshcd_gear_ctrl_inner(struct ufs_hba *hba, u32 set_gear)
{
	int ret;

	ret = ufshcd_change_gear(hba, set_gear);

	return ret;
}

static int ufshcd_gear_ctrl(struct scsi_device *sdev, u32 set_gear)
{
	int ret;
	struct ufs_hba *hba = NULL;
	u32 cur_gear;

	if (unlikely(!sdev || set_gear > UFS_CTRL_GEAR_LEVEL_MAX))
		return -EINVAL;

	hba = shost_priv(sdev->host);
	if (unlikely(!hba))
		return -EINVAL;

	ret = ufshcd_gear_level_get(sdev, &cur_gear);
	if (unlikely(ret))
		return -EINVAL;

	if (set_gear == cur_gear)
		return 0;

	ret = ufshcd_gear_ctrl_inner(hba, set_gear);
	return ret;
}

static int ufshcd_gear_level_cap_get(struct scsi_device *sdev, u32 *min_gear, u32 *max_gear)
{
	if (min_gear)
		*min_gear = UFS_CTRL_GEAR_LEVEL_MIN;
	if (max_gear)
		*max_gear = UFS_CTRL_GEAR_LEVEL_MAX;
	return 0;
}

static int ufshcd_is_gear_ctrl_support(struct ufs_hba *hba)
{
	struct hufs_host *host = NULL;

	if (!hba || !hba->max_pwr_info.is_valid)
		return -EINVAL;

	if (hba->max_pwr_info.info.gear_rx != UFS_CTRL_GEAR_VALUE_GEAR4 ||
	    hba->max_pwr_info.info.gear_tx != UFS_CTRL_GEAR_VALUE_GEAR4 ||
	    hba->max_pwr_info.info.lane_rx != UFS_CTRL_GEAR_VALUE_LANE2 ||
	    hba->max_pwr_info.info.lane_tx != UFS_CTRL_GEAR_VALUE_LANE2)
		return -EINVAL;

	host = (struct hufs_host *)hba->priv;
	if (host->caps & USE_HUFS_MPHY_TC)
		return -EPERM;
	if (!strstarts(hba->model, UFS_MODEL_THOR935))
		return -EPERM;

	return 0;
}

static void hck_gear_ctrl_adjust_capbility(struct ufs_hba *hba)
{
	if (ufshcd_is_gear_ctrl_support(hba))
		mas_blk_disable_gear_ctrl(true);
}

struct scsi_host_template_gear_ctrl_ops ufs_gear_ctrl_ops = {
	.read_lrb_use_rate = ufshcd_dev_read_lrb_use_rate,
	.gear_ctrl = ufshcd_gear_ctrl,
	.gear_level_get = ufshcd_gear_level_get,
	.gear_level_cap_get = ufshcd_gear_level_cap_get,
};

void ufshcd_gear_ctrl_op_register(struct ufs_hba *hba)
{
	struct scsi_host_template *hostt = NULL;

	if (!hba || !hba->host)
		return;

	hostt = hba->host->hostt;
	if (!hostt)
		return;

	hostt->gear_ctrl_ops = &ufs_gear_ctrl_ops;
	REGISTER_HCK_VH(scsi_dev_gear_ctrl_register, hck_scsi_dev_gear_ctrl_register);
	REGISTER_HCK_VH(ufs_gear_ctrl_adjust_capbility, hck_gear_ctrl_adjust_capbility);
	return;
}

void ufshcd_gear_ctrl_set_type(struct ufs_hba *hba, ufs_ctrl_gear_type_t type)
{
	int ret;

	if (type <= UFS_CTRL_GEAR_TYPE_MAX && g_gear_type != type) {
		ret = ufshcd_gear_ctrl_inner(hba, UFS_CTRL_GEAR_LEVEL_MAX);
		if (!ret)
			g_gear_type = type;
	}
}

ufs_ctrl_gear_type_t ufshcd_gear_ctrl_get_type(void)
{
	return g_gear_type;
}

static ssize_t hufs_gear_type_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long value;
	struct ufs_hba *hba = dev_get_drvdata(dev);

	if (kstrtoul(buf, 0, &value))
		return -EINVAL;

	ufshcd_gear_ctrl_set_type(hba, value);
	return count;
}

static ssize_t hufs_gear_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	const char *strinfo[] = { "Gear", "Lane" };
	ssize_t offset = 0;
	int ret;
	int i;

	ret = snprintf_s(buf, PAGE_SIZE - offset, PAGE_SIZE - offset - 1, "Support gear type below:\n");
	break_if_fail_else_add_ret(ret, offset);

	for (i = UFS_CTRL_GEAR_TYPE_GEAR; i <= UFS_CTRL_GEAR_TYPE_MAX; i++) {
		ret = snprintf_s(buf + offset, PAGE_SIZE - offset, PAGE_SIZE - offset - 1,
				"%d: %s\n", i, strinfo[i]);
		break_if_fail_else_add_ret(ret, offset);
	}

	ret = snprintf_s(buf + offset, PAGE_SIZE - offset, PAGE_SIZE - offset - 1, "==>> %d\n",
			     ufshcd_gear_ctrl_get_type());
	break_if_fail_else_add_ret(ret, offset);

	return offset;
}

void hufs_gear_type_sysfs_init(struct ufs_hba *hba)
{
	hba->gear_type.gear_type.show = hufs_gear_type_show;
	hba->gear_type.gear_type.store = hufs_gear_type_store;
	sysfs_attr_init(&hba->gear_type.gear_type.attr);
	hba->gear_type.gear_type.attr.name = "gear_type";
	hba->gear_type.gear_type.attr.mode = S_IRUSR | S_IRGRP | S_IWUSR;
	if (device_create_file(hba->dev, &hba->gear_type.gear_type))
		dev_err(hba->dev, "Failed to create sysfs for gear_type\n");
}

