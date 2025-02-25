/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: define macros and structs for emcom_mproute.c
 * Author: lijiqing jiqing.li@huawei.com
 * Create: 2021-10-10
 */
#ifndef __EMCOM_MPROUTE_H__
#define __EMCOM_MPROUTE_H__

#include <linux/types.h>
#include "emcom/mpflow.h"

#define PATH0_RATIO 0
#define PATH1_RATIO 1
#define PATH2_RATIO 2
#define PATH3_RATIO 3

#define MPROUTE_SK_MARK_MIN 2000
#define MPROUTE_SK_MARK_MAX 2100
#define MPROUTE_FWMARK_CONTROL_MASK 0xFFFF0000
#define MPROUTE_FWMARK_NEW_MASK 0xFFFF

struct mproute_xengine_head {
	unsigned short type;
	unsigned short len;
};

enum MpRouteReqMsgType {
	MPROUTE_UPDATE_POLICY,
	MPROUTE_INIT_BIND_CONFIG,
	MPROUTE_RESET_FLOW,
	MPROUTE_RESET_ALL_FLOW,
	MPROUTE_NOTIFY_NETWORK_INFO,
	MPROUTE_STOP,
	MPROUTE_CLEAR_POLICY,
	MPROUTE_NOTIFY_MPDNS_CFG,
	MPROUTE_NOTIFY_MPDNS_RESULT,
	MPROUTE_NOTIFY_CLEAR_STATE,
	MPROUTE_CMD_NUM,
};

typedef struct {
	uint32_t uid;
	uint32_t best_mark; // the best qos mark
	uint8_t mark_count; // the num of mark
	uint32_t mark[EMCOM_MPROUTER_MAX_MARK]; // all mark value
} mproute_policy;

struct reset_mproute_policy {
	uint8_t act;
	uint32_t const_perid;
	uint32_t rst_to_mark;
};

struct reset_mproute_policy_info {
	uint32_t uid;
	struct reset_mproute_policy policy;
	struct flow_info flow;
};

struct transfer_mproute_flow_info {
	uint32_t uid;
	uint32_t rst_from_mark;
	struct reset_mproute_policy policy;
};

uint32_t emcom_mproute_get_best_mark(void);
void emcom_mproute_evt_proc(int32_t event, const uint8_t *data, uint16_t len);
int emcom_mproute_getmark(int8_t index, uid_t uid, struct sockaddr *uaddr, uint8_t proto);
int emcom_mproute_bind_burst(int8_t index, uint16_t dport, uint8_t proto,
	struct emcom_xengine_mpflow_ai_burst_port *burst_info);
void emcom_mproute_set_default_mark(uint32_t mark);
uint32_t emcom_mproute_get_default_mark(void);
void emcom_mproute_init(void);
void emcom_mproute_close_tcp_flow(const uint32_t uid, const uint32_t rst_to_mark,
	const uint32_t rst_from_mark);
void emcom_mproute_close_udp_flow(const uint32_t uid, const uint32_t rst_to_mark,
	const reset_act act, const uint32_t rst_from_mark);

#endif
