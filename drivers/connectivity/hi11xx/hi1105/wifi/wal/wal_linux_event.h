/*
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 * 功能说明 : wal_linux_event.c 的头文件
 * 作    者 : wifi
 * 创建日期 : 2013年8月26日
 */

#ifndef WAL_LINUX_EVENT_H
#define WAL_LINUX_EVENT_H

#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_EVENT_H

uint32_t wal_cfg80211_start_scan(oal_net_device_stru *pst_net_dev, mac_cfg80211_scan_param_stru *pst_scan_param);
uint32_t wal_cfg80211_start_sched_scan(oal_net_device_stru *pst_net_dev,
    oal_cfg80211_sched_scan_request_stru *pst_request);
int32_t wal_cfg80211_start_connect(oal_net_device_stru *pst_net_dev, hmac_conn_param_stru *pst_connect_param);
int32_t wal_cfg80211_start_disconnect(oal_net_device_stru *pst_net_dev,
    mac_cfg_kick_user_param_stru *pst_disconnect_param);

#ifdef _PRE_WLAN_FEATURE_SAE
uint32_t wal_cfg80211_do_external_auth(oal_net_device_stru *pst_netdev, hmac_external_auth_req_stru *pst_ext_auth);
#endif /* _PRE_WLAN_FEATURE_SAE */
int32_t wal_cfg80211_start_req(oal_net_device_stru *pst_net_dev, const void *ps_param, uint16_t us_len,
    wlan_cfgid_enum_uint16 en_wid, oal_bool_enum_uint8 en_need_rsp);

#endif /* end of wal_linux_event.h */
