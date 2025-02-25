/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
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

#include "bsp_dump.h"
#include "osl_irq.h"
#include "securec.h"


#ifndef ENABLE_BUILD_OM
void mdrv_om_system_error(int mod_id, int arg1, int arg2, char *arg3, int arg3_len)
{
    return;
}

u8 *mdrv_om_register_field(u32 field_id, char *field_name, u32 length, u16 version)
{
    return 0;
}

u8 *mdrv_om_get_field_addr(u32 field_id)
{
    return 0;
}

dump_handle mdrv_om_register_callback(char *name, dump_hook p_func)
{
    return 0;
}

#else
void mdrv_om_system_error(int mod_id, int arg1, int arg2, char *arg3, int arg3_len)
{
    system_error(mod_id, arg1, arg2, (char *)arg3, arg3_len);
}

u8 *mdrv_om_register_field(u32 field_id, char *field_name, u32 length, u16 version)
{
    return bsp_dump_register_field(field_id, field_name, length, version);
}

u8 *mdrv_om_get_field_addr(u32 field_id)
{
    return bsp_dump_get_field_addr(field_id);
}

dump_handle mdrv_om_register_callback(char *name, dump_hook p_func)
{
    return bsp_dump_register_hook(name, p_func);
}

EXPORT_SYMBOL_GPL(mdrv_om_register_field);
EXPORT_SYMBOL_GPL(mdrv_om_get_field_addr);
EXPORT_SYMBOL_GPL(mdrv_om_register_callback);
EXPORT_SYMBOL_GPL(mdrv_om_system_error);

#endif

