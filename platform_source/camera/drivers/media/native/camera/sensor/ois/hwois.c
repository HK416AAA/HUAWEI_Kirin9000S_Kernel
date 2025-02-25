/*
 * hwois.c
 *
 * driver for hwois
 *
 * Copyright (c) 2014-2020 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <linux/version.h>
#include <linux/compiler.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <platform_include/camera/native/camera.h>
#include <media/v4l2-subdev.h>
#include "hwois.h"
#include "securec.h"

#define sd_to_ois(sd) container_of(sd, struct hw_ois_t, subdev)

int hw_ois_get_dt_data(struct platform_device *pdev, struct ois_t *ois)
{
	struct device_node *dev_node = pdev->dev.of_node;
	struct hw_ois_info *ois_info = NULL;
	int rc;

	ois_info = kzalloc(sizeof(struct hw_ois_info), GFP_KERNEL);
	if (!ois_info) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}
	ois->ois_info = ois_info;

	rc = of_property_read_string(dev_node, "vendor,ois-name",
		&ois_info->ois_name);
	cam_info("%s chip, ois-name %s, rc %d\n", __func__,
		ois_info->ois_name, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		goto fail;
	}

	rc = of_property_read_u32(dev_node, "vendor,ois-support",
		(u32 *)&ois_info->ois_support);
	cam_info("%s chip, ois-support %d, rc %d\n", __func__,
		ois_info->ois_support, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		goto fail;
	}

	rc = of_property_read_u32(dev_node, "hisi,slave-addr",
		&ois_info->slave_address);
	cam_info("%s chip, slave-addr 0x%x, rc %d\n", __func__,
		ois_info->slave_address, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		goto fail;
	}

	return rc;
fail:
	cam_err("%s can not read ois info exit\n", __func__);
	kfree(ois_info);
	ois_info = NULL;
	return rc;
}

int hw_ois_config(struct hw_ois_t *hw_ois, void *arg)
{
	int rc = 0;
	struct hw_ois_cfg_data *cdata = (struct hw_ois_cfg_data *)arg;

	if (!cdata) {
		cam_debug("%s, arg is NULL\n", __func__);
		return -EINVAL;
	}

	cam_debug("%s enter cfgtype=%d\n", __func__, cdata->cfgtype);

	mutex_lock(&hw_ois->lock);

	switch (cdata->cfgtype) {
	case CFG_OIS_I2C_READ:
		rc = hw_ois->intf->vtbl->ois_i2c_read(hw_ois->intf,
			arg);
		break;
	case CFG_OIS_I2C_WRITE:
		rc = hw_ois->intf->vtbl->ois_i2c_write(hw_ois->intf,
			arg);
		break;
	case CFG_OIS_GET_OIS_NAME:
		rc = strncpy_s(cdata->cfg.name,
			sizeof(cdata->cfg.name) - 1,
			hw_ois->ois_info->ois_name,
			sizeof(cdata->cfg.name) - 1);
		if (rc != 0)
			cam_err("%s ois name copy fail\n", __func__);

		break;
	case CFG_OIS_GET_SUPPORT_FLAG:
		cdata->cfg.ois_sup = hw_ois->ois_info->ois_support;
		break;
	default:
		rc = hw_ois->intf->vtbl->ois_ioctl(hw_ois->intf, arg);
		break;
	}

	mutex_unlock(&hw_ois->lock);

	return rc;
}

static long hw_ois_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
	void *arg)
{
	long rc = -EINVAL;
	struct hw_ois_t *s = NULL;

	if (!arg)
		cam_err("%s, the parameters is a null pointer", __func__);

	s = sd_to_ois(sd);
	cam_err("hw ois cmd = %x", cmd);

	switch (cmd) {
	case VIDIOC_KERNEL_OIS_CFG:
		rc = s->intf->vtbl->ois_config(s, arg);
		break;
	default:
		cam_err("%s, invalid IOCTL CMD %d\n", __func__, cmd);
		break;
	}
	return rc;
}

static int hw_ois_subdev_open(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_notice("hw_ois_sbudev open\n");
	return 0;
}

static int hw_ois_subdev_close(struct v4l2_subdev *sd,
	struct v4l2_subdev_fh *fh)
{
	cam_notice("hw_ois_sbudev close\n");
	return 0;
}

static struct v4l2_subdev_internal_ops s_subdev_internal_ops_hw_ois = {
	.open = hw_ois_subdev_open,
	.close = hw_ois_subdev_close,
};

static struct v4l2_subdev_core_ops s_subdev_core_ops_hw_ois = {
	.ioctl = hw_ois_subdev_ioctl,
};

static struct v4l2_subdev_ops s_subdev_ops_hw_ois = {
	.core = &s_subdev_core_ops_hw_ois,
};

int hw_ois_register(struct platform_device *pdev, struct hw_ois_intf_t *intf,
	struct hw_ois_info *hw_ois_info)
{
	int rc = 0;
	struct hw_ois_t *hw_ois = NULL;
	struct v4l2_subdev *subdev = NULL;

	cam_info("%s, ois register", __func__);
	if (!intf || !pdev || !hw_ois_info) {
		rc = -ENOMEM;
		cam_err("%s, the parameters is a null pointer", __func__);
		goto register_fail;
	}

	hw_ois = (struct hw_ois_t *)kzalloc(sizeof(struct hw_ois_t), GFP_KERNEL);
	if (!hw_ois) {
		rc = -ENOMEM;
		cam_err("%s, ois is null", __func__);
		goto register_fail;
	}

	subdev = &hw_ois->subdev;
	mutex_init(&hw_ois->lock);

	v4l2_subdev_init(subdev, &s_subdev_ops_hw_ois);
	subdev->internal_ops = &s_subdev_internal_ops_hw_ois;
	if (snprintf_s(subdev->name, sizeof(subdev->name),
		sizeof(subdev->name) - 1,
		"%s", hw_ois_info->ois_name) < 0)
		cam_err("%s snprint_s failed", __func__);

	subdev->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	v4l2_set_subdevdata(subdev, pdev);

	init_subdev_media_entity(subdev, CAM_SUBDEV_OIS);
	cam_cfgdev_register_subdev(subdev, CAM_SUBDEV_OIS);

	intf->subdev = subdev;
	hw_ois->intf = intf;
	hw_ois->ois_info = hw_ois_info;
	hw_ois->pdev = pdev;

register_fail:
	return rc;
}

void hw_ois_unregister(struct v4l2_subdev *subdev)
{
	struct hw_ois_t *hw_ois = sd_to_ois(subdev);

	media_entity_cleanup(&subdev->entity);
	cam_cfgdev_unregister_subdev(subdev);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	kzfree(hw_ois->ois_info);
	kzfree(hw_ois);
#else
	kfree_sensitive(hw_ois->ois_info);
	kfree_sensitive(hw_ois);
#endif
}
