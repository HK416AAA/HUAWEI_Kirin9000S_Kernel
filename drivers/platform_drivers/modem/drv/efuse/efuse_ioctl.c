/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <product_config_drv_ap.h>
#include <bsp_efuse.h>
#include <bsp_print.h>
#include <bsp_sec_call.h>
#include <securec.h>
#include <mdrv_efuse.h>
#include "efuse_plat.h"
#include "efuse_driver.h"
#include "efuse_layout.h"
#include "efuse_error.h"

#define THIS_MODU mod_efuse

#include <adrv_efuse.h>

#ifdef CONFIG_EFUSE

/* ************************************************
 函 数 名   : DRV_GET_DIEID
 功能描述   : DRV_GET_DIEID
 输入参数   : buf: data buffer
              len: length of the group
 输出参数   :
 返 回 值   : OK                (0)
              BUF_ERROR         (-55)
              LEN_ERROR         (-56)
              READ_EFUSE_ERROR  (-57)

************************************************ */
int mdrv_efuse_get_dieid(unsigned char *buf, int length)
{
    return get_efuse_dieid_value(buf, (unsigned int)length, 0);
}
EXPORT_SYMBOL(mdrv_efuse_get_dieid);

/* ************************************************
 函 数 名   : DRV_GET_CHIPID
 功能描述   : DRV_GET_CHIPID
 输入参数   : buf: data buffer
              len: length of the group
 输出参数   :
 返 回 值   : OK                (0)
              BUF_ERROR         (-55)
              LEN_ERROR         (-56)
              READ_EFUSE_ERROR  (-57)

************************************************ */
int mdrv_efuse_get_chipid(unsigned char *buf, int length)
{
    return get_efuse_chipid_value(buf, (unsigned int)length, 0);
}
int mdrv_efuse_ioctl(int cmd, int arg, unsigned char *buf, int len)
{
    efuse_print_error("It's only a stub");

    return EFUSE_OK;
}


#else /* CONFIG_EFUSE */

int mdrv_efuse_write_huk(char *buf, unsigned int len)
{
    return EFUSE_OK;
}

int mdrv_efuse_check_huk_valid(void)
{
    return EFUSE_OK;
}

int mdrv_efuse_get_dieid(unsigned char *buf, int length)
{
    return EFUSE_OK;
}

int mdrv_efuse_get_chipid(unsigned char *buf, int length)
{
    return EFUSE_OK;
}

int mdrv_efuse_ioctl(int cmd, int arg, unsigned char *buf, int len)
{
    return EFUSE_OK;
}

#endif

EXPORT_SYMBOL(mdrv_efuse_ioctl);
EXPORT_SYMBOL(mdrv_efuse_get_chipid);
