/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: ufs rpmb
 * Create: 2019-05-01
 */

#include "ufs_rpmb.h"
#include "hufs_plat.h"
#ifdef CONFIG_BOOTDEVICE
#include <linux/bootdevice/bootdevice.h>
#endif
#ifdef CONFIG_SCSI_UFS_VIRTUALIZ_SERVICE
#include <linux/init.h>
#include <linux/ufs/ufs_virtualiz_service.h>
#endif

#ifdef CONFIG_PM
/**
 * ufs_rpmb_pm_runtime_delay_enable - when rpmb_pm_work in workqueue 
 * is scheduled, after pm_runtime dalay time set 5ms
* @work: pointer to work structure
 *
 */
void ufs_rpmb_pm_runtime_delay_enable(struct work_struct *work)
{
	struct ufs_hba *hba = NULL;
	struct hufs_host *host = NULL;

	/*lint -e826*/
	hba = container_of(work, struct ufs_hba, rpmb_pm_work);
	/*lint -e826*/
	host = (struct hufs_host *)hba->priv;
	/* optimize delay time to reduce power consumption */
	msleep(100);

#ifdef CONFIG_UFS_H8_IDLE_3MS
	if (ufshcd_is_auto_hibern8_allowed(hba)) {
		if (strstarts(hba->model, UFS_MODEL_THOR935)) {
			ufshcd_set_auto_hibern8_delay(hba, UFS_AHIT_AUTOH8_OPT_TIMER);
		} else {
			ufshcd_set_auto_hibern8_delay(hba, UFS_AHIT_AUTOH8_TIMER);
		}
	} else {
		pm_runtime_set_autosuspend_delay(hba->dev, 5);
	}
#else
	if (ufshcd_is_auto_hibern8_allowed(hba))
		ufshcd_set_auto_hibern8_delay(hba, UFS_AHIT_AUTOH8_TIMER);
	else
		pm_runtime_set_autosuspend_delay(hba->dev, 5);
#endif
}

/**
 * ufs_rpmb_pm_runtime_delay_process - rpmb request issue, 
 * and pm_runtime dalay time set 5000ms, work in queue is scheduled
 * @hba: pointer to adapter instance
 *
 */
void ufs_rpmb_pm_runtime_delay_process(struct ufs_hba *hba)
{
	if (ufshcd_is_auto_hibern8_allowed(hba)) {
		if (hba->ahit != UFS_AHIT_AUTOH8_RPMB_TIMER) {
			ufshcd_set_auto_hibern8_delay(
				hba, UFS_AHIT_AUTOH8_RPMB_TIMER);
			schedule_work(&hba->rpmb_pm_work);
		}
	} else {
		if (hba->dev->power.autosuspend_delay == 5) {
			pm_runtime_set_autosuspend_delay(
				hba->dev, RPMB_ACCESS_PM_RUNTIME_DELAY_TIME);
			schedule_work(&hba->rpmb_pm_work);
		}
	}
}
#else
void ufs_rpmb_pm_runtime_delay_enable(struct work_struct *work)
{
}

void ufs_rpmb_pm_runtime_delay_process(struct ufs_hba *hba)
{
}
#endif

/**
 * ufs_get_rpmb_info - get rpmb info from device and set the rpmb config
 * @hba: pointer to adapter instance
 *
 */
void ufs_get_rpmb_info(struct ufs_hba *hba)
{
	int err;
	u8 desc_buf[QUERY_DESC_RPMB_UNIT_MAZ_SIZE] = {0};
	struct rpmb_config_info rpmb_config = {0};
	int i;
	/* get rpmb unit size */
	rpmb_config.rpmb_unit_size = MAX_RPMB_REGION_UNIT_SIZE;

	err = ufshcd_read_unit_desc_param(hba, UFS_UPIU_RPMB_WLUN, UNIT_DESC_PARAM_LEN, desc_buf,
		QUERY_DESC_RPMB_UNIT_MAZ_SIZE);
	if (err) {
		dev_err(hba->dev, "%s: Failed getting rpmb info\n", __func__);
		return;
	}
	/* get rpmb total blks */
	rpmb_config.rpmb_total_blks = ((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 0] << 56) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 1] << 48) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 2] << 40) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 3] << 32) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 4] << 24) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 5] << 16) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 6] << 8) |
	((u64)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_COUNT + 7] << 0);
	/* general rpmb_logical_block_size 8, means the size is 256Byte */
	rpmb_config.rpmb_blk_size = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_LOGICAL_BLK_SIZE];
	rpmb_config.rpmb_region_enable = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION_ENABLE];
	/* if rpmb is not support multi key, we do something.default is zero, so we set 0x1, means support 1 region */
	if (rpmb_config.rpmb_region_enable == 0x0)
		rpmb_config.rpmb_region_enable = 0x1;
	rpmb_config.rpmb_region_size[0] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION0_SIZE];
	rpmb_config.rpmb_region_size[1] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION1_SIZE];
	rpmb_config.rpmb_region_size[2] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION2_SIZE];
	rpmb_config.rpmb_region_size[3] = (u8)desc_buf[RPMB_UNIT_DESC_PARAM_RPMB_REGION3_SIZE];
	/* if rpmb is not support multi key, we do something.default is zero, so we set region 1 size is total rpmb size */
	if ((rpmb_config.rpmb_region_size[0] | rpmb_config.rpmb_region_size[1] |
		    rpmb_config.rpmb_region_size[2] | rpmb_config.rpmb_region_size[3]) == 0x0)
		rpmb_config.rpmb_region_size[0] =
			(u8)((rpmb_config.rpmb_total_blks << rpmb_config.rpmb_blk_size) /
				rpmb_config.rpmb_unit_size);

	set_rpmb_total_blks(rpmb_config.rpmb_total_blks );
	set_rpmb_blk_size(rpmb_config.rpmb_blk_size);
	set_rpmb_unit_size(rpmb_config.rpmb_unit_size);
	set_rpmb_region_enable(rpmb_config.rpmb_region_enable);
	set_rpmb_read_align(rpmb_config.rpmb_read_align);
	set_rpmb_write_align(rpmb_config.rpmb_write_align);
	for (i = 0; i < MAX_RPMB_REGION_NUM; i++)
		set_rpmb_region_size(i, rpmb_config.rpmb_region_size[i]);

	set_rpmb_config_ready_status();

	return;
}

#ifdef CONFIG_SCSI_UFS_VIRTUALIZ_SERVICE
static int rpmb_virtualiz_service_recv_process(char *buf, unsigned int buf_size)
{
	struct rpmb_config_info rpmb_config;
	int i;

	/* format send buf */
	rpmb_config.rpmb_total_blks = get_rpmb_total_blks();
	rpmb_config.rpmb_read_frame_support = get_rpmb_read_frame_support();
	rpmb_config.rpmb_write_frame_support = get_rpmb_write_frame_support();
	rpmb_config.rpmb_unit_size = get_rpmb_unit_size();
	for (i = 0; i < MAX_RPMB_REGION_NUM; i++)
		rpmb_config.rpmb_region_size[i] = get_rpmb_region_size(i);

	rpmb_config.rpmb_blk_size = get_rpmb_blk_size();
	rpmb_config.rpmb_read_align = get_rpmb_read_align();
	rpmb_config.rpmb_write_align = get_rpmb_write_align();
	rpmb_config.rpmb_region_enable = get_rpmb_region_enable();

	return ufs_virtualiz_service_send_buf((char*)(&rpmb_config),
				sizeof(struct rpmb_config_info), SHMEC_MODULE_UFS_RPMB);
}

static struct ufs_service_ops rpmb_service_ops = {
	{ SHMEC_UFS_RPMB },
	rpmb_virtualiz_service_recv_process,
};

static int __init rpmb_virtualiz_service_init(void)
{
	int ret;

	ret = ufs_virtualiz_service_register(&rpmb_service_ops, SHMEC_MODULE_UFS_RPMB);
	if (ret) {
		pr_err("%s register service fail\n", __func__);
		return ret;
	}

	return 0;
}
late_initcall(rpmb_virtualiz_service_init);
#endif
