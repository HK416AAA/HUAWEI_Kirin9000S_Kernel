// SPDX-License-Identifier: GPL-2.0

/*
 * mcu_hc32l110_fw.c
 *
 * mcu_hc32l110 firmware driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "mcu_hc32l110.h"
#include "mcu_hc32l110_fw.h"
#include "mcu_hc32l110_i2c.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG mcu_hc32l110_fw
HWLOG_REGIST();

static int g_cur_version = 0;
static const u8 g_mcu_hc32l110_mtp_data[] = {
#include "mcu_hc32l110_fw.dat"
};

static const u8 g_mcu_hc32l110_mtp_data_xy[] = {
#include "mcu_hc32l110_fw_xy.dat"
};

#define MCU_HC32L110_MTP_VER	g_mcu_hc32l110_mtp_data[0xC0]
#define MCU_HC32L110_MTP_SIZE	ARRAY_SIZE(g_mcu_hc32l110_mtp_data)
#define MCU_HC32L110_MTP_VER_XY		g_mcu_hc32l110_mtp_data_xy[0xC0]
#define MCU_HC32L110_MTP_SIZE_XY	ARRAY_SIZE(g_mcu_hc32l110_mtp_data_xy)

static int mcu_hc32l110_fw_retry_write_bootloader(struct mcu_hc32l110_device_info *di, u8 *cmd)
{
	int i;

	/* retry max 10 times to wait i2c init ok */
	for (i = 0; i < MCU_HC32L110_FW_ACK_COUNT; i++) {
		if (!mcu_hc32l110_write_word_bootloader(di, cmd, MCU_HC32L110_FW_CMD_SIZE))
			return 0;

		power_usleep(DT_USLEEP_2MS);
	}

	return -EIO;
}

static int mcu_hc32l110_fw_write_cmd(struct mcu_hc32l110_device_info *di, u16 cmd)
{
	int i;
	u8 ack;
	u8 buf[MCU_HC32L110_FW_CMD_SIZE] = { 0 };

	/* fill cmd */
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;

	if (mcu_hc32l110_fw_retry_write_bootloader(di, buf))
		return -EIO;

	for (i = 0; i < MCU_HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_1MS);
		ack = 0;
		(void)mcu_hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_cmd:ack=%x\n", ack);
		if (ack == MCU_HC32L110_FW_ACK_VAL) {
			hwlog_info("write_cmd succ: i=%d cmd=%x ack=%x\n", i, cmd, ack);
			return 0;
		}
	}

	hwlog_err("write_cmd fail: i=%d cmd=%x\n", i, cmd);
	return -EIO;
}

static int mcu_hc32l110_fw_write_addr(struct mcu_hc32l110_device_info *di, u32 addr)
{
	int i;
	u8 ack;
	u8 buf[MCU_HC32L110_FW_ADDR_SIZE] = { 0 };

	/* fill address */
	buf[0] = addr >> 24;
	buf[1] = (addr >> 16) & 0xFF;
	buf[2] = (addr >> 8) & 0xFF;
	buf[3] = (addr >> 0) & 0xFF;
	buf[4] = buf[0];
	for (i = 1; i < MCU_HC32L110_FW_ADDR_SIZE - 1; i++)
		buf[4] ^= buf[i];

	(void)mcu_hc32l110_write_word_bootloader(di, buf, MCU_HC32L110_FW_ADDR_SIZE);
	for (i = 0; i < MCU_HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_1MS);
		ack = 0;
		(void)mcu_hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_addr:ack=%x\n", ack);
		if (ack == MCU_HC32L110_FW_ACK_VAL) {
			hwlog_info("write_addr succ: i=%d addr=%x ack=%x\n", i, addr, ack);
			return 0;
		}
	}

	hwlog_err("write_addr fail: i=%d addr=%x\n", i, addr);
	return -EIO;
}

static int mcu_hc32l110_fw_erase(struct mcu_hc32l110_device_info *di)
{
	int ret;
	int i;
	u8 ack;
	u8 buf[MCU_HC32L110_FW_ERASE_SIZE] = { 0 };

	/* special erase */
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = buf[0] ^ buf[1];

	ret = mcu_hc32l110_fw_write_cmd(di, MCU_HC32L110_FW_ERASE_CMD);
	if (ret)
		return ret;

	(void)mcu_hc32l110_write_word_bootloader(di, buf, MCU_HC32L110_FW_ERASE_SIZE);
	for (i = 0; i < MCU_HC32L110_FW_ERASE_ACK_COUNT; i++) {
		power_msleep(DT_MSLEEP_500MS, 0, NULL);
		ack = 0;
		(void)mcu_hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("erase:ack=%x\n", ack);
		if (ack == MCU_HC32L110_FW_ACK_VAL) {
			hwlog_info("erase succ: i=%d ack=%x\n", i, ack);
			return 0;
		}
	}

	hwlog_err("erase fail\n");
	return -EIO;
}

static int mcu_hc32l110_fw_write_data(struct mcu_hc32l110_device_info *di,
	const u8 *data, int len)
{
	int i;
	u8 ack;
	u8 checksum = len - 1;
	u8 buf[MCU_HC32L110_FW_PAGE_SIZE + 2] = {0};

	if ((len > MCU_HC32L110_FW_PAGE_SIZE) || (len <= 0)) {
		hwlog_err("data len illegal, len=%d\n", len);
		return -EINVAL;
	}

	/* buf content: (len of data - 1) + data + checksum */
	buf[0] = len - 1;
	for (i = 1; i <= len; i++) {
		buf[i] = data[i - 1];
		checksum ^= buf[i];
	}
	buf[len + 1] = checksum;

	(void)mcu_hc32l110_write_word_bootloader(di, buf, len + 2);
	for (i = 0; i < MCU_HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_20MS);
		ack = 0;
		(void)mcu_hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_data:ack=%x\n", ack);
		if (ack == MCU_HC32L110_FW_ACK_VAL) {
			hwlog_info("write_data succ: i=%d len=%d ack=%x\n", i, len, ack);
			return 0;
		}
	}

	hwlog_err("write_data fail\n");
	return -EIO;
}

static int mcu_hc32l110_fw_write_mtp_data(struct mcu_hc32l110_device_info *di,
	const u8 *mtp_data, int mtp_size)
{
	int ret;
	int size;
	int offset = 0;
	int remaining = mtp_size;
	u32 addr = MCU_HC32L110_FW_MTP_ADDR;

	while (remaining > 0) {
		ret = mcu_hc32l110_fw_write_cmd(di, MCU_HC32L110_FW_WRITE_CMD);
		ret += mcu_hc32l110_fw_write_addr(di, addr);
		size = (remaining > MCU_HC32L110_FW_PAGE_SIZE) ? MCU_HC32L110_FW_PAGE_SIZE : remaining;
		ret += mcu_hc32l110_fw_write_data(di, mtp_data + offset, size);
		if (ret) {
			hwlog_err("write mtp data fail\n");
			return ret;
		}

		offset += size;
		remaining -= size;
		addr += size;
	}

	return 0;
}

static void mcu_hc32l110_fw_program_begin(struct mcu_hc32l110_device_info *di)
{
	u8 boot_cmd[] = {0x0F, 'b', 'o', 'o', 't'};

	/* mcu boot0 to low, soc pull high */
	(void)gpio_direction_output(di->gpio_boot0, 1);
	power_usleep(DT_USLEEP_2MS);
	mcu_hc32l110_write_word_bootloader(di, boot_cmd, sizeof(boot_cmd));
	power_msleep(DT_MSLEEP_500MS, 0, NULL);
}

static int mcu_hc32l110_fw_program_end(struct mcu_hc32l110_device_info *di)
{
	int ret;

	/* enable pin pull low */
	(void)gpio_direction_output(di->gpio_boot0, 0);

	/* write go cmd */
	ret = mcu_hc32l110_fw_write_cmd(di, MCU_HC32L110_FW_GO_CMD);
	ret += mcu_hc32l110_fw_write_addr(di, MCU_HC32L110_FW_MTP_ADDR);

	/* wait for program jump */
	power_msleep(DT_MSLEEP_5S, 0, NULL);

	return ret;
}

static int mcu_hc32l110_fw_get_version_id(struct mcu_hc32l110_device_info *di)
{
	if (!mcu_hc32l110_fw_get_ver_id(di))
		return di->fw_ver_id;
	hwlog_info("get fwver err\n");
	return -EINVAL;
}

static int mcu_hc32l110_fw_set_update_flag(struct mcu_hc32l110_device_info *di, u8 value)
{
	return mcu_hc32l110_write_byte(di, MCU_HC32L110_FW_VER_UPDATE_REG, value);
}

static int mcu_hc32l110_fw_notify_update_result(struct mcu_hc32l110_device_info *di)
{
	int new_version_id;
	int fw_version_id;
	/* enable pin pull low */
	(void)gpio_direction_output(di->gpio_boot0, 1);
	power_usleep(DT_USLEEP_5MS);

	new_version_id = mcu_hc32l110_fw_get_version_id(di);
	if (di->fw_usage == 1) {
		fw_version_id = MCU_HC32L110_MTP_VER_XY;
	} else {
		fw_version_id = MCU_HC32L110_MTP_VER;
	}
	if (new_version_id == fw_version_id) {
		(void)gpio_direction_output(di->gpio_boot0, 0);
		mcu_hc32l110_fw_set_update_flag(di, 1);
		hwlog_info("mcu update succeed\n");
		return 0;
	} else {
		mcu_hc32l110_fw_set_update_flag(di, 0);
		hwlog_err("mcu update failed\n");
		return -EINVAL;
	}
}

static int mcu_hc32l110_fw_check_bootloader_mode(struct mcu_hc32l110_device_info *di)
{
	int ret;
	u8 ack = 0;
	u8 dat = 0;

	/* write ver cmd and wait ack */
	ret = mcu_hc32l110_fw_write_cmd(di, MCU_HC32L110_FW_GET_VER_CMD);
	if (ret) {
		hwlog_err("not work bootloader mode\n");
		return -EIO;
	}

	/* get data and wait ack */
	ret += mcu_hc32l110_read_word_bootloader(di, &ack, 1);
	ret += mcu_hc32l110_read_word_bootloader(di, &dat, 1);
	hwlog_info("mcu ack : %x ack=0x%x\n", dat, ack);
	if (ret) {
		hwlog_err("not work bootloader mode\n");
		return -EIO;
	}

	hwlog_info("work bootloader mode\n");
	return 0;
}
static int mcu_hc32l110_fw_update_mtp(struct mcu_hc32l110_device_info *di,
	const u8 *mtp_data, int mtp_size)
{
	int i;
	for (i = 0; i < MCU_HC32L110_FW_RETRY_COUNT; i++) {
		hwlog_info("mcu update begin\n");
		mcu_hc32l110_fw_program_begin(di);
		if (mcu_hc32l110_fw_check_bootloader_mode(di))
			continue;
		if (mcu_hc32l110_fw_erase(di))
			continue;
		if (mcu_hc32l110_fw_write_mtp_data(di, mtp_data, mtp_size))
			continue;
		if (mcu_hc32l110_fw_program_end(di))
			continue;
		if (mcu_hc32l110_fw_notify_update_result(di))
			continue;
		hwlog_info("mcu update end\n");
		return 0;
	}
	// /* enable pin pull low */
	(void)gpio_direction_output(di->gpio_boot0, 0);
	return -EINVAL;
}

int mcu_hc32l110_fw_get_dev_id(struct mcu_hc32l110_device_info *di)
{
	u8 id = 0;
	int ret;
	ret = mcu_hc32l110_read_byte(di, MCU_HC32L110_FW_DEV_ID_REG, &id);
	if (ret)
		return ret;

	di->fw_dev_id = id;
	hwlog_info("fw_dev_id: [%x]=0x%x\n", MCU_HC32L110_FW_DEV_ID_REG, id);
	return 0;
}

int mcu_hc32l110_fw_get_ver_id(struct mcu_hc32l110_device_info *di)
{
	u8 id = 0;
	int ret;

	ret = mcu_hc32l110_read_byte(di, MCU_HC32L110_FW_VER_ID_REG, &id);
	if (ret)
		return ret;

	di->fw_ver_id = id;
	hwlog_info("fw_ver_id: [%x]=0x%x\n", MCU_HC32L110_FW_VER_ID_REG, id);
	return 0;
}

int mcu_hc32l110_fw_get_boot_ver_id(struct mcu_hc32l110_device_info *di)
{
	u8 id = 0;
	int ret;

	ret = mcu_hc32l110_read_byte(di, MCU_HC32L110_FW_BOOT_VER_ID_REG, &id);
	if (ret)
		return ret;

	di->fw_boot_ver_id = id;
	hwlog_info("fw_boot_ver_id: [%x]=0x%x\n", MCU_HC32L110_FW_BOOT_VER_ID_REG, id);
	return 0;
}

int mcu_hc32l110_fw_update_empty_mtp(struct mcu_hc32l110_device_info *di)
{
	int ret;
	if (!di)
		return -EINVAL;
	ret = mcu_hc32l110_fw_check_bootloader_mode(di);
	if (ret)
		return ret;
	if (di->fw_usage == 1)
		return mcu_hc32l110_fw_update_mtp(di, g_mcu_hc32l110_mtp_data_xy, MCU_HC32L110_MTP_SIZE_XY);
	return mcu_hc32l110_fw_update_mtp(di, g_mcu_hc32l110_mtp_data, MCU_HC32L110_MTP_SIZE);
}

static int mcu_hc32l110_fw_from_bootloader(struct mcu_hc32l110_device_info *di)
{
	int ret;
	int i;
	for (i = 0; i < MCU_HC32L110_FW_RETRY_COUNT; i++) {
		ret = mcu_hc32l110_fw_check_bootloader_mode(di);
		if (!ret) {
			hwlog_err("update check, now work in bootloader mode\n");
			if (di->fw_usage == 1) {
				ret = mcu_hc32l110_fw_update_mtp(di, g_mcu_hc32l110_mtp_data_xy, MCU_HC32L110_MTP_SIZE_XY);
			} else {
				ret = mcu_hc32l110_fw_update_mtp(di, g_mcu_hc32l110_mtp_data, MCU_HC32L110_MTP_SIZE);
			}
			if (!ret) {
				hwlog_err("update succeed from bootloader mode\n");
				return 0;
			}
		}
	}
	hwlog_err("now work not in bootloader mode, can never update\n");
	return -EINVAL;
}

int mcu_hc32l110_fw_update_latest_mtp(struct mcu_hc32l110_device_info *di)
{
	int fw_version_id;
	int fw_version_size;
	const u8 *fw_data;
	g_cur_version = mcu_hc32l110_fw_get_version_id(di);
	if ((g_cur_version == -EINVAL) || (di->fw_ver_id == 0) || (di->fw_ver_id == 0xFF))
		return mcu_hc32l110_fw_from_bootloader(di);

	if (di->fw_usage == 1) {
		fw_version_id = MCU_HC32L110_MTP_VER_XY;
		fw_version_size = MCU_HC32L110_MTP_SIZE_XY;
		fw_data = g_mcu_hc32l110_mtp_data_xy;
	} else {
		fw_version_id = MCU_HC32L110_MTP_VER;
		fw_version_size = MCU_HC32L110_MTP_SIZE;
		fw_data = g_mcu_hc32l110_mtp_data;
	}

	if (g_cur_version >= fw_version_id) {
		hwlog_info("no need update_latest_mtp: ver_id=%x mtp_ver=%x\n", di->fw_ver_id, fw_version_id);
		return 0;
	}
	hwlog_info("update: ver %02X -> %02X\n", di->fw_ver_id, fw_version_id);
	return mcu_hc32l110_fw_update_mtp(di, fw_data, fw_version_size);
}

int mcu_hc32l110_fw_update_online_mtp(struct mcu_hc32l110_device_info *di,
	u8 *mtp_data, int mtp_size, int ver_id)
{
	int ret;

	if (mcu_hc32l110_fw_get_ver_id(di))
		return -EINVAL;

	if (di->fw_ver_id >= ver_id)
		return 0;

	hwlog_info("need update mtp: ver_id=%x mtp_ver=%x\n", di->fw_ver_id, ver_id);
	ret = mcu_hc32l110_fw_update_mtp(di, mtp_data, mtp_size);
	ret += mcu_hc32l110_fw_get_ver_id(di);
	return ret;
}
