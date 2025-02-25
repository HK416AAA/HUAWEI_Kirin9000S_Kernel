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

#ifndef __BSP_TRANS_REPORT_H__
#define __BSP_TRANS_REPORT_H__

#include "product_config.h"
#include <linux/list.h>
#include <linux/types.h>

#define TRANS_IF_NAME_SIZE 16
#define TRANS_PKT_HDR_LEN 60

/* get pkt info in each point */
enum net_om_report_type {
    NET_OM_TYPE_PFA_DL = 0,
    NET_OM_TYPE_PFA_UL = 1,
    NET_OM_TYPE_PFA_TFT_DL = 2,
    NET_OM_TYPE_PFA_TFT_UL = 3,
    NET_OM_TYPE_USB_TX = 4,
    NET_OM_TYPE_USB_TX_CMP = 5,
    NET_OM_TYPE_USB_RX = 6,
    NET_OM_TYPE_WETH_RX = 7,
    NET_OM_TYPE_WETH_TX = 8,
    NET_OM_TYPE_MAX
};

enum net_om_report_msg_type {
    /* msgid for trans debug info for hids */
    NET_OM_MSGID_DRA = 0x9f383331,
    NET_OM_MSGID_PFA,
    NET_OM_MSGID_WAN,
    NET_OM_MSGID_WETH,
    NET_OM_MSGID_PCDEV,
    NET_OM_MSGID_GMAC,
    NET_OM_MSGID_USB,
    NET_OM_MSGID_WETH_RC,
    NET_OM_MSGID_HMI,
    /* msgid for trans pkt info */
    NET_OM_PKT_MSGID_BASE = 0x9f383340,
    NET_OM_PKT_MSGID_MAX = 0x9f383350,
};

struct net_om_report_pkt_info {
    unsigned short l3_version;
    unsigned short l4_protocol;
    unsigned int l3_id;
    unsigned int l4_seq;
    unsigned int l4_ack;
    unsigned int time_stamp;
};

struct weth_hids_trans_open_msg {
    unsigned short en_type;
    unsigned short enable;
};

struct net_om_msg_info {
    unsigned short prim_id;
    unsigned short tool_id;

    unsigned char net_if_name[TRANS_IF_NAME_SIZE];
    unsigned short len;
    unsigned short data[0];
};

struct net_om_report_default_pkt {
    struct net_om_msg_info msg_hdr;
    unsigned char data[TRANS_PKT_HDR_LEN];
};

enum net_om_hook_id {
    TRANS_HOOK_BR_PRE_ROUTING = 0x00,
    TRANS_HOOK_BR_POST_ROUTING = 0x01,
    TRANS_HOOK_BR_FORWARD = 0x02,
    TRANS_HOOK_BR_LOCAL_IN = 0x03,
    TRANS_HOOK_BR_LOCAL_OUT = 0x04,
    TRANS_HOOK_ARP_LOCAL_IN = 0x05,
    TRANS_HOOK_ARP_LOCAL_OUT = 0x06,
    TRANS_HOOK_IP4_PRE_ROUTING = 0x07,
    TRANS_HOOK_IP4_POST_ROUTING = 0x08,
    TRANS_HOOK_IP4_LOCAL_IN = 0x09,
    TRANS_HOOK_IP4_LOCAL_OUT = 0x0A,
    TRANS_HOOK_IP4_FORWARD = 0x0B,
    TRANS_HOOK_IP6_PRE_ROUTING = 0x0C,
    TRANS_HOOK_IP6_POST_ROUTING = 0x0D,
    TRANS_HOOK_IP6_LOCAL_IN = 0x0E,
    TRANS_HOOK_IP6_LOCAL_OUT = 0x0F,
    TRANS_HOOK_IP6_FORWARD = 0x10,
    TRANS_HOOK_BR_FORWARD_FLOW_CTRL = 0x11,
    TRANS_HOOK_ENUM_BUTT
};

struct net_om_report_ctx {
    unsigned long long last_jiff;
    unsigned long long last_slice;
    unsigned int buf_cnt;

    unsigned int stat_addr_err;
    unsigned int stat_addr_set_err;
    unsigned int stat_addr_protocol_err;
    unsigned int stat_addr_pkt_other;
    unsigned int stat_addr_pkt_succ;
    unsigned int stat_addr_pkt_;
    unsigned int stat_addr_trans_fail;
    unsigned int stat_alloc_fail;
    unsigned int stat_buff_err;
    unsigned int stat_buff_full;
    unsigned int stat_buff_ok;
    unsigned int stat_addr_trans_succ;
    unsigned int stat_addr_trans_byte;
    unsigned int stat_send_process;
    unsigned int stat_last_err;
    unsigned int stat_last_length;
    unsigned int stat_last_num;
    unsigned int stat_not_enable;

    unsigned int stat_rc_default_pkt;
    unsigned int stat_rc_all_pkt;
    unsigned int stat_rc_pkt_succ;
    unsigned int stat_rc_pkt_fail;
};

struct net_om_diag_info {
    unsigned int        ulModule;
    unsigned int        ulPid;
    unsigned int        ulMsgId;
    unsigned int        ulReserve;
    unsigned int        ulLength;
    void               *pData;
};

struct net_om_info {
    unsigned int ul_msg_id;
    void *data;
    unsigned int size;
    void (*func_cb)(void);
    struct list_head list;
};

#ifdef CONFIG_NET_OM /* ep */
void bsp_net_report_mark_pkt_info(unsigned long long dra_addr, enum net_om_report_type trans_type, bool is_skb,
    bool is_cache);
void bsp_net_report_set_time(void);
void bsp_net_report_enable_by_type(enum net_om_report_type trans_type, int enable);
void bsp_net_report_rc_pkt(unsigned int msg_id, void *data, unsigned int len);
int bsp_net_report_register(struct net_om_info *info);

#else
static inline void bsp_net_report_mark_pkt_info(unsigned long long dra_addr, enum net_om_report_type trans_type,
    bool is_skb, bool is_cache)
{
    return;
}
static inline void bsp_net_report_set_time(void)
{
    return;
}
static inline void bsp_net_report_enable_by_type(enum net_om_report_type trans_type, int enable)
{
    return;
}

static inline void bsp_net_report_rc_pkt(unsigned int msg_id, void *data, unsigned int len)
{
    return;
}

static inline int bsp_net_report_trans(struct net_om_diag_info *trans_info)
{
    return 0;
}

static inline int bsp_net_report_register(struct net_om_info *info)
{
    return 0;
}

#endif

#ifdef CONFIG_WETH
void bsp_net_report_enable_all_pkt(unsigned int enable);
void bsp_net_enable_hooks(void);

#else
static inline void bsp_net_report_enable_all_pkt(unsigned int enable)
{
    return;
}
static inline void bsp_net_enable_hooks(void)
{
    return;
}

#endif

#endif
