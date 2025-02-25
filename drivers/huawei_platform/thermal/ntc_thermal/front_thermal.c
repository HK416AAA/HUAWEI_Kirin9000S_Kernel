/*
 * front_thermal.c
 *
 * front thermal for ntc module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#include <securec.h>
#include <linux/platform_device.h>

#include <huawei_platform/log/hw_log.h>
#include "thermal_common.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif

#define HWLOG_TAG watch_ntc
HWLOG_REGIST();

#define FRONT_TEMP_DEFAULT 30000

static struct thermal_priv *front_tz_info;
struct hw_thermal_class {
	struct class *thermal_class;
	struct device *temperature_device;
};

static int last_front_temp = FRONT_TEMP_DEFAULT;

static int get_front_thermal(struct thermal_zone_device *thermal, int *temp)
{
	int ret;

	if (!thermal || !temp)
		return -EINVAL;
	ret = watch_query_thermal_info(SHELL_FRONT, temp);
	if (ret) {
		hwlog_err("get front temp failed, ret %d\n", ret);
		*temp = last_front_temp;
	}
	last_front_temp = *temp;
	return 0;
}

static struct thermal_zone_device_ops front_thermal_ops = {
	.get_temp = get_front_thermal,
};

static ssize_t temp_store(struct device *dev, struct device_attribute *devattr,
			  const char *buf, size_t count)
{
	return (ssize_t)count;
}

static ssize_t temp_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	int ret;
	int temp = 0;
	struct thermal_zone_device thermal;

	if (!dev || !devattr || !buf)
		return 0;

	if (!dev->driver_data)
		return 0;

	ret = get_front_thermal(&thermal, &temp);
	if (ret)
		hwlog_err("temp_show failed, ret %d\n", ret);

	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d\n", temp);
	if (ret < 0)
		hwlog_err("snprintf_s failed, ret %d\n", ret);

	return ret;
}

static DEVICE_ATTR_RW(temp);

static struct attribute *front_attributes[] = {
	&dev_attr_temp.attr,
	NULL
};

static struct attribute_group front_attribute_group = {
	.attrs = front_attributes,
};

static int front_thermal_probe(struct platform_device *pdev)
{
	return thermal_probe(pdev, &front_tz_info,
		&front_thermal_ops, &front_attribute_group, "shell_front");
}

static int front_thermal_remove(struct platform_device *pdev)
{
	struct thermal_priv *priv = platform_get_drvdata(pdev);

	if (!priv)
		return -ENODEV;

	thermal_zone_device_unregister(priv->sensor.tz_dev);

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, priv);
	front_tz_info = NULL;

	return 0;
}

static const struct of_device_id front_thermal_match[] = {
	{
		.compatible = "huawei,front_thermal",
		.data = NULL,
	},
	{},
};

static struct platform_driver front_thermal_driver = {
	.probe = front_thermal_probe,
	.remove = front_thermal_remove,
	.driver = {
		.name = "front thermal",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(front_thermal_match),
	},
};

static int __init front_thermal_init(void)
{
	return platform_driver_register(&front_thermal_driver);
}

static void __exit front_thermal_exit(void)
{
	platform_driver_unregister(&front_thermal_driver);
}

late_initcall(front_thermal_init);
module_exit(front_thermal_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei ext_front temp driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
