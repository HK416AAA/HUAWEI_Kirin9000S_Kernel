/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: thp channel source file
 * Author: linjianpeng <linjianpeng1@huawei.com>
 * Create: 2020-05-25
 */

#include "thp_channel.h"

#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include <huawei_platform/inputhub/thphub.h>

#include "contexthub_boot.h"
#include "contexthub_pm.h"
#include "contexthub_recovery.h"
#include "contexthub_route.h"
#include "sensor_info.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#ifdef CONFIG_CONTEXTHUB_SHMEM
#include "shmem.h"
#endif

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif

#define HWLOG_TAG thp_channel
HWLOG_REGIST();

#define MAX_PANEL_COUNT 2
#define MAIN_TOUCH_PANEL 0
#define SUB_TOUCH_PANEL 1
static thp_status_data g_thp_status;
static uint8_t g_thp_open_flag;

struct thp_cmd_map {
	unsigned int thp_ioctl_app_cmd;
	int tag;
	enum obj_sub_cmd cmd;
};

static const struct thp_cmd_map thp_cmd_map_tab[] = {
	{ THP_IOCTL_THP_DEBUG_CMD, TAG_THP, SUB_CMD_THP_DEBUG_REQ },
	{ THP_IOCTL_THP_CONFIG_CMD, TAG_THP, SUB_CMD_THP_CONFIG_REQ },
	{ THP_IOCTL_THP_OPEN_CMD, TAG_THP, SUB_CMD_THP_OPEN_REQ },
	{ THP_IOCTL_THP_CLOSE_CMD, TAG_THP, SUB_CMD_THP_CLOSE_REQ },
	{ THP_IOCTL_THP_STATUS_CMD, TAG_THP, SUB_CMD_THP_STATUS_REQ },
};

static int send_thp_switch_cmd(int tag, enum obj_sub_cmd subcmd, bool use_lock)
{
	struct write_info pkg_ap;
	pkt_parameter_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;
	enum obj_cmd cmd;
	int ret;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	cmd = (subcmd == SUB_CMD_THP_OPEN_REQ) ?
			CMD_CMN_OPEN_REQ : CMD_CMN_CLOSE_REQ;

	pkg_ap.cmd = cmd;
	pkg_ap.tag = tag;
	cpkt.subcmd = subcmd;
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = SUBCMD_LEN;
	hwlog_info("thp: %s cmd %d, tag %d sub cmd: %u\n", __func__,
		pkg_ap.cmd, pkg_ap.tag, cpkt.subcmd);

	if ((get_iom3_state() == IOM3_ST_RECOVERY) ||
		(get_iomcu_power_state() == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, NULL, true);
	if (ret < 0) {
		hwlog_err("thp: %s write cmd err\n", __func__);
		return -1;
	}

	return 0;
}

void set_multi_panel_flag(uint16_t multi_panel_support)
{
	g_thp_status.multi_panel_flag = multi_panel_support;
	hwlog_info("thp: %s, multi_panel_flag = %u\n",
		__func__, g_thp_status.multi_panel_flag);
}

int send_thp_open_cmd(void)
{
	if (g_thp_open_flag && g_thp_status.multi_panel_flag) {
		hwlog_err(" %s:g_thp_open_flag = %d\n", __func__, g_thp_open_flag);
		return 0;
	}
	g_thp_open_flag = 1;
	hwlog_info(" %s enter\n", __func__);
	return send_thp_switch_cmd(TAG_THP, SUB_CMD_THP_OPEN_REQ, true);
}

int send_thp_close_cmd(void)
{
	if (!g_thp_open_flag && g_thp_status.multi_panel_flag) {
		hwlog_err(" %s g_thp_open_flag = %d\n", __func__, g_thp_open_flag);
		return 0;
	}
	g_thp_open_flag = 0;
	hwlog_info(" %s enter\n", __func__);
	return send_thp_switch_cmd(TAG_THP, SUB_CMD_THP_CLOSE_REQ, true);
}

static int send_thp_config_cmd(unsigned int subcmd, int inpara_len,
	const char *inpara, struct read_info *pkg_mcu, bool use_lock)
{
	struct write_info pkg_ap;
	pkt_parameter_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;
	int ret;

	if (!(inpara) || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}

	memset(&pkg_ap, 0, sizeof(pkg_ap));

	if (THP_IS_INVALID_BUFF(inpara, inpara_len,
		(MAX_PKT_LENGTH - SUBCMD_LEN - (int)sizeof(struct pkt_header)))) {
		hwlog_err("%s: subcmd: %u, inpara len: %d, maxlen: 128\n",
			__func__, subcmd, inpara_len);
		return -EINVAL;
	}

	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.tag = TAG_THP;
	cpkt.subcmd = subcmd;
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = SUBCMD_LEN + inpara_len;
	if (inpara_len > 0)
		memcpy(cpkt.para, inpara, (unsigned int)inpara_len);

	hwlog_info("%s: cmd %d, tag %d, subcmd %u, para0 %u, para1 %u\n",
		__func__, pkg_ap.cmd, pkg_ap.tag, subcmd,
		(unsigned int)(cpkt.para[0]), (unsigned int)(cpkt.para[1]));

	if (get_iom3_state() != IOM3_ST_NORMAL) {
		hwlog_err("thp: %s iomcu is recovery.\n", __func__);
		return -1;
	}
	if ((get_iom3_state() == IOM3_ST_RECOVERY) ||
		(get_iomcu_power_state() == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, pkg_mcu, use_lock);
	if (ret < 0) {
		hwlog_err("%s: err. write cmd\n", __func__);
		return -1;
	}

	return 0;
}

static int send_thp_status_cmd(int inpara_len, const char *inpara)
{
	int ret;

	if (!(inpara) || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}
	if (inpara[0] >= ST_CMD_TYPE_MAX) {
		hwlog_err("%s: cmd is invalid\n", __func__);
		return -1;
	}
	ret = send_thp_config_cmd(SUB_CMD_THP_STATUS_REQ, inpara_len,
			inpara, NULL, true);
	return ret;
}

static int send_thp_screen_status_cmd(unsigned char power_on,
	unsigned int gesture_support)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	if (!g_thp_open_flag && g_thp_status.multi_panel_flag) {
		hwlog_err("%s: g_thp_open_flag = %d\n", __func__, g_thp_open_flag);
		return 0;
	}
	g_thp_status.screen_state = power_on ? TP_SWITCH_ON : TP_SWITCH_OFF;
	if (g_thp_status.screen_state == TP_SWITCH_OFF)
		g_thp_status.support_mode = gesture_support;

	/* 0:cmd type offset, 1:power status offset, 2:gesture offset */
	cmd_para[0] = ST_CMD_TYPE_SET_SCREEN_STATUS;
	cmd_para[1] = g_thp_status.screen_state;
	/* 2~5: put 4 bytes gesture_support into cmd_para from index 2 */
	cmd_para[2] = (char)(gesture_support & 0x000000ff);
	cmd_para[3] = (char)((gesture_support >> 8) & 0x000000ff);
	cmd_para[4] = (char)((gesture_support >> 16) & 0x000000ff);
	cmd_para[5] = (char)((gesture_support >> 24) & 0x000000ff);
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called\n", __func__);
	ret = send_thp_status_cmd(cmd_len, cmd_para);
	return ret;
}

static int send_thp_tui_status_cmd(unsigned char status)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	g_thp_status.tui_mode = status;
	/* 0:cmd type offset, 1:tui mode offset */
	cmd_para[0] = ST_CMD_TYPE_SET_TUI_STATUS;
	cmd_para[1] = g_thp_status.tui_mode;
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called, status = %u\n", __func__, status);
	ret = send_thp_status_cmd(cmd_len, cmd_para);
	return ret;
}

int send_active_panel_id(unsigned char panel_id)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	if (panel_id > MAX_PANEL_COUNT) {
		panel_id = MAIN_TOUCH_PANEL;
		hwlog_err("%s: invalid panel_id = %u\n", __func__, panel_id);
	}
	g_thp_status.active_panel_id = panel_id;
	/* 0:cmd type offset, 1:active panel id */
	cmd_para[0] = ST_CMD_TYPE_SET_ACTIVE_PANEL_ID;
	cmd_para[1] = g_thp_status.active_panel_id;
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called, panel_id = %u\n", __func__, panel_id);
	ret = send_thp_status_cmd(cmd_len, cmd_para);
	return ret;
}

static int send_thp_audio_status_cmd(unsigned char audio_on)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	if (audio_on == TP_SWITCH_ON) {
		g_thp_status.support_mode |= (1 << THP_RING_ON);
		g_thp_status.support_mode &= ~(1 << THP_PHONE_STATUS);
	} else if (audio_on == TP_ON_THE_PHONE) {
		g_thp_status.support_mode |= (1 << THP_PHONE_STATUS);
		g_thp_status.support_mode &= ~(1 << THP_RING_ON);
	} else {
		g_thp_status.support_mode &= ~(1 << THP_RING_ON);
		g_thp_status.support_mode &= ~(1 << THP_PHONE_STATUS);
	}

	/* 0:cmd type offset, 1:audio status offset */
	cmd_para[0] = ST_CMD_TYPE_SET_AUDIO_STATUS;
	cmd_para[1] = audio_on;
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called, status = %u\n", __func__, audio_on);
	ret = send_thp_status_cmd(cmd_len, cmd_para);
	return ret;
}

static int send_thp_phone_sleep_mode_status_cmd(unsigned int para)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	/* 0:cmd type offset, 1:sleep mode status offset */
	cmd_para[0] = ST_CMD_TYPE_SET_PHONE_SLEEP_MODE_STATUS;
	/* bit16 ~ bit23 is sleep mode status */
	cmd_para[1] = (para >> 16) & 0xFF;
	/* bit8 ~ bit15 is total time */
	cmd_para[2] = (para >> 8) & 0xFF;
	/* bit0 ~ bit7 is active time */
	cmd_para[3] = para & 0xFF;
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called, para = %x, %d,%d,%d\n", __func__,
		para, cmd_para[1], cmd_para[2], cmd_para[3]);
	ret = send_thp_status_cmd(cmd_len, cmd_para);
	return ret;
}

static int send_tp_ud_config_cmd(unsigned int subcmd, int inpara_len,
	const char *inpara, struct read_info *pkg_mcu, bool use_lock)
{
	struct write_info pkg_ap;
	pkt_parameter_req_t cpkt;
	struct pkt_header *hd = (struct pkt_header *)&cpkt;
	int ret;

	if (!(inpara) || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}

	memset(&pkg_ap, 0, sizeof(pkg_ap));

	if (THP_IS_INVALID_BUFF(inpara, inpara_len,
		(MAX_PKT_LENGTH - SUBCMD_LEN - (int)sizeof(struct pkt_header)))) {
		hwlog_err("%s: subcmd: %u, inpara len: %d, maxlen: 128\n",
			__func__, subcmd, inpara_len);
		return -EINVAL;
	}

	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.tag = TAG_TP;
	cpkt.subcmd = subcmd;
	pkg_ap.wr_buf = &hd[1];
	pkg_ap.wr_len = SUBCMD_LEN + inpara_len;
	if (inpara_len > 0)
		memcpy(cpkt.para, inpara, (unsigned int)inpara_len);

	hwlog_info("%s: cmd %d, tag %d, subcmd %u, para0 %u, para1 %u\n",
		__func__, pkg_ap.cmd, pkg_ap.tag, subcmd,
		(unsigned int)(cpkt.para[0]), (unsigned int)(cpkt.para[1]));
	if ((get_iom3_state() == IOM3_ST_RECOVERY) ||
		(get_iomcu_power_state() == ST_SLEEP))
		ret = write_customize_cmd(&pkg_ap, NULL, false);
	else
		ret = write_customize_cmd(&pkg_ap, pkg_mcu, use_lock);
	if (ret < 0) {
		hwlog_err("%s: err. write cmd\n", __func__);
		return -1;
	}

	return 0;
}

static int send_thp_multi_tp_ud_status_cmd(unsigned char power_on,
	unsigned int pannel_id)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	g_thp_status.screen_state = power_on ? TP_SWITCH_ON : TP_SWITCH_OFF;

	/* 0:cmd type offset, 1:power status offset, 2:gesture offset */
	cmd_para[0] = ST_CMD_TYPE_SET_SCREEN_STATUS;
	cmd_para[1] = g_thp_status.screen_state;
	cmd_para[2] = (char)pannel_id;
	cmd_len = THP_STATUS_CMD_PARA_LEN;
	hwlog_info("%s: called\n", __func__);
	ret = send_tp_ud_config_cmd(SUB_CMD_THP_STATUS_REQ, cmd_len, cmd_para,
		NULL, true);
	return ret;
}

int send_tp_ap_event(int inpara_len, const char *inpara, uint8_t cmd)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	if (!(inpara) || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}
	cmd_para[0] = (char)cmd;
	hwlog_info("thp: %s, cmd = %u, inpara_len = %d\n", __func__, cmd,
		inpara_len);
	if (inpara_len > (THP_STATUS_CMD_PARA_LEN - 1)) {
		hwlog_err("thp: %s, inpara_len is too large. size = %d\n",
			__func__, inpara_len);
		return -1;
	}
	memcpy((void *)&cmd_para[1], (void *)inpara, (unsigned int)inpara_len);
	cmd_len = inpara_len + 1;
	ret = send_tp_ud_config_cmd(SUB_CMD_TSA_EVENT_REQ, cmd_len,
			cmd_para, NULL, true);
	return ret;
}

int send_thp_driver_status_cmd(unsigned char status,
	unsigned int para, unsigned char cmd)
{
	int ret;

	switch (cmd) {
	case ST_CMD_TYPE_SET_SCREEN_STATUS:
		ret = send_thp_screen_status_cmd(status, para);
		break;
	case ST_CMD_TYPE_SET_TUI_STATUS:
		ret = send_thp_tui_status_cmd(status);
		break;
	case ST_CMD_TYPE_SET_AUDIO_STATUS:
		ret = send_thp_audio_status_cmd(status);
		break;
	case ST_CMD_TYPE_MULTI_TP_UD_STATUS:
		ret = send_thp_multi_tp_ud_status_cmd(status, para);
		break;
	case ST_CMD_TYPE_SET_PHONE_SLEEP_MODE_STATUS:
		ret = send_thp_phone_sleep_mode_status_cmd(para);
		break;
	default:
		hwlog_err("thp: %s unknown cmd : %u\n", __func__, cmd);
		return -1;
	}
	return ret;
}

int send_thp_ap_event(int inpara_len, const char *inpara, uint8_t cmd)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};
	int cmd_len;

	if (!(inpara) || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}
	cmd_para[0] = (char)cmd;
	hwlog_info("thp: %s, cmd = %u, inpara_len = %d\n", __func__, cmd,
		inpara_len);
	if (inpara_len > (THP_STATUS_CMD_PARA_LEN - 1)) {
		hwlog_err("thp: %s, inpara_len is too large. size = %d\n",
			__func__, inpara_len);
		return -1;
	}
	memcpy((void *)&cmd_para[1], (void *)inpara, (unsigned int)inpara_len);
	cmd_len = inpara_len + 1;
	ret = send_thp_status_cmd(cmd_len, cmd_para);

	return ret;
}

static int send_thp_algo_sync_info_normal(int inpara_len, const char *inpara)
{
	char cmd_para[THP_ALGO_SYNC_BUFF_LEN] = {0};
	int cmd_len;

	cmd_para[0] = ST_CMD_TYPE_ALGO_SYNC_EVENT;
	memcpy((void *)&cmd_para[1], (void *)inpara, (unsigned int)inpara_len);
	cmd_len = inpara_len + 1;
	return send_thp_config_cmd(SUB_CMD_THP_STATUS_REQ, cmd_len,
		cmd_para, NULL, true);
}

int send_thp_algo_sync_event(int inpara_len, const char *inpara)
{
	int ret;

	if (!inpara || (inpara_len <= 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -1;
	}
	if (inpara_len <= MAX_PKT_LENGTH - SUBCMD_LEN -
			THP_CMD_TYPE_LEN - (int)sizeof(struct pkt_header)) {
		ret = send_thp_algo_sync_info_normal(inpara_len, inpara);
		if (ret) {
			hwlog_err("%s error\n", __func__);
			return ret;
		}
	} else {
		/* only for screen on/off, support more than 128 byte */
		ret = shmem_send(TAG_THP, inpara, (unsigned int)inpara_len);
		if (ret) {
			hwlog_err("%s shmem send sync info error\n", __func__);
			return ret;
		}
	}
	hwlog_info("%s send ok\n", __func__);
	return ret;
}

int send_thp_auxiliary_data(unsigned int inpara_len, const char *inpara)
{
	int ret;

	if (!inpara || (inpara_len == 0)) {
		hwlog_err("%s para failed\n", __func__);
		return -EINVAL;
	}
	ret = shmem_send(TAG_FP_UD, inpara, inpara_len);
	if (ret) {
		hwlog_err("%s shmem send auxiliary data error\n", __func__);
		return ret;
	}
	hwlog_info("%s send ok, datalen:%u\n", __func__, inpara_len);
	return ret;
}

static int execute_thp_config_cmd(int tag, enum obj_sub_cmd subcmd,
	unsigned long arg, bool use_lock)
{
	void __user *argp = (void __user *)(uintptr_t)arg;
	int ret;
	struct thp_ioctl_para para;
	struct read_info resp_para;
	char tmp_para[MAX_PKT_LENGTH];
	int datalen;

	memset((void *)tmp_para, 0, sizeof(tmp_para));
	memset((void *)&resp_para, 0, sizeof(resp_para));

	if (copy_from_user(&para, argp, sizeof(para))) {
		hwlog_err("thp: %s copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	ret = send_thp_config_cmd(subcmd, sizeof(para.cmd_para),
			para.cmd_para, &resp_para, use_lock);

	if ((resp_para.data_length != 0) && (para.outbuf_len > 0) &&
		(para.outbuf)) {
		datalen = (para.outbuf_len < resp_para.data_length) ?
				para.outbuf_len : resp_para.data_length;
		memcpy(tmp_para, resp_para.data, (unsigned int)datalen);
		if (copy_to_user(para.outbuf, tmp_para,
			(unsigned int)datalen)) {
			hwlog_err("thp: %s copy_to_user failed\n", __func__);
			return -EFAULT;
		}
	}

	return ret;
}

static int send_thp_cmd(unsigned int cmd, unsigned long arg)
{
	int i;
	int ret;
	int len = sizeof(thp_cmd_map_tab) / sizeof(thp_cmd_map_tab[0]);

	if (get_flag_for_sensor_test()) {
		hwlog_info("thp: %s flag_for_sensor_test is enabled, cmd = %u\n",
			__func__, cmd);
		return 0;
	}
	for (i = 0 ; i < len; i++) {
		if (thp_cmd_map_tab[i].thp_ioctl_app_cmd == cmd)
			break;
	}
	if (i == len) {
		hwlog_err("thp: %s unknown cmd %u\n", __func__, cmd);
		return -EFAULT;
	}

	switch (thp_cmd_map_tab[i].cmd) {
	case SUB_CMD_THP_DEBUG_REQ:
		ret = execute_thp_config_cmd(thp_cmd_map_tab[i].tag,
			thp_cmd_map_tab[i].cmd, arg, true);
		break;
	case SUB_CMD_THP_CONFIG_REQ:
	case SUB_CMD_THP_STATUS_REQ:
		ret = execute_thp_config_cmd(thp_cmd_map_tab[i].tag,
			thp_cmd_map_tab[i].cmd, arg, true);
		break;
	case SUB_CMD_THP_OPEN_REQ:
	case SUB_CMD_THP_CLOSE_REQ:
		ret = send_thp_switch_cmd(thp_cmd_map_tab[i].tag,
			thp_cmd_map_tab[i].cmd, true);
		break;
	default:
		hwlog_err("thp: %s unknown cmd : %u\n", __func__, cmd);
		return -ENOTTY;
	}

	return ret;
}

static void enable_thp_when_recovery_iom3(void)
{
	int ret;
	char cmd_para[THP_STATUS_CMD_PARA_LEN] = {0};

	if (g_thp_status.multi_panel_flag) {
		(void)send_active_panel_id(g_thp_status.active_panel_id);
		(void)send_thp_switch_cmd(TAG_THP, SUB_CMD_THP_OPEN_REQ, true);
	}
	/* 0:cmd type offset, 1:screen status offset, 2:tui mode offset */
	cmd_para[0] = ST_CMD_TYPE_SET_RECOVER_STATUS;
	cmd_para[1] = g_thp_status.screen_state;
	cmd_para[2] = g_thp_status.tui_mode;
	/* 3~6: put 4 bytes gesture_support into cmd_para from index 3 */
	memcpy((void *)&cmd_para[3], (void *)&g_thp_status.support_mode,
		sizeof(g_thp_status.support_mode));
	hwlog_info("%s: screen_state = %u, tui_mode = %u\n", __func__,
		g_thp_status.screen_state, g_thp_status.tui_mode);
	ret = send_thp_status_cmd(THP_STATUS_CMD_PARA_LEN, cmd_para);
	if (ret)
		hwlog_err("%s: send status failed\n", __func__);
}

void disable_thp_when_sysreboot(void)
{
	hwlog_info("thp: thp state: %s\n", __func__);
}

/* read /dev/thphub, do nothing now */
static ssize_t thp_read(struct file *file, char __user *buf, size_t count,
	loff_t *pos)
{
	hwlog_info("thp: enter %s\n", __func__);
	return 0;
}

/* write to /dev/thphub, do nothing now */
static ssize_t thp_write(struct file *file, const char __user *data,
	size_t len, loff_t *ppos)
{
	hwlog_info("thp: enter %s\n", __func__);
	return 0;
}

/*
 * Description: ioctl function to /dev/thphub, do open, do config/debug
 *              cmd to thphub
 * Return:      result of ioctrl command, 0 successed, -ENOTTY failed
 */
static long thp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case THP_IOCTL_THP_DEBUG_CMD:
		hwlog_info("thp: %s  cmd: THP_IOCTL_THP_DEBUG_CMD\n", __func__);
		break;
	case THP_IOCTL_THP_CONFIG_CMD:
		hwlog_info("thp: %s cmd: THP_IOCTL_THP_CONFIG_CMD\n", __func__);
		break;
	case THP_IOCTL_THP_OPEN_CMD:
		hwlog_info("thp: %s  cmd: THP_IOCTL_THP_OPEN_CMD\n", __func__);
		break;
	case THP_IOCTL_THP_CLOSE_CMD:
		hwlog_info("thp: %s cmd: THP_IOCTL_THP_CLOSE_CMD\n", __func__);
		break;
	case THP_IOCTL_THP_STATUS_CMD:
		hwlog_info("thp: %s cmd: THP_IOCTL_THP_STATUS_CMD\n", __func__);
		break;
	default:
		hwlog_err("thp: %s unknown cmd : %u\n", __func__, cmd);
		return -ENOTTY;
	}

	return send_thp_cmd(cmd, arg);
}

/* open to /dev/thphub, do nothing now */
static int thp_open(struct inode *inode, struct file *file)
{
	hwlog_info("thp: enter %s\n", __func__);
	return 0;
}

/* release to /dev/thphub, do nothing now */
static int thp_release(struct inode *inode, struct file *file)
{
	hwlog_info("thp: %s ok\n", __func__);
	return 0;
}

static int thp_recovery_notifier(struct notifier_block *nb,
			unsigned long foo, void *bar)
{
	/* prevent access the emmc now: */
	hwlog_info("%s: %lu\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
		if (g_thp_open_flag)
			enable_thp_when_recovery_iom3();
		break;
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		break;
	default:
		hwlog_err("%s: unknown state %lu\n", __func__, foo);
		break;
	}
	hwlog_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block thp_recovery_notify = {
	.notifier_call = thp_recovery_notifier,
	.priority = -1,
};

/* file_operations to thphub */
static const struct file_operations thp_fops = {
	.owner = THIS_MODULE,
	.read = thp_read,
	.write = thp_write,
	.unlocked_ioctl = thp_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = thp_ioctl,
#endif
	.open = thp_open,
	.release = thp_release,
};

/* miscdevice to thphub */
static struct miscdevice thphub_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "thphub",
	.fops = &thp_fops,
};

/*
 * Description: apply kernel buffer, register thphub_miscdev
 * Return:     result of function, 0 successed, else false
 */
static int __init thphub_init(void)
{
	int ret;

	if (is_sensorhub_disabled())
		return -1;

	ret = misc_register(&thphub_miscdev);
	if (ret) {
		hwlog_err("%s cannot register miscdev err=%d\n", __func__, ret);
		inputhub_route_close(ROUTE_FHB_PORT);
		return ret;
	}
	register_iom3_recovery_notifier(&thp_recovery_notify);
	g_thp_status.screen_state = TP_SWITCH_ON;
	g_thp_status.tui_mode = TP_SWITCH_OFF;
	hwlog_info("%s ok\n", __func__);
	return ret;
}

/* release kernel buffer, deregister thphub_miscdev */
static void __exit thphub_exit(void)
{
	misc_deregister(&thphub_miscdev);
	hwlog_info("exit %s\n", __func__);
}

late_initcall_sync(thphub_init);
module_exit(thphub_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("THPHub driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
