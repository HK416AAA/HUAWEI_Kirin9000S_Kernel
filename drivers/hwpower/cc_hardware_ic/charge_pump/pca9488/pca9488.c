// SPDX-License-Identifier: GPL-2.0
/*
 * pca9488.c
 *
 * charge-pump pca9488 driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include "pca9488.h"
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_common.h>

#define HWLOG_TAG cp_pca9488
HWLOG_REGIST();

static int pca9488_read_byte(struct pca9488_dev_info *di, u8 reg, u8 *data)
{
	int i;

	if (!di) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_read_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int pca9488_write_byte(struct pca9488_dev_info *di, u8 reg, u8 data)
{
	int i;

	if (!di) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_write_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int pca9488_write_mask(struct pca9488_dev_info *di, u8 reg, u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 val = 0;

	ret = pca9488_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((value << shift) & mask);

	return pca9488_write_byte(di, reg, val);
}

static int pca9488_read_mask(struct pca9488_dev_info *di, u8 reg, u8 mask, u8 shift, u8 *value)
{
	int ret;
	u8 val = 0;

	ret = pca9488_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*value = val;

	return ret;
}

static int pca9488_config_switching_frequency(struct pca9488_dev_info *di)
{
	u8 value;
	unsigned int freq;
	int ret;

	if (!di) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	if (!di->cp_freq)
		return 0;

	if (di->cp_freq < PCA9488_FSW_LOW)
		freq = PCA9488_FSW_LOW;
	else if (di->cp_freq > PCA9488_FSW_HIGH)
		freq = PCA9488_FSW_HIGH;
	else
		freq = di->cp_freq;
	value = (u8)((freq - PCA9488_FSW_LOW) / PCA9488_FSW_UNIT);
	ret = pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG0,
		PCA9488_FSW_CFG_MASK, PCA9488_FSW_CFG_SHIFT, value);
	hwlog_info("pca9488_config_switching_frequency %d 0x%x\n", freq, value);

	return ret;
}

static int pca9488_chip_init(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_write_byte(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_DEV_CTRL_REG0_VAL);
	ret += pca9488_write_byte(di, PCA9488_HV_SC_CTRL_REG1,
		PCA9488_HV_SC_CTRL_REG1_VAL);
	ret += pca9488_write_byte(di, PCA9488_TRACK_CTRL_REG,
		PCA9488_TRACK_CTRL_VAL);
	ret += pca9488_config_switching_frequency(di);

	return ret;
}

static int pca9488_reverse_chip_init(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	power_usleep(DT_USLEEP_10MS);
	ret = pca9488_write_mask(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_INFET_EN_MASK, PCA9488_INFET_EN_SHIFT,
		PCA9488_INFET_EN);
	if (ret) {
		hwlog_err("init reverse chip failed\n");
		return ret;
	}

	return 0;
}

static bool pca9488_is_cp_open(void *dev_data)
{
	int ret;
	u8 status = 0;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_read_mask(di, PCA9488_HV_SC_STS_REG0,
		PCA9488_SWITCHING_EN_MASK, PCA9488_SWITCHING_EN_SHIFT,
		&status);
	if (!ret && status)
		return true;

	return false;
}

static bool pca9488_is_bp_open(void *dev_data)
{
	int ret;
	u8 status = 0;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_read_mask(di, PCA9488_HV_SC_STS_REG0,
		PCA9488_BYPASS_EN_MASK, PCA9488_BYPASS_EN_SHIFT,
		&status);
	hwlog_info("[is_bp_open] status=%u\n", status);
	if (!ret && status)
		return true;

	return false;
}

static int pca9488_set_bp_mode(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;
	int retry_time = CP_SET_BP_RETRY_TIMES;

	do {
		(void)pca9488_chip_init(dev_data);
		ret = pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG0,
			PCA9488_SC_OP_MODE_MASK, PCA9488_SC_OP_MODE_SHIFT,
			PCA9488_SC_OP_BP);
		if (ret) {
			hwlog_err("set bp failed\n");
			return ret;
		}
		power_msleep(DT_MSLEEP_150MS, DT_MSLEEP_25MS, wlrx_msleep_exit);
	} while (!pca9488_is_bp_open(dev_data) && (retry_time-- > 0) && (!wlrx_msleep_exit()));

	return 0;
}

static int pca9488_set_cp_mode(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG0,
		PCA9488_SC_OP_MODE_MASK, PCA9488_SC_OP_MODE_SHIFT,
		PCA9488_SC_OP_2TO1);
	if (ret) {
		hwlog_err("set 2:1 cp failed\n");
		return ret;
	}

	return 0;
}

static int pca9488_set_reverse_bp2cp_mode(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_write_mask(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_INFET_EN_MASK, PCA9488_INFET_EN_SHIFT,
		PCA9488_INFET_EN);
	ret += pca9488_write_mask(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_INFET_EN_MASK, PCA9488_INFET_EN_SHIFT,
		PCA9488_INFET_DIS);
	ret += pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG0,
		PCA9488_SC_OP_MODE_MASK, PCA9488_SC_OP_MODE_SHIFT,
		PCA9488_SC_OP_1TO2);
	if (ret) {
		hwlog_err("set 1:2 cp mode failed\n");
		return ret;
	}

	msleep(50); /* 50ms: delay for entering sc mode */
	ret = pca9488_write_mask(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_INFET_EN_MASK, PCA9488_INFET_EN_SHIFT,
		PCA9488_INFET_EN);
	if (ret) {
		hwlog_err("enable 1:2 mode failed\n");
		return ret;
	}

	hwlog_info("set rev 1:2 cp mode succ\n");
	return 0;
}

static int pca9488_set_reverse_cp_mode(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	ret = pca9488_write_mask(di, PCA9488_DEV_CTRL_REG0,
		PCA9488_INFET_EN_MASK, PCA9488_INFET_EN_SHIFT,
		PCA9488_INFET_DIS);
	power_usleep(DT_MSLEEP_20MS);
	ret += pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG1, PCA9488_HV_SC_CTRL_MASK,
		PCA9488_HV_SC_CTRL_SHIFT, PCA9488_HV_SC_CTRL_VAL);
	ret += pca9488_write_mask(di, PCA9488_TRACK_CTRL_REG, PCA9488_TRACK_CTRL_TIME_MASK,
		PCA9488_TRACK_CTRL_TIME_SHIFT, PCA9488_TRACK_CTRL_TIME_VAL);
	ret += pca9488_write_byte(di, PCA9488_ENABLE_HIDDEN_REG,
		PCA9488_ENABLE_HIDDEN_VAL);
	ret += pca9488_write_byte(di, PCA9488_DRIVE_CAPABILITY_REG,
		PCA9488_DRIVE_CAPABILITY_VAL);
	ret += pca9488_write_byte(di, PCA9488_INT_DEV0_MASK_REG,
		PCA9488_INT_DEV0_MASK_VAL);
	ret += pca9488_write_byte(di, PCA9488_INT_DEV1_MASK_REG,
		PCA9488_INT_DEV1_MASK_EN);
	ret += pca9488_write_byte(di, PCA9488_INT_HV_SC0_MASK_REG,
		PCA9488_INT_HV_SC0_MASK_VAL);
	ret += pca9488_write_byte(di, PCA9488_INT_HV_SC1_MASK_REG,
		PCA9488_INT_HV_SC1_MASK_VAL);
	power_usleep(DT_USLEEP_10MS);
	ret += pca9488_write_mask(di, PCA9488_HV_SC_CTRL_REG0,
		PCA9488_SC_OP_MODE_MASK, PCA9488_SC_OP_MODE_SHIFT,
		PCA9488_SC_OP_1TO2);
	if (ret) {
		hwlog_err("set 1:2 cp failed\n");
		return ret;
	}

	msleep(40); /* 40ms: delay for entering sc mode */
	return 0;
}

static int pca9488_reverse_cp_chip_init(void *dev_data, bool enable)
{
	int ret;

	if (!enable)
		return 0;

	ret = pca9488_set_reverse_cp_mode(dev_data);
	ret += pca9488_reverse_chip_init(dev_data);

	return ret;
}

static void pca9488_irq_work(struct work_struct *work)
{
	hwlog_info("[irq_work] ++\n");
}

static irqreturn_t pca9488_irq_handler(int irq, void *p)
{
	struct pca9488_dev_info *di = p;

	if (!di) {
		hwlog_err("di is null\n");
		return IRQ_NONE;
	}

	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static int pca9488_irq_init(struct pca9488_dev_info *di, struct device_node *np)
{
	int ret;

	INIT_WORK(&di->irq_work, pca9488_irq_work);

	if (power_gpio_config_interrupt(np,
		"gpio_int", "pca9488_int", &di->gpio_int, &di->irq_int))
		return 0;

	ret = request_irq(di->irq_int, pca9488_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "pca9488_irq", di);
	if (ret) {
		hwlog_err("could not request pca9488_irq\n");
		di->irq_int = -EINVAL;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	return 0;
}

static int pca9488_device_check(void *dev_data)
{
	int ret;
	struct pca9488_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	if (di->post_probe_done)
		return 0;

	ret = pca9488_read_byte(di, PCA9488_DEVICE_ID_REG, &di->chip_id);
	if (ret) {
		hwlog_err("get chip_id failed\n");
		return ret;
	}

	hwlog_info("chip_id=0x%x\n", di->chip_id);
	return 0;
}

static int pca9488_post_probe(void *dev_data)
{
	struct pca9488_dev_info *di = dev_data;
	struct power_devices_info_data *power_dev_info = NULL;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	if (di->post_probe_done)
		return 0;

	if (pca9488_irq_init(di, di->client->dev.of_node)) {
		hwlog_err("irq init failed\n");
		return -EPERM;
	}

	power_dev_info = power_devices_info_register();
	if (power_dev_info) {
		power_dev_info->dev_name = di->dev->driver->name;
		power_dev_info->dev_id = di->chip_id;
		power_dev_info->ver_id = 0;
	}

	di->post_probe_done = true;
	hwlog_info("post_probe done\n");

	return 0;
}

static struct charge_pump_ops pca9488_ops = {
	.cp_type = CP_TYPE_MAIN,
	.chip_name = "pca9488",
	.dev_check = pca9488_device_check,
	.post_probe = pca9488_post_probe,
	.chip_init = pca9488_chip_init,
	.rvs_chip_init = pca9488_reverse_chip_init,
	.rvs_cp_chip_init = pca9488_reverse_cp_chip_init,
	.set_bp_mode = pca9488_set_bp_mode,
	.set_cp_mode = pca9488_set_cp_mode,
	.set_rvs_bp_mode = pca9488_set_bp_mode,
	.set_rvs_cp_mode = pca9488_set_reverse_cp_mode,
	.is_cp_open = pca9488_is_cp_open,
	.is_bp_open = pca9488_is_bp_open,
	.set_rvs_bp2cp_mode = pca9488_set_reverse_bp2cp_mode,
};

static struct charge_pump_ops pca9488_aux_ops = {
	.cp_type = CP_TYPE_AUX,
	.chip_name = "pca9488_aux",
	.dev_check = pca9488_device_check,
	.post_probe = pca9488_post_probe,
	.chip_init = pca9488_chip_init,
	.rvs_chip_init = pca9488_reverse_chip_init,
	.rvs_cp_chip_init = pca9488_reverse_cp_chip_init,
	.set_bp_mode = pca9488_set_bp_mode,
	.set_cp_mode = pca9488_set_cp_mode,
	.set_rvs_bp_mode = pca9488_set_bp_mode,
	.set_rvs_cp_mode = pca9488_set_reverse_cp_mode,
	.is_cp_open = pca9488_is_cp_open,
	.is_bp_open = pca9488_is_bp_open,
	.set_rvs_bp2cp_mode = pca9488_set_reverse_bp2cp_mode,
};

static void pca9488_parse_dts(struct device_node *np, struct pca9488_dev_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"cp_freq", (u32 *)&di->cp_freq, 0);
}

static int pca9488_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct pca9488_dev_info *di = NULL;
	struct device_node *np = NULL;

	if (!client || !client->dev.of_node || !id)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &client->dev;
	np = client->dev.of_node;
	di->client = client;
	i2c_set_clientdata(client, di);

	pca9488_parse_dts(np, di);

	if (id->driver_data == CP_TYPE_MAIN) {
		pca9488_ops.dev_data = (void *)di;
		ret = charge_pump_ops_register(&pca9488_ops);
	} else {
		pca9488_aux_ops.dev_data = (void *)di;
		ret = charge_pump_ops_register(&pca9488_aux_ops);
	}
	if (ret)
		goto ops_register_fail;

	return 0;

ops_register_fail:
	i2c_set_clientdata(client, NULL);
	devm_kfree(&client->dev, di);
	return ret;
}

static int pca9488_remove(struct i2c_client *client)
{
	struct pca9488_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	i2c_set_clientdata(client, NULL);
	return 0;
}

MODULE_DEVICE_TABLE(i2c, charge_pump_pca9488);
static const struct of_device_id pca9488_of_match[] = {
	{
		.compatible = "charge_pump_pca9488",
		.data = NULL,
	},
	{
		.compatible = "pca9488_aux",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id pca9488_i2c_id[] = {
	{ "charge_pump_pca9488", CP_TYPE_MAIN },
	{ "pca9488_aux", CP_TYPE_AUX },
	{},
};

static struct i2c_driver pca9488_driver = {
	.probe = pca9488_probe,
	.remove = pca9488_remove,
	.id_table = pca9488_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "charge_pump_pca9488",
		.of_match_table = of_match_ptr(pca9488_of_match),
	},
};

static int __init pca9488_init(void)
{
	return i2c_add_driver(&pca9488_driver);
}

static void __exit pca9488_exit(void)
{
	i2c_del_driver(&pca9488_driver);
}

rootfs_initcall(pca9488_init);
module_exit(pca9488_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("pca9488 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
