/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2019. All rights reserved.
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

#ifndef NETFILTEREX_H
#define NETFILTEREX_H

#include "product_config_ps_ap.h"
#include "network_interface.h"
#include "netfilter_ips_mntn.h"
#include "netfilter_buffer.h"
#include "netfilter_os_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NF_EXT_MEM_ALLOC(pid, size) kmalloc(size, GFP_ATOMIC)
#define NF_EXT_MEM_FREE(pid, p) kfree(p)

#define NF_EXT_BR_FORWARD_FLOW_CTRL_MASK 1U

#ifdef CONFIG_PS_AP_ENG
enum NfExtStatsEnum {
    NF_EXT_STATS_BR_FC_DROP = 0,
    NF_EXT_STATS_BR_FC_ENTER,
    NF_EXT_STATS_BUF_FULL_DROP,
    NF_EXT_STATS_PUT_BUF_FAIL,
    NF_EXT_STATS_ALLOC_MEM_FAIL,
    NF_EXT_STATS_GET_BUF_FAIL,
    NF_EXT_STATS_MDRV_RPT_FAIL,

    NF_EXT_STATS_BUT
};

typedef struct {
    uint32_t stats[NF_EXT_STATS_BUT];
} NfExtStats;

extern NfExtStats g_nfExtStats;

#define NF_EXT_STATS_INC(a, b) (g_nfExtStats.stats[(b)] += (a))
#else
#define NF_EXT_STATS_INC(a, b) do { \
} while (0)
#endif

typedef struct {
    uint32_t isBlkflag;     /* 阻塞条件 */
    uint32_t curHookOnMask; /* 当前Hook掩码 */
    uint32_t isDeviceOpen;  /* 设备是否开启的标志 */
    uint32_t omIp;
    NFEXT_Buff *ringBuf; /* 环形buff */
    struct cpumask origMask;
    struct cpumask currMask;
} NfExtEntity;

extern unsigned int NFEXT_BrPreRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_BrPostRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_BrLocalInHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_BrLocalOutHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_BrForwardHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_ArpInHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_ArpOutHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip4PreRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip4PostRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip4LocalInHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip4LocalOutHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip4ForwardHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip6PreRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip6PostRoutingHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip6LocalInHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip6LocalOutHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_Ip6ForwardHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

extern unsigned int NFEXT_BrForwardFlowCtrlHook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state);

#ifdef __cplusplus
}
#endif

#endif
