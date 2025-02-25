/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
 */

#ifndef RNIC_VNET_DPL_I_H
#define RNIC_VNET_DPL_I_H

#include <linux/skbuff.h>
#include <linux/device.h>
#include <linux/jump_label.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_RNIC_DPL
extern struct static_key_false rnic_diag_conn;

void __rnic_vnet_dpl_tx(const struct sk_buff *skb);
void __rnic_vnet_dpl_rx(const struct sk_buff *skb);
void __rnic_vnet_dpl_rx_cmplt(void);

static inline void rnic_vnet_dpl_tx(const struct sk_buff *skb)
{
	if (static_branch_unlikely(&rnic_diag_conn))
		__rnic_vnet_dpl_tx(skb);
}

static inline void rnic_vnet_dpl_rx(const struct sk_buff *skb)
{
	if (static_branch_unlikely(&rnic_diag_conn))
		__rnic_vnet_dpl_rx((skb));
}

static inline void rnic_vnet_dpl_rx_cmplt(void)
{
	if (static_branch_unlikely(&rnic_diag_conn))
		__rnic_vnet_dpl_rx_cmplt();
}

void rnic_vnet_dpl_init(struct device *dev);
void rnic_vnet_dpl_exit(void);
#else
static inline void rnic_vnet_dpl_tx(const struct sk_buff *skb) {}
static inline void rnic_vnet_dpl_rx(const struct sk_buff *skb) {}
static inline void rnic_vnet_dpl_rx_cmplt(void) {}
static inline void rnic_vnet_dpl_init(struct device *dev) {}
static inline void rnic_vnet_dpl_exit(void) {}
#endif

#ifdef __cplusplus
}
#endif

#endif /* RNIC_VNET_DPL_I_H */
