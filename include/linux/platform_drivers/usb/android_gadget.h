/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: USB gadget function interface
 * Create: 2019
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#ifndef __ANDROID_GADGET_H
#define __ANDROID_GADGET_H

#include <linux/dcache.h>
#include <linux/device.h>
#include <linux/fs.h>

extern int configfs_unlink(struct inode *dir, struct dentry *dentry);

#ifdef CONFIG_USB_CONFIGFS_UEVENT

struct device *create_function_device(char *name);

#endif
#endif
