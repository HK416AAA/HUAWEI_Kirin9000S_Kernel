/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Battery thermal driver
 *
 * battery_thermal.c
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/version.h>
#include <platform_include/basicplatform/linux/power/platform/coul/coul_drv.h>
#ifdef CONFIG_COUL_ALGORITHM_CORE
#include <platform_include/basicplatform/linux/power/platform/coul/coul_merge_drv.h>
#endif

struct battery_thermal_priv {
	struct thermal_zone_device *bat_thermal;
	struct thermal_zone_device *supply_avg;
	struct thermal_zone_device *supply_peak;
};

static int battery_thermal_get_temp(
	struct thermal_zone_device *thermal, int *temp)
{
	int raw_temp = 0;

	if (thermal == NULL || temp == NULL)
		return -EINVAL;
	coul_drv_battery_temperature_permille(&raw_temp);
	*temp = raw_temp; /* to fit iaware interface */

	return 0;
}

#ifdef CONFIG_COUL_ALGORITHM_CORE
static int battery0_thermal_get_temp(
	struct thermal_zone_device *thermal, int *temp)
{
	int raw_temp = 0;

	if (thermal == NULL || temp == NULL)
		return -EINVAL;
	raw_temp = coul_merge_drv_battery_temperature_raw(0);
	*temp = raw_temp; /* to fit iaware interface */

	return 0;
}

static int battery1_thermal_get_temp(
	struct thermal_zone_device *thermal, int *temp)
{
	int raw_temp = 0;

	if (thermal == NULL || temp == NULL)
		return -EINVAL;
	raw_temp = coul_merge_drv_battery_temperature_raw(1);
	*temp = raw_temp; /* to fit iaware interface */

	return 0;
}
#endif

static struct thermal_zone_device_ops battery_thermal_ops = {
	.get_temp = battery_thermal_get_temp,
};

#ifdef CONFIG_COUL_ALGORITHM_CORE
static struct thermal_zone_device_ops battery0_thermal_ops = {
	.get_temp = battery0_thermal_get_temp,
};

static struct thermal_zone_device_ops battery1_thermal_ops = {
	.get_temp = battery1_thermal_get_temp,
};
#endif

static int supply_avg_get_temp(
	struct thermal_zone_device *thermal, int *temp)
{
	int raw_temp;

	if (thermal == NULL || temp == NULL)
		return -EINVAL;
	raw_temp = coul_drv_get_polar_avg();
	*temp = raw_temp;
	return 0;
}

static struct thermal_zone_device_ops supply_avg_ops = {
	.get_temp = supply_avg_get_temp,
};

static int supply_peak_get_temp(
	struct thermal_zone_device *thermal, int *temp)
{
	int raw_temp;

	if (thermal == NULL || temp == NULL)
		return -EINVAL;
	raw_temp = coul_drv_get_polar_peak();
	*temp = raw_temp;
	return 0;
}

static struct thermal_zone_device_ops supply_peak_ops = {
	.get_temp = supply_peak_get_temp,
};

static int battery_thermal_probe(struct platform_device *pdev)
{
	struct battery_thermal_priv *priv = NULL;
	int result;

	priv = kzalloc(sizeof(struct battery_thermal_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, priv);

	priv->bat_thermal = thermal_zone_device_register(
		"bat_raw_temp", 0, 0, priv, &battery_thermal_ops, NULL, 0, 0);
	if (IS_ERR(priv->bat_thermal)) {
		result = PTR_ERR(priv->bat_thermal);
		goto register_bat_thermal_fail;
	}

#ifdef CONFIG_COUL_ALGORITHM_CORE
	priv->bat_thermal = thermal_zone_device_register(
		"bat0_raw_temp", 0, 0, priv, &battery0_thermal_ops, NULL, 0, 0);
	if (IS_ERR(priv->bat_thermal)) {
		result = PTR_ERR(priv->bat_thermal);
		goto register_bat_thermal_fail;
	}

	priv->bat_thermal = thermal_zone_device_register(
		"bat1_raw_temp", 0, 0, priv, &battery1_thermal_ops, NULL, 0, 0);
	if (IS_ERR(priv->bat_thermal)) {
		result = PTR_ERR(priv->bat_thermal);
		goto register_bat_thermal_fail;
	}
#endif

	priv->supply_avg = thermal_zone_device_register(
		"supply_avg", 0, 0, priv, &supply_avg_ops, NULL, 0, 0);
	if (IS_ERR(priv->supply_avg)) {
		result = PTR_ERR(priv->supply_avg);
		goto register_supply_avg_fail;
	}

	priv->supply_peak = thermal_zone_device_register(
		"supply_peak", 0, 0, priv, &supply_peak_ops, NULL, 0, 0);
	if (IS_ERR(priv->supply_peak)) {
		result = PTR_ERR(priv->supply_peak);
		goto register_supply_peak_fail;
	}

	return 0;

register_supply_peak_fail:
	thermal_zone_device_unregister(priv->supply_avg);
register_supply_avg_fail:
	thermal_zone_device_unregister(priv->bat_thermal);
register_bat_thermal_fail:
	kfree(priv);
	return result;
}

static int battery_thermal_remove(struct platform_device *pdev)
{
	struct battery_thermal_priv *priv = platform_get_drvdata(pdev);

	if (priv == NULL)
		return 0;

	thermal_zone_device_unregister(priv->bat_thermal);
	thermal_zone_device_unregister(priv->supply_avg);
	thermal_zone_device_unregister(priv->supply_peak);
	kfree(priv);

	return 0;
}

static const struct of_device_id battery_thermal_match[] = {
	{
		.compatible = "hisilicon,bat_thermal",
		.data = NULL
	},
	{
	}
};

MODULE_DEVICE_TABLE(of, battery_thermal_match);

static struct platform_driver battery_thermal_driver = {
	.probe = battery_thermal_probe,
	.remove = battery_thermal_remove,
	.driver = {
		.name = "battery thermal",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(battery_thermal_match),
	},
};

static int __init bat_thermal_init(void)
{
	platform_driver_register(&battery_thermal_driver);

	return 0;
}

device_initcall(bat_thermal_init);

static void __exit bat_thermal_exit(void)
{
	platform_driver_unregister(&battery_thermal_driver);
}

module_exit(bat_thermal_exit);

MODULE_DESCRIPTION("Battery Thermal driver");
MODULE_LICENSE("GPL v2");
