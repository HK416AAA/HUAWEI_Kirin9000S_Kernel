
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_regdomain.h"
#include "hmac_main.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_mgmt_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_p2p.h"
#include "hmac_sae.h"
#include "hmac_ftm.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#include "oal_main.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SME_STA_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
    g_handle_rsp_func_sta: 处理发给sme的消息
*****************************************************************************/
OAL_STATIC hmac_sme_handle_rsp_func g_handle_rsp_func_sta[HMAC_SME_RSP_BUTT] = {
    hmac_handle_scan_rsp_sta,
    hmac_handle_join_rsp_sta,
    hmac_handle_auth_rsp_sta,
    hmac_handle_asoc_rsp_sta,
};


/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_void  hmac_send_rsp_to_sme_sta(hmac_vap_stru *pst_hmac_vap, hmac_sme_rsp_enum_uint8 en_type,
                                   oal_uint8 *puc_msg);


OAL_STATIC oal_void  hmac_prepare_scan_req(mac_scan_req_stru *pst_scan_params, oal_int8 *pc_desired_ssid)
{
#ifndef  _PRE_WLAN_FPGA_DIGITAL
    oal_uint8       uc_chan_idx;
    oal_uint32      ul_ret;
    oal_uint8       uc_2g_chan_num = 0;
    oal_uint8       uc_5g_chan_num = 0;
    oal_uint8       uc_chan_number;
#endif

    memset_s(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru), 0, OAL_SIZEOF(mac_scan_req_stru));

    /* 设置初始扫描请求的参数 */
    pst_scan_params->en_bss_type         = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type        = WLAN_SCAN_TYPE_ACTIVE;
    pst_scan_params->us_scan_time        = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    pst_scan_params->uc_probe_delay      = 0;
    pst_scan_params->uc_scan_func        = MAC_SCAN_FUNC_BSS;               /* 默认扫描bss */
    pst_scan_params->p_fn_cb             = hmac_cfg80211_scan_comp_cb;      /* 使用cfg80211接口回调，因为ut/dmt都跟这个接口耦合 */
    pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;

    /* 设置初始扫描请求的ssid */
    /* 初始扫描请求只指定1个ssid, 要么是通配, 要么是mib值 */
    if (pc_desired_ssid[0] == '\0') {
        pst_scan_params->ast_mac_ssid_set[0].auc_ssid[0] = '\0';   /* 通配ssid */
    } else {
        if (memcpy_s(pst_scan_params->ast_mac_ssid_set[0].auc_ssid, WLAN_SSID_MAX_LEN,
                     pc_desired_ssid, OAL_STRLEN(pc_desired_ssid) + 1) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_prepare_scan_req::memcpy fail!");
            return;
        }
    }

    pst_scan_params->uc_ssid_num = 1;

    /* 设置初始扫描请求的bssid */
    /* 初始扫描请求只指定1个bssid，为广播地址 */
    oal_set_mac_addr(pst_scan_params->auc_bssid[0], BROADCAST_MACADDR);
    pst_scan_params->uc_bssid_num = 1;

#ifndef  _PRE_WLAN_FPGA_DIGITAL

    /* 2G初始扫描信道, 全信道扫描 */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++) {
            /* 判断信道是不是在管制域内 */
            ul_ret = mac_is_channel_idx_valid(WLAN_BAND_2G, uc_chan_idx);
            if (ul_ret == OAL_SUCC) {
                mac_get_channel_num_from_idx(WLAN_BAND_2G, uc_chan_idx, &uc_chan_number);

                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_chan_number = uc_chan_number;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].en_band        = WLAN_BAND_2G;
                pst_scan_params->ast_channel_list[uc_2g_chan_num].uc_idx         = uc_chan_idx;
                pst_scan_params->uc_channel_nums++;
                uc_2g_chan_num++;
            }
        }
    oam_info_log1(0, OAM_SF_SCAN, "{hmac_prepare_scan_req::after regdomain filter, the 2g total channel num is %d",
                  uc_2g_chan_num);

    /* 5G初始扫描信道, 全信道扫描 */
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++) {
            /* 判断信道是不是在管制域内 */
            ul_ret = mac_is_channel_idx_valid(WLAN_BAND_5G, uc_chan_idx);
            if (ul_ret == OAL_SUCC) {
                mac_get_channel_num_from_idx(WLAN_BAND_5G, uc_chan_idx, &uc_chan_number);

                pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_chan_number = uc_chan_number;
                pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].en_band        = WLAN_BAND_5G;
                pst_scan_params->ast_channel_list[uc_2g_chan_num + uc_5g_chan_num].uc_idx         = uc_chan_idx;
                pst_scan_params->uc_channel_nums++;
                uc_5g_chan_num++;
            }
        }
    oam_info_log1(0, OAM_SF_SCAN, "{hmac_prepare_scan_req::after regdomain filter, the 5g total channel num is %d",
                  uc_5g_chan_num);
#endif
}
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))

oal_int32   hmac_cfg80211_dump_survey(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_netdev,
                                      oal_int32 l_idx, oal_survey_info_stru *pst_info)
{
    mac_vap_stru             *pst_mac_vap;
    hmac_device_stru         *pst_hmac_dev;
    mac_scan_chan_stats_stru *pst_rcd;
    oal_int32 l_freq;

    if ((pst_wiphy == OAL_PTR_NULL) || (pst_netdev == OAL_PTR_NULL) || (pst_info == OAL_PTR_NULL) || (l_idx < 0) ||
        (l_idx >= WLAN_MAX_CHANNEL_NUM)) {
        oam_error_log4(0, OAM_SF_ANY,
                       "{hmac_cfg80211_dump_survey::pst_wiphy = %x, pst_netdev = %x, pst_info = %x l_idx=%d!}",
                       (uintptr_t)pst_wiphy, (uintptr_t)pst_netdev, (uintptr_t)pst_info, l_idx);
        return -OAL_EINVAL;
    }

    memset_s(pst_info, OAL_SIZEOF(oal_survey_info_stru), 0, OAL_SIZEOF(oal_survey_info_stru));

    pst_mac_vap = oal_net_dev_priv(pst_netdev);
    if (!pst_mac_vap) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_cfg80211_dump_survey::pst_mac_vap = NULL}");
        return -OAL_EINVAL;
    }

    pst_hmac_dev = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (!pst_hmac_dev) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_cfg80211_dump_survey::pst_hmac_dev = NULL}");
        return -OAL_EINVAL;
    }

    if (pst_hmac_dev->st_scan_mgmt.en_is_scanning) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_cfg80211_dump_survey::drop request while scan running}");
        return -OAL_EINVAL;
    }

    pst_rcd = pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.ast_chan_results;

    if (l_idx >= pst_hmac_dev->st_scan_mgmt.st_scan_record_mgmt.uc_chan_numbers) {
        return -ENOENT;
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
    pst_info->time          = pst_rcd[l_idx].ul_total_stats_time_us / 1000; /* 1ms等于1000us */
    pst_info->time_busy     = (pst_rcd[l_idx].ul_total_stats_time_us -
                               pst_rcd[l_idx].ul_total_free_time_20M_us) / 1000; /* 1ms等于1000us */
    pst_info->time_ext_busy = 0;
    pst_info->time_rx = pst_rcd[l_idx].ul_total_recv_time_us / 1000; /* 1ms等于1000us */
    pst_info->time_tx = pst_rcd[l_idx].ul_total_send_time_us / 1000; /* 1ms等于1000us */

    pst_info->filled = SURVEY_INFO_TIME | SURVEY_INFO_TIME_BUSY | SURVEY_INFO_TIME_RX | SURVEY_INFO_TIME_TX;
#else
    pst_info->channel_time          = pst_rcd[l_idx].ul_total_stats_time_us / 1000; /* 1ms等于1000us */
    pst_info->channel_time_busy     = (pst_rcd[l_idx].ul_total_stats_time_us -
                                       pst_rcd[l_idx].ul_total_free_time_20M_us) / 1000; /* 1ms等于1000us */
    pst_info->channel_time_ext_busy = 0;
    pst_info->channel_time_rx = pst_rcd[l_idx].ul_total_recv_time_us / 1000; /* 1ms等于1000us */
    pst_info->channel_time_tx = pst_rcd[l_idx].ul_total_send_time_us / 1000; /* 1ms等于1000us */

    pst_info->filled = SURVEY_INFO_CHANNEL_TIME
                       | SURVEY_INFO_CHANNEL_TIME_BUSY
                       | SURVEY_INFO_CHANNEL_TIME_RX
                       | SURVEY_INFO_CHANNEL_TIME_TX;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)) */

    if (pst_rcd[l_idx].uc_free_power_cnt && (pst_rcd[l_idx].s_free_power_stats_20M < 0)) {
        pst_info->noise = pst_rcd[l_idx].s_free_power_stats_20M / ((oal_int16)pst_rcd[l_idx].uc_free_power_cnt);
        pst_info->filled |= SURVEY_INFO_NOISE_DBM;
    }
    l_freq = mac_get_band_by_channel_num(pst_rcd[l_idx].uc_channel_number);
    pst_info->channel = oal_ieee80211_get_channel(pst_wiphy,
                                                  oal_ieee80211_channel_to_frequency(pst_rcd[l_idx].uc_channel_number,
                                                                                     l_freq));
    return 0;
}
#endif


oal_void  hmac_cfg80211_scan_comp_cb(void  *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record = (hmac_scan_record_stru *)p_scan_record;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    hmac_scan_rsp_stru               st_scan_rsp;

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_scan_comp_cb::pst_hmac_vap is null.");
        return;
    }

    /* 上报扫描结果结构体初始化 */
    memset_s(&st_scan_rsp, OAL_SIZEOF(st_scan_rsp), 0, OAL_SIZEOF(st_scan_rsp));

    st_scan_rsp.uc_result_code = pst_scan_record->en_scan_rsp_status;

    /* 扫描结果发给sme */
    hmac_send_rsp_to_sme_sta(pst_hmac_vap, HMAC_SME_SCAN_RSP, (oal_uint8 *)&st_scan_rsp);

    return;
}

oal_void hmac_cfg80211_prepare_scan_para(mac_scan_req_stru *pst_scan_params,
                                         mac_cfg80211_scan_param_stru *pst_cfg80211_scan_param)
{
    pst_scan_params->en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_params->en_scan_type = pst_cfg80211_scan_param->en_scan_type;

    /* 设置扫描时间 */
    if (pst_scan_params->en_scan_type == WLAN_SCAN_TYPE_ACTIVE) {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;
    } else {
        pst_scan_params->us_scan_time = WLAN_DEFAULT_PASSIVE_SCAN_TIME;
    }

    pst_scan_params->uc_probe_delay = 0;
    pst_scan_params->uc_scan_func = MAC_SCAN_FUNC_BSS; /* 默认扫描bss */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)
    pst_scan_params->uc_scan_func |= MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS;
#endif
    pst_scan_params->p_fn_cb = hmac_cfg80211_scan_comp_cb; /* 扫描完成回调函数 */
    pst_scan_params->uc_max_scan_count_per_channel = 2;  /* 每个信道的最大扫描次数为2 */
}
OAL_STATIC oal_void hmac_2g_scan_channel_init(mac_cfg80211_scan_param_stru *pst_cfg80211_scan_param,
    mac_vap_stru *pst_mac_vap, mac_scan_req_stru *pst_scan_params, oal_uint8 *puc_chan_num_2g)
{
    oal_uint8 uc_loop; /* for循环数组下标 */
    oal_uint8 uc_channel_number; /* 信道号 */
    oal_uint8 uc_channel_idx = 0; /* 信道号索引 */
    oal_uint32 ul_ret;

    for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->uc_num_channels_2G; uc_loop++) {
        uc_channel_number = (oal_uint8)pst_cfg80211_scan_param->pul_channels_2G[uc_loop];

        /* 判断信道是不是在管制域内 */
        ul_ret = mac_is_channel_num_valid(WLAN_BAND_2G, uc_channel_number);
        if (ul_ret == OAL_SUCC) {
            ul_ret = mac_get_channel_idx_from_num(WLAN_BAND_2G, uc_channel_number, &uc_channel_idx);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(0, OAM_SF_SCAN, "mac_get_channel_idx_from_num fail.chn=%d", uc_channel_number);
            }
#ifdef _PRE_WLAN_FEATURE_FTM
            if (!hmac_ftm_is_in_scan_list(pst_mac_vap, WLAN_BAND_2G, uc_channel_number)) {
                continue;
            }
#endif
            pst_scan_params->ast_channel_list[*puc_chan_num_2g].en_band        = WLAN_BAND_2G;
            pst_scan_params->ast_channel_list[*puc_chan_num_2g].uc_chan_number = uc_channel_number;
            pst_scan_params->ast_channel_list[*puc_chan_num_2g].uc_idx         = uc_channel_idx;

            pst_scan_params->uc_channel_nums++;
            (*puc_chan_num_2g)++;
        }
    }
}

OAL_STATIC oal_void hmac_5g_scan_channel_init(mac_cfg80211_scan_param_stru *pst_cfg80211_scan_param,
    mac_vap_stru *pst_mac_vap, mac_scan_req_stru *pst_scan_params, oal_uint8 uc_chan_num_2g)
{
    oal_uint8 uc_loop; /* for循环数组下标 */
    oal_uint8 uc_channel_number; /* 信道号 */
    oal_uint8 uc_channel_idx = 0; /* 信道号索引 */
    oal_uint32 ul_ret;
    oal_uint8 uc_chan_num_5g = 0;

    for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->uc_num_channels_5G; uc_loop++) {
        uc_channel_number = (oal_uint8)pst_cfg80211_scan_param->pul_channels_5G[uc_loop];

        /* 判断信道是不是在管制域内 */
        ul_ret = mac_is_channel_num_valid(WLAN_BAND_5G, uc_channel_number);
        if (ul_ret == OAL_SUCC) {
            ul_ret = mac_get_channel_idx_from_num(WLAN_BAND_5G, uc_channel_number, &uc_channel_idx);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{WLAN_BAND_5G:hmac_cfg80211_prepare_scan_req_sta\
                                 ::mac_get_channel_idx_from_num fail. channel_number: %u}", uc_channel_number);
            }
#ifdef _PRE_WLAN_FEATURE_FTM
            if (!hmac_ftm_is_in_scan_list(pst_mac_vap, WLAN_BAND_5G, uc_channel_number)) {
                continue;
            }
#endif

            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].en_band        = WLAN_BAND_5G;
            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].uc_chan_number = uc_channel_number;
            pst_scan_params->ast_channel_list[uc_chan_num_2g + uc_chan_num_5g].uc_idx         = uc_channel_idx;

            pst_scan_params->uc_channel_nums++;
            uc_chan_num_5g++;
        }
    }
}

OAL_STATIC oal_void  hmac_cfg80211_prepare_scan_req_sta(mac_vap_stru *pst_mac_vap,
                                                        mac_scan_req_stru *pst_scan_params, oal_int8 *puc_param)
{
    oal_uint8                       uc_loop;            /* for循环数组下标 */
    mac_cfg80211_scan_param_stru   *pst_cfg80211_scan_param;
    mac_device_stru                *pst_mac_device = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_vap_stru                   *pst_mac_vap_temp;
    oal_uint32                      ul_ret;
#endif
    oal_bool_enum_uint8             en_scan_when_go_up = OAL_FALSE;
    oal_uint8                       uc_chan_num_2g = 0;

    pst_cfg80211_scan_param = (mac_cfg80211_scan_param_stru *)puc_param;

    /* 外部已经zero, 配置扫描参数 */
    hmac_cfg80211_prepare_scan_para(pst_scan_params, pst_cfg80211_scan_param);

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        return;
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    /* p2p Go发起扫描时，会使用p2p device设备进行 */
    if (pst_mac_vap->en_p2p_mode == WLAN_P2P_DEV_MODE) {
        ul_ret = mac_device_find_up_p2p_go(pst_mac_device, &pst_mac_vap_temp);
        if (ul_ret == OAL_SUCC) {
            en_scan_when_go_up = OAL_TRUE;
        }
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */

    if ((pst_cfg80211_scan_param->l_ssid_num <= 1) && ((en_scan_when_go_up == OAL_TRUE) ||
        (pst_mac_vap->en_vap_state == MAC_VAP_STATE_UP) || (pst_mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE) ||
         ((pst_mac_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN) && (pst_mac_vap->us_user_nums > 0)))) {
        pst_scan_params->uc_max_scan_count_per_channel = 1;
    }

    for (uc_loop = 0; uc_loop < pst_cfg80211_scan_param->l_ssid_num; uc_loop++) {
        if (memcpy_s(pst_scan_params->ast_mac_ssid_set[uc_loop].auc_ssid, WLAN_SSID_MAX_LEN,
            pst_cfg80211_scan_param->st_ssids[uc_loop].auc_ssid,
            pst_cfg80211_scan_param->st_ssids[uc_loop].uc_ssid_len) != EOK) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cfg80211_prepare_scan_req_sta::memcpy fail.}");
            return;
        }
        pst_scan_params->ast_mac_ssid_set[uc_loop].auc_ssid[pst_cfg80211_scan_param->st_ssids[uc_loop].uc_ssid_len] =
            '\0';   /* ssid末尾置'\0' */
    }

    /* 如果上层下发了指定ssid，则每次扫描发送的probe req帧的个数为下发的ssid个数 */
    pst_scan_params->uc_max_send_probe_req_count_per_channel = (oal_uint8)pst_cfg80211_scan_param->l_ssid_num;

    /* 如果上层未下发指定ssid，则采用默认值 */
    if (pst_scan_params->uc_max_send_probe_req_count_per_channel == 0) {
        pst_scan_params->ast_mac_ssid_set[0].auc_ssid[0] = '\0';   /* 通配ssid */
        pst_scan_params->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    }
    /* 如果指定SSID个数大于3个,则调整发送超时时间为40ms,默认发送等待超时20ms */
    if ((pst_scan_params->uc_max_send_probe_req_count_per_channel > 3) &&
        (pst_scan_params->en_scan_type == WLAN_SCAN_TYPE_ACTIVE)) {
        pst_scan_params->us_scan_time = WLAN_LONG_ACTIVE_SCAN_TIME;
    }

    pst_scan_params->uc_ssid_num = (oal_uint8)pst_cfg80211_scan_param->l_ssid_num;

    oal_set_mac_addr(pst_scan_params->auc_bssid[0], BROADCAST_MACADDR);
    pst_scan_params->uc_bssid_num = 1;  /* 初始扫描请求指定1个bssid，为广播地址 */

    /* 2G初始扫描信道 */
    hmac_2g_scan_channel_init(pst_cfg80211_scan_param, pst_mac_vap, pst_scan_params, &uc_chan_num_2g);

    /* 5G初始扫描信道 */
    hmac_5g_scan_channel_init(pst_cfg80211_scan_param, pst_mac_vap, pst_scan_params, uc_chan_num_2g);
#ifdef _PRE_WLAN_FEATURE_P2P
    /* WLAN/P2P 共存时，判断是否p2p0 发起扫描 */
    pst_scan_params->bit_is_p2p0_scan = pst_cfg80211_scan_param->bit_is_p2p0_scan;
    if (pst_cfg80211_scan_param->bit_is_p2p0_scan) {
        pst_scan_params->en_bss_type = 0;
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */
}


oal_void  hmac_prepare_join_req(hmac_join_req_stru *pst_join_req, mac_bss_dscr_stru *pst_bss_dscr)
{
    memset_s(pst_join_req, OAL_SIZEOF(hmac_join_req_stru), 0, OAL_SIZEOF(hmac_join_req_stru));

    if (memcpy_s(&(pst_join_req->st_bss_dscr), OAL_SIZEOF(mac_bss_dscr_stru),
                 pst_bss_dscr, OAL_SIZEOF(mac_bss_dscr_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_prepare_join_req::memcpy fail!");
        return;
    }

    pst_join_req->us_join_timeout = WLAN_JOIN_START_TIMEOUT;
    pst_join_req->us_probe_delay = WLAN_PROBE_DELAY_TIME;
}


oal_void hmac_prepare_auth_req(hmac_vap_stru *pst_hmac_vap, hmac_auth_req_stru *pst_auth_req)
{
    memset_s(pst_auth_req, OAL_SIZEOF(*pst_auth_req), 0, OAL_SIZEOF(*pst_auth_req));
#ifdef _PRE_WLAN_FEATURE_SAE
    if ((pst_hmac_vap->en_auth_mode == WLAN_WITP_AUTH_SAE) &&
        (pst_hmac_vap->bit_sae_connect_with_pmkid == OAL_FALSE)) {
        /*
         * SAE auth timeout time increase from 150ms to 300ms/600ms for compatibility, such as 1151 AP
         * The processing capability of some IoT devices is poor. Therefore, the auth timeout period needs to be
         * changed to 1.2s.
         */
#ifdef _PRE_WLAN_CHBA_MGMT
        /* CHBA不存在兼容性问题，auth超时时间改为150ms。 */
        if (mac_is_chba_mode(&(pst_hmac_vap->st_vap_base_info)) == OAL_TRUE) {
            pst_auth_req->us_timeout = WLAN_AUTH_TIMEOUT >> 1;
            return;
        }
#endif
        pst_auth_req->us_timeout = WLAN_AUTH_TIMEOUT * 4; /* 4*300ms等1.2s */
    } else
#endif /* _PRE_WLAN_FEATURE_SAE */
    {
        /* 增加AUTH报文次数后，将超时时间减半 */
        pst_auth_req->us_timeout = WLAN_AUTH_TIMEOUT >> 1;
    }
}


OAL_STATIC oal_void  hmac_prepare_asoc_req(mac_vap_stru *pst_mac_vap, hmac_asoc_req_stru *pst_asoc_req)
{
    memset_s(pst_asoc_req, OAL_SIZEOF(hmac_asoc_req_stru), 0, OAL_SIZEOF(hmac_asoc_req_stru));

    /* 增加ASOC报文次数后，将超时时间减半 */
    pst_asoc_req->us_assoc_timeout = (oal_uint16)
        pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11AssociationResponseTimeOut >> 1;
}


oal_uint32  hmac_sta_initiate_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32              ul_ret;
    mac_scan_req_stru       st_scan_params;
    hmac_vap_stru          *pst_hmac_vap = OAL_PTR_NULL;
    oal_int8               *pc_desired_ssid = OAL_PTR_NULL;

#ifdef  _PRE_WLAN_FEATURE_P2P
    mac_device_stru        *pst_mac_device;
#endif

    if (oal_unlikely(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initial_scan: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 针对配置vap做保护 */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_CONFIG) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_initiate_scan::config vap! can't get info.}");
        return OAL_FAIL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_initiate_scan::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_desired_ssid = (oal_int8 *)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;
    hmac_prepare_scan_req(&st_scan_params, pc_desired_ssid);
#ifdef _PRE_WLAN_FEATURE_P2P
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (oal_unlikely(pst_mac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_sta_initial_scan: pst_mac_device is null device id[%d].}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    st_scan_params.bit_is_p2p0_scan = puc_param[0];
    /* 记录扫描参数信息到device结构体 */
    if (memcpy_s(&(pst_mac_device->st_scan_params), OAL_SIZEOF(mac_scan_req_stru),
                 &st_scan_params, OAL_SIZEOF(mac_scan_req_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "hmac_sta_initiate_scan::memcpy fail!");
        return OAL_FAIL;
    }
#endif  /* _PRE_WLAN_FEATURE_P2P */

    /* 状态机调用: hmac_scan_proc_scan_req_event */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&st_scan_params));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_initiate_scan::fsm_call_func_sta fail[%d].}",
                         ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_cfg80211_check_can_start_sched_scan(mac_vap_stru *pst_mac_vap)
{
    hmac_device_stru      *pst_hmac_device;
    mac_vap_stru          *pst_mac_vap_tmp = OAL_PTR_NULL;
    oal_uint32             ul_ret;

    /* 获取mac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (oal_unlikely(pst_hmac_device == OAL_PTR_NULL)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::pst_hmac_device[%d] null.}",
                         pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果vap的模式不是STA，则返回，不支持其它模式的vap的调度扫描 */
    if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::vap mode[%d] don't support sched scan.}",
                         pst_mac_vap->en_vap_mode);

        pst_hmac_device->st_scan_mgmt.pst_sched_scan_req = OAL_PTR_NULL;
        pst_hmac_device->st_scan_mgmt.en_sched_scan_complete = OAL_TRUE;
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* 如果存在当前device存在up的vap，则不启动调度扫描 */
    ul_ret = mac_device_find_up_vap(pst_hmac_device->pst_device_base_info, &pst_mac_vap_tmp);
    if (ul_ret == OAL_SUCC) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_check_can_start_sched_scan::exist up vap, don't start sched scan.}");

        pst_hmac_device->st_scan_mgmt.pst_sched_scan_req = OAL_PTR_NULL;
        pst_hmac_device->st_scan_mgmt.en_sched_scan_complete = OAL_TRUE;
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    return OAL_SUCC;
}



oal_uint32  hmac_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                       *pst_hmac_vap = OAL_PTR_NULL;
    mac_pno_scan_stru                    st_pno_scan_params;
    oal_uint32                           ul_ret;
    oal_bool_enum_uint8                  en_is_random_mac_addr_scan;
    hmac_device_stru                     *pst_hmac_device = OAL_PTR_NULL;
    mac_pno_scan_stru    *pst_cfg80211_pno_scan_params = NULL;

    /* 参数合法性检查 */
    if (oal_unlikely(puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_start_sched_scan::puc_param is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_cfg80211_pno_scan_params = (mac_pno_scan_stru *)(uintptr_t)(*(oal_uint *)puc_param);
    if (pst_mac_vap == OAL_PTR_NULL) {
        oal_mem_free_m(pst_cfg80211_pno_scan_params, OAL_TRUE);
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_cfg80211_start_sched_scan:pst_mac_vap is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取hmac vap */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (oal_unlikely(pst_hmac_vap == OAL_PTR_NULL)) {
        oal_mem_free_m(pst_cfg80211_pno_scan_params, OAL_TRUE);
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cfg80211_start_sched_scan::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        oal_mem_free_m(pst_cfg80211_pno_scan_params, OAL_TRUE);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_start_sched_scan::hmac_device[%d] null.}",
                         pst_hmac_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 拷贝上层下发的pno扫描参数 */
    if (memcpy_s(&st_pno_scan_params, OAL_SIZEOF(mac_pno_scan_stru),
                 pst_cfg80211_pno_scan_params, OAL_SIZEOF(mac_pno_scan_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "hmac_cfg80211_start_sched_scan::memcpy fail!");
        oal_mem_free_m(pst_cfg80211_pno_scan_params, OAL_TRUE);
        return OAL_FAIL;
    }

    oal_mem_free_m(pst_cfg80211_pno_scan_params, OAL_TRUE);

    /* 检测当前device是否可以启动调度扫描 */
    ul_ret = hmac_cfg80211_check_can_start_sched_scan(pst_mac_vap);
    if (ul_ret != OAL_SUCC) {
        return ul_ret;
    }

    /* 设置PNO调度扫描结束时，如果有结果上报，则上报扫描结果的回调函数 */
    st_pno_scan_params.p_fn_cb = hmac_cfg80211_scan_comp_cb;

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    en_is_random_mac_addr_scan = g_wlan_customize.uc_random_mac_addr_scan;
#else
    en_is_random_mac_addr_scan = pst_hmac_device->st_scan_mgmt.en_is_random_mac_addr_scan;
#endif

    /* 设置发送的probe req帧中源mac addr */
    st_pno_scan_params.en_is_random_mac_addr_scan = en_is_random_mac_addr_scan;
    hmac_scan_set_sour_mac_addr_in_probe_req(pst_hmac_vap, st_pno_scan_params.auc_sour_mac_addr,
                                             en_is_random_mac_addr_scan, OAL_FALSE);

    /* 状态机调用: hmac_scan_proc_sched_scan_req_event */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCHED_SCAN_REQ, (oal_void *)(&st_pno_scan_params));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_start_sched_scan::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        pst_hmac_device->st_scan_mgmt.pst_sched_scan_req     = OAL_PTR_NULL;
        pst_hmac_device->st_scan_mgmt.en_sched_scan_complete = OAL_TRUE;
    }

    return ul_ret;
}


oal_uint32  hmac_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
                         抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap,
                                    WLAN_CFGID_STOP_SCHED_SCAN,
                                    us_len,
                                    puc_param);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_cfg80211_stop_sched_scan::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_free_scan_param_resource(mac_cfg80211_scan_param_stru *pst_scan_param)
{
    if (pst_scan_param->pul_channels_2G != OAL_PTR_NULL) {
        oal_free(pst_scan_param->pul_channels_2G);
        pst_scan_param->pul_channels_2G = OAL_PTR_NULL;
    }
    if (pst_scan_param->pul_channels_5G != OAL_PTR_NULL) {
        oal_free(pst_scan_param->pul_channels_5G);
        pst_scan_param->pul_channels_5G = OAL_PTR_NULL;
    }
    if (pst_scan_param->puc_ie != OAL_PTR_NULL) {
        oal_free(pst_scan_param->puc_ie);
        pst_scan_param->puc_ie = OAL_PTR_NULL;
    }
}


oal_uint32  hmac_cfg80211_start_scan_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_scan_req_stru                    st_scan_params;
    hmac_vap_stru                       *pst_hmac_vap = OAL_PTR_NULL;
    mac_device_stru                     *pst_mac_device = OAL_PTR_NULL;
    mac_cfg80211_scan_param_stru        *pst_mac_cfg80211_scan_param = OAL_PTR_NULL;
    oal_app_ie_stru                     *pst_app_ie = OAL_PTR_NULL;
    oal_uint32                           ul_ret = OAL_FAIL;
    if (oal_unlikely(puc_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::null ptr, %p.}", (uintptr_t)puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(&st_scan_params, OAL_SIZEOF(mac_scan_req_stru), 0, OAL_SIZEOF(mac_scan_req_stru));

    pst_mac_cfg80211_scan_param = (mac_cfg80211_scan_param_stru *)puc_param;
    if (oal_unlikely(pst_mac_cfg80211_scan_param == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN,
                       "{hmac_cfg80211_start_scan_sta::scan failed,pst_mac_cfg80211_scan_param is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_unlikely(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::scan failed, pst_mac_vap is NULL.}");
        /*lint -e801*/
        goto ERROR_STEP;
        /*lint +e801*/
    }

    /* 初始扫描列表清0 */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_start_scan_sta::scan failed, null ptr, pst_mac_device null.}");
        /*lint -e801*/
        goto ERROR_STEP;
        /*lint +e801*/
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_start_scan_sta::scan failed, null ptr, pst_hmac_vap null.}");
        /*lint -e801*/
        goto ERROR_STEP;
        /*lint +e801*/
    }

    /* 将内核下发的扫描参数更新到驱动扫描请求结构体中 */
    hmac_cfg80211_prepare_scan_req_sta(pst_mac_vap, &st_scan_params, (oal_int8 *)pst_mac_cfg80211_scan_param);

    /* 设置P2P/WPS IE 信息到 vap 结构体中 */
    /* BEGIN: WLAN发送的 probe request 不携带P2P IE */
    if (IS_LEGACY_VAP(pst_mac_vap)) {
        hmac_config_del_p2p_ie((oal_uint8 *)(pst_mac_cfg80211_scan_param->puc_ie),
                               &(pst_mac_cfg80211_scan_param->ul_ie_len));
    }
    /* END: WLAN发送的 probe request 不携带P2P IE */
#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P0 扫描时记录P2P listen channel */
    if (pst_mac_cfg80211_scan_param->bit_is_p2p0_scan == OAL_TRUE) {
        hmac_find_p2p_listen_channel(pst_mac_vap, (oal_uint16)(pst_mac_cfg80211_scan_param->ul_ie_len),
                                     (oal_uint8 *)(pst_mac_cfg80211_scan_param->puc_ie));
        st_scan_params.uc_p2p0_listen_channel = pst_mac_vap->uc_p2p_listen_channel;
    }
#endif
    pst_app_ie = (oal_app_ie_stru *)oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(oal_app_ie_stru), OAL_TRUE);
    if (oal_unlikely(pst_app_ie == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::scan failed, oal_memalloc failed.}");
        /*lint -e801*/
        goto ERROR_STEP;
        /*lint +e801*/
    }

    pst_app_ie->ul_ie_len = pst_mac_cfg80211_scan_param->ul_ie_len;
    if ((pst_app_ie->ul_ie_len > 0) &&
        memcpy_s(pst_app_ie->auc_ie, OAL_SIZEOF(pst_app_ie->auc_ie),
                 pst_mac_cfg80211_scan_param->puc_ie, pst_app_ie->ul_ie_len) != EOK) {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "hmac_cfg80211_start_scan_sta::memcpy fail! src_len[%d]", pst_app_ie->ul_ie_len);
    }
    pst_app_ie->en_app_ie_type = OAL_APP_PROBE_REQ_IE;
    ul_ret = hmac_config_set_app_ie_to_vap(pst_mac_vap, pst_app_ie, pst_app_ie->en_app_ie_type);
    if (ul_ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_cfg80211_start_scan_sta::set_app_ie_to_vap fail err_code=%d.}", ul_ret);
        oal_mem_free_m(pst_app_ie, OAL_TRUE);
        /*lint -e801*/
        goto ERROR_STEP;
        /*lint +e801*/
    }
    oal_mem_free_m(pst_app_ie, OAL_TRUE);

#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    /* ut do noting */
#else
    /* 因为扫描命令下发修改为非阻塞方式，因此，信道申请的内存需要再此处释放 */
    hmac_free_scan_param_resource(pst_mac_cfg80211_scan_param);
#endif

    /* 状态机调用: hmac_scan_proc_scan_req_event, hmac_scan_proc_scan_req_event_exception */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&st_scan_params));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_cfg80211_start_scan_sta::hmac_fsm_call_func_sta fail[%d].}", ul_ret);

        /* BEGIN: :如果hmac 扫描执行失败，则上报wal 层扫描失败事件,不用等待wal 层扫描定时器超时 */
        hmac_scan_proc_scan_req_event_exception(pst_hmac_vap, &ul_ret);
        /* END: :如果hmac 扫描执行失败，则上报wal 层扫描失败事件,不用等待wal 层扫描定时器超时 */
        return ul_ret;
    }

    return OAL_SUCC;

ERROR_STEP:
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
    /* ut do noting */
#else
    /* 因为扫描命令下发修改为非阻塞方式，因此，信道申请的内存需要再此处释放 */
    hmac_free_scan_param_resource(pst_mac_cfg80211_scan_param);
#endif
    return OAL_ERR_CODE_PTR_NULL;
}


oal_uint32  hmac_sta_initiate_join(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    hmac_join_req_stru  st_join_req;
    hmac_vap_stru      *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32          ul_ret;
    oal_uint8           uc_rate_num;

    if (oal_unlikely((pst_mac_vap == OAL_PTR_NULL) || (pst_bss_dscr == OAL_PTR_NULL))) {
        oam_error_log2(0, OAM_SF_ASSOC, "{hmac_sta_initiate_join::input null %x %x.}", (uintptr_t)pst_mac_vap,
                       (uintptr_t)pst_bss_dscr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_initiate_join::mac_res_get_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* :将需要关联的速率放入hmac_vap中 */
    uc_rate_num = (pst_bss_dscr->uc_num_supp_rates < WLAN_MAX_SUPP_RATES) ? pst_bss_dscr->uc_num_supp_rates :
                  WLAN_MAX_SUPP_RATES;
    if (memcpy_s(pst_hmac_vap->auc_supp_rates, WLAN_MAX_SUPP_RATES,
        pst_bss_dscr->auc_supp_rates, uc_rate_num) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "hmac_sta_initiate_join::memcpy fail!");
        return OAL_FAIL;
    }

    pst_hmac_vap->uc_rs_nrates = pst_bss_dscr->uc_num_supp_rates;

    hmac_prepare_join_req(&st_join_req, pst_bss_dscr);

    /* 状态机调用 hmac_sta_wait_join */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_JOIN_REQ, (oal_void *)(&st_join_req));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_sta_initiate_join::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_initiate_auth(mac_vap_stru *pst_mac_vap)
{
    hmac_auth_req_stru  st_auth_req;
    hmac_vap_stru      *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32          ul_ret;

    if (oal_unlikely(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_auth: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_initiate_auth: pst_hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_prepare_auth_req(pst_hmac_vap, &st_auth_req);

    /* 状态机调用 hmac_sta_wait_auth */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_AUTH_REQ, (oal_void *)(&st_auth_req));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_sta_initiate_join::hmac_fsm_call_func_sta fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_initiate_asoc(mac_vap_stru *pst_mac_vap)
{
    hmac_asoc_req_stru  st_asoc_req;
    hmac_vap_stru      *pst_hmac_vap = OAL_PTR_NULL;
    oal_uint32          ul_ret;

    if (oal_unlikely(pst_mac_vap == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_sta_initiate_asoc::pst_mac_vap null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "hmac_sta_initiate_asoc: pst_hmac_vap null!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_prepare_asoc_req(pst_mac_vap, &st_asoc_req);

    /* 状态机调用 */
    ul_ret = hmac_fsm_call_func_sta(pst_hmac_vap, HMAC_FSM_INPUT_ASOC_REQ, (oal_void *)(&st_asoc_req));
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "hmac_sta_initiate_asoc::hmac_fsm_call_func_sta fail[%d]", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


#if  (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)

OAL_STATIC oal_void  hmac_join_bss_sta(hmac_vap_stru *pst_hmac_vap)
{
    mac_bss_dscr_stru   *pst_bss_dscr = OAL_PTR_NULL;
    oal_uint32           ul_bss_idx;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        oam_warning_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "hmac_join_bss_sta::pst_mac_device null");
        return;
    }

    /* 是否有期望的AP存在 */
    if (pst_hmac_device->uc_desired_bss_num > 0) {
        ul_bss_idx   = pst_hmac_device->auc_desired_bss_idx[pst_hmac_device->uc_desired_bss_num - 1];  /* 从后往前取期望的bss */
        pst_bss_dscr =
            hmac_scan_find_scanned_bss_dscr_by_index(pst_hmac_vap->st_vap_base_info.uc_device_id, ul_bss_idx);

        pst_hmac_device->uc_desired_bss_num--;      /* 期望加入的AP个数-1 */

        oam_info_log1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                      "{hmac_join_bss_sta::try to join bss[%d].}", ul_bss_idx);

        /* 加入请求 */
        hmac_sta_initiate_join(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr);
    }
}


oal_void  hmac_prepare_bss_list(hmac_vap_stru *pst_hmac_vap)
{
    oal_int8               *pc_mib_ssid = OAL_PTR_NULL;
    oal_uint32              ul_bss_idx = 0;
    hmac_device_stru       *pst_hmac_device;
    hmac_bss_mgmt_stru     *pst_bss_mgmt = OAL_PTR_NULL;
    hmac_scanned_bss_info  *pst_scanned_bss = OAL_PTR_NULL;
    mac_bss_dscr_stru      *pst_bss_dscr = OAL_PTR_NULL;
    oal_dlist_head_stru    *pst_entry = OAL_PTR_NULL;
    oal_uint8               uc_device_id;

    /* 获取device id */
    uc_device_id = pst_hmac_vap->st_vap_base_info.uc_device_id;

    /* 获取hmac device结构体 */
    pst_hmac_device = hmac_res_get_mac_dev(uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ASSOC, "hmac_prepare_bss_list::pst_hmac_device null");
        return;
    }

    pc_mib_ssid = (oal_int8 *)pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11DesiredSSID;

    /* 没有设置期望的ssid, 继续发起扫描 */
    if (pc_mib_ssid[0] == 0) {
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                      "{hmac_prepare_bss_list::desired ssid is not set.}");
        return;
    }

    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 更新期望的ap列表，遍历扫描到的bss, 看是否是期望的 */
    oal_dlist_search_for_each(pst_entry, &(pst_bss_mgmt->st_bss_list_head))
    {
        pst_scanned_bss = oal_dlist_get_entry(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        if (oal_strcmp(pc_mib_ssid, pst_bss_dscr->ac_ssid) == 0) {
            /* 检测期望join 的AP能力信息是否匹配 */
            if (hmac_check_capability_mac_phy_supplicant(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr) != OAL_SUCC) {
                continue;
            }

            pst_bss_dscr->en_desired = OAL_TRUE;

            pst_hmac_device->auc_desired_bss_idx[pst_hmac_device->uc_desired_bss_num] = (oal_uint8)ul_bss_idx;
            pst_hmac_device->uc_desired_bss_num++;
        } else {
            pst_bss_dscr->en_desired = OAL_FALSE;
        }

        ul_bss_idx++;
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    hmac_join_bss_sta(pst_hmac_vap);
}
#endif


oal_void  hmac_handle_scan_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event = OAL_PTR_NULL;
    hmac_scan_rsp_stru  *pst_scan_rsp = OAL_PTR_NULL;

    /* 抛扫描完成事件到WAL */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(hmac_scan_rsp_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_handle_scan_rsp_sta::frw_event_alloc fail.}");
        return;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA,
                       OAL_SIZEOF(hmac_scan_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_scan_rsp = (hmac_scan_rsp_stru *)pst_event->auc_event_data;

    if (memcpy_s(pst_scan_rsp, OAL_SIZEOF(hmac_scan_rsp_stru), puc_msg, OAL_SIZEOF(hmac_scan_rsp_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "hmac_handle_scan_rsp_sta::memcpy fail!");
        frw_event_free_m(pst_event_mem);
        return;
    }

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    frw_event_free_m(pst_event_mem);

#if  (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* 准备期望加入的bss列表 */
    hmac_prepare_bss_list(pst_hmac_vap);
#endif
}


oal_void  hmac_handle_join_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    hmac_join_rsp_stru  *pst_join_rsp = (hmac_join_rsp_stru *)puc_msg;

    if (pst_join_rsp->en_result_code == HMAC_MGMT_SUCCESS) {
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_join_rsp_sta::join succ.}");

        /* 初始化AUTH次数 */
        pst_hmac_vap->uc_auth_cnt = 1;
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));
    } else {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "hmac_handle_join_rsp_sta::join fail[%d]", pst_join_rsp->en_result_code);
#ifdef _PRE_WLAN_1102A_CHR
        hmac_chr_connect_fail_query_and_report(pst_hmac_vap, MAC_JOIN_RSP_TIMEOUT);
#endif
        hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);
    }
}


oal_void hmac_report_connect_failed_result(hmac_vap_stru *pst_hmac_vap, mac_status_code_enum_uint16 reason_code,
    uint8_t *peer_addr)
{
    frw_event_mem_stru  *pst_event_mem = OAL_PTR_NULL;
    frw_event_stru      *pst_event = OAL_PTR_NULL;
    hmac_asoc_rsp_stru   st_asoc_rsp;
    oal_uint8           *mgmt_data = NULL;
    oal_uint32           ret;

    if (pst_hmac_vap->ul_asoc_req_ie_len > 0 && pst_hmac_vap->puc_asoc_req_ie_buff != NULL) {
        mgmt_data = (oal_uint8 *)oal_memalloc(pst_hmac_vap->ul_asoc_req_ie_len);
        if (mgmt_data == NULL) {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                "{hmac_report_connect_failed_result::mgmt_data alloc null,size %d.}", pst_hmac_vap->ul_asoc_req_ie_len);
            return;
        }
        memcpy_s(mgmt_data, pst_hmac_vap->ul_asoc_req_ie_len,
            (oal_uint8 *)(pst_hmac_vap->puc_asoc_req_ie_buff), pst_hmac_vap->ul_asoc_req_ie_len);
    }

    memset_s(&st_asoc_rsp, sizeof(st_asoc_rsp), 0, sizeof(st_asoc_rsp));
    st_asoc_rsp.en_result_code       = HMAC_MGMT_TIMEOUT;
    st_asoc_rsp.en_status_code       = reason_code;
    /* 扫描超时需要释放对应HMAC VAP下的关联请求buff */
    st_asoc_rsp.puc_asoc_req_ie_buff = mgmt_data;
    memcpy_s(st_asoc_rsp.auc_addr_ap, WLAN_MAC_ADDR_LEN, peer_addr, WLAN_MAC_ADDR_LEN);
    /* 抛加入完成事件到WAL */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(hmac_asoc_rsp_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        if (mgmt_data != NULL) {
            oal_free(mgmt_data);
        }
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_report_connect_failed_result::frw_event_alloc fail!}");
        return;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA,
                       OAL_SIZEOF(hmac_asoc_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    if (memcpy_s(frw_get_event_payload(pst_event_mem), OAL_SIZEOF(hmac_asoc_rsp_stru),
        &st_asoc_rsp, OAL_SIZEOF(hmac_asoc_rsp_stru)) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_report_connect_failed_result::memcpy fail!}");
    }

    ret = frw_event_dispatch_event(pst_event_mem);
    if (ret != OAL_SUCC) {
        if (mgmt_data != NULL) {
            oal_free(mgmt_data);
        }
    }

    frw_event_free_m(pst_event_mem);
}
oal_void hmac_handle_free_buff(oal_uint8 *buf1, oal_uint8 *buf2)
{
    if (buf1 != NULL) {
        oal_free(buf1);
        buf1 = NULL;
    }
    if (buf2 != NULL) {
        oal_free(buf2);
        buf2 = NULL;
    }
}


oal_void hmac_handle_connect_failed_result(hmac_vap_stru *pst_hmac_vap, oal_uint16 us_status, uint8_t *user_addr)
{
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;

    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                     "{hmac_handle_connect_failed_result::connect fail[%d]}", us_status);

    /* 上报关联失败到wpa_supplicant */
    hmac_report_connect_failed_result(pst_hmac_vap, us_status, user_addr);

    /* 获取用户指针 */
    pst_hmac_user = mac_res_get_hmac_user(pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (pst_hmac_user != OAL_PTR_NULL) {
        /* 发送去认证帧到AP */
        hmac_mgmt_send_deauth_frame(&pst_hmac_vap->st_vap_base_info,
                                    pst_hmac_user->st_user_base_info.auc_user_mac_addr,
                                    WLAN_MAC_ADDR_LEN, us_status, OAL_FALSE);

        /* 删除对应用户 */
        hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
    } else {
        /* 设置状态为FAKE UP */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "asoc:pst_hmac_user[%d] NULL.",
                         pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    }

    /* 同步DMAC状态 */
    hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);
}


oal_void  hmac_handle_auth_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    hmac_auth_rsp_stru     *pst_auth_rsp = (hmac_auth_rsp_stru *)puc_msg;
    hmac_user_stru         *pst_hmac_user = OAL_PTR_NULL;
    oal_uint8               auc_param[] = "all";

    if (pst_auth_rsp->us_status_code == HMAC_MGMT_SUCCESS) {
        oam_info_log0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_handle_auth_rsp_sta::auth succ.}");

        /* 初始化AOSC次数 */
        pst_hmac_vap->uc_asoc_cnt = 1;
        hmac_sta_initiate_asoc(&pst_hmac_vap->st_vap_base_info);

        return;
    }

    oam_warning_log2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
        "hmac_handle_auth_rsp_sta::auth fail[%d],cnt[%d]", pst_auth_rsp->us_status_code, pst_hmac_vap->uc_auth_cnt);

    if (((pst_auth_rsp->us_status_code == MAC_UNSUPT_ALG) ||
         (pst_hmac_vap->uc_auth_cnt >= MAX_AUTH_CNT)) &&
        (pst_hmac_vap->en_auth_mode == WLAN_WITP_AUTH_AUTOMATIC)) {
        pst_hmac_vap->en_auth_mode = WLAN_WITP_AUTH_SHARED_KEY;
        /* 需要将状态机设置为 */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_JOIN_COMP);

        /* 更新AUTH的次数 */
        pst_hmac_vap->uc_auth_cnt = 0;

        /* 重新发起关联动作 */
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));

        return;
    }

    if (pst_hmac_vap->uc_auth_cnt < MAX_AUTH_CNT) {
        /* 需要将状态机设置为 */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_JOIN_COMP);

        /* 更新AUTH的次数 */
        pst_hmac_vap->uc_auth_cnt++;

        /* 重新发起关联动作 */
        hmac_sta_initiate_auth(&(pst_hmac_vap->st_vap_base_info));

        return;
    } else {
        hmac_config_reg_info(&(pst_hmac_vap->st_vap_base_info), OAL_SIZEOF(auc_param), auc_param);
    }

#ifdef _PRE_WLAN_FEATURE_SAE
    if (pst_hmac_vap->en_auth_mode == WLAN_WITP_AUTH_SAE && pst_auth_rsp->us_status_code != MAC_AP_FULL) {
        /* SAE关联失败，上报停止external auth到wpa_s */
        hmac_report_external_auth_req(pst_hmac_vap, NL80211_EXTERNAL_AUTH_ABORT);
    }
#endif

    hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);

    /* 获取用户指针 */
    pst_hmac_user = mac_res_get_hmac_user((oal_uint16)pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    if (pst_hmac_user != OAL_PTR_NULL) {
        /* 删除对应用户 */
        hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
    } else {
        /* The MAC state is changed to fake up state. Further MLME */
        /* requests are processed in this state. */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "auth pst_hmac_user[%d] NULL.",
                         pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
    }

#ifdef _PRE_WLAN_1102A_CHR
    hmac_chr_connect_fail_query_and_report(pst_hmac_vap, pst_auth_rsp->us_status_code);
#endif
    /* 上报关联失败到wpa_supplicant */
    hmac_report_connect_failed_result(pst_hmac_vap, pst_auth_rsp->us_status_code,
        pst_hmac_vap->st_vap_base_info.auc_bssid);
}

static uint32_t hmac_sta_prepare_connect_succ_data_to_host(hmac_vap_stru *pst_hmac_vap,
    hmac_asoc_rsp_stru *pst_asoc_rsp)
{
    oal_uint8     *puc_mgmt_data_rsp = NULL;
    oal_uint8     *puc_mgmt_data_req = NULL;

    /* : asoc_rsp 帧拷贝一份上报上层,防止帧内容上报wal侧处理后被hmac侧释放 */
    puc_mgmt_data_rsp = (oal_uint8 *)oal_memalloc(pst_asoc_rsp->ul_asoc_rsp_ie_len);
    if (puc_mgmt_data_rsp == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_handle_asoc_rsp_sta::mgmt_data alloc null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    memcpy_s(puc_mgmt_data_rsp, pst_asoc_rsp->ul_asoc_rsp_ie_len,
        (oal_uint8 *)pst_asoc_rsp->puc_asoc_rsp_ie_buff, pst_asoc_rsp->ul_asoc_rsp_ie_len);
    pst_asoc_rsp->puc_asoc_rsp_ie_buff = puc_mgmt_data_rsp;

    /* 准备上报关联请求内存 */
    if (pst_asoc_rsp->ul_asoc_req_ie_len != 0 && pst_asoc_rsp->puc_asoc_req_ie_buff != NULL) {
        puc_mgmt_data_req = (oal_uint8 *)oal_memalloc(pst_asoc_rsp->ul_asoc_req_ie_len);
        if (puc_mgmt_data_req == NULL) {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                "{hmac_handle_asoc_rsp_succ_sta::puc_mgmt_data_req alloc null.}");
            hmac_handle_free_buff(puc_mgmt_data_rsp, puc_mgmt_data_req);
            return OAL_ERR_CODE_PTR_NULL;
        }
        memcpy_s(puc_mgmt_data_req, pst_asoc_rsp->ul_asoc_req_ie_len,
            (oal_uint8 *)pst_asoc_rsp->puc_asoc_req_ie_buff, pst_asoc_rsp->ul_asoc_req_ie_len);
        pst_asoc_rsp->puc_asoc_req_ie_buff = puc_mgmt_data_req;
    }

    return OAL_SUCC;
}

/*
 * 功能描述：STA关联成功处理
 */
void hmac_handle_assoc_rsp_succ_sta(hmac_vap_stru *pst_hmac_vap, hmac_asoc_rsp_stru *pst_asoc_rsp)
{
    frw_event_mem_stru      *pst_event_mem = NULL;
    frw_event_stru          *pst_event = NULL;
    mac_vap_stru            *pst_mac_vap = NULL;
    uint32_t ret;

    if (oal_any_null_ptr2(pst_hmac_vap, pst_asoc_rsp)) {
        return;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
#ifdef _PRE_WLAN_NARROW_BAND
    if (g_hitalk_status & NARROW_BAND_ON_MASK) {
        mac_cfg_ps_open_stru st_ps_open;
        st_ps_open.uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_CMD;
        st_ps_open.uc_pm_enable = 0;
        hmac_config_set_sta_pm_on(pst_mac_vap, OAL_SIZEOF(st_ps_open), (oal_uint8 *)&st_ps_open);
    }
#endif

    if (hmac_sta_prepare_connect_succ_data_to_host(pst_hmac_vap, pst_asoc_rsp) != OAL_SUCC) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_handle_assoc_rsp_succ_sta::failed.}");
        return;
    }

    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(hmac_asoc_rsp_stru));
    if (pst_event_mem == OAL_PTR_NULL) {
        hmac_handle_free_buff(pst_asoc_rsp->puc_asoc_req_ie_buff, pst_asoc_rsp->puc_asoc_rsp_ie_buff);
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta::frw_event_alloc fail!}");
        return;
    }
    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr), FRW_EVENT_TYPE_HOST_CTX,
        HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA, OAL_SIZEOF(hmac_asoc_rsp_stru),
        FRW_EVENT_PIPELINE_STAGE_0, pst_mac_vap->uc_chip_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_vap_id);

    if (memcpy_s((oal_uint8 *)frw_get_event_payload(pst_event_mem), OAL_SIZEOF(hmac_asoc_rsp_stru),
        (oal_uint8 *)pst_asoc_rsp, OAL_SIZEOF(hmac_asoc_rsp_stru)) != EOK) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "hmac_handle_asoc_rsp_sta::memcpy fail!");
        frw_event_free_m(pst_event_mem);
        hmac_handle_free_buff(pst_asoc_rsp->puc_asoc_req_ie_buff, pst_asoc_rsp->puc_asoc_rsp_ie_buff);
        return;
    }

    /* 分发事件 */
    ret = frw_event_dispatch_event(pst_event_mem);
    if (ret != OAL_SUCC) {
        hmac_handle_free_buff(pst_asoc_rsp->puc_asoc_req_ie_buff, pst_asoc_rsp->puc_asoc_rsp_ie_buff);
    }
    frw_event_free_m(pst_event_mem);

    return;
}


oal_void  hmac_handle_asoc_rsp_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_msg)
{
    hmac_asoc_rsp_stru      *pst_asoc_rsp = (hmac_asoc_rsp_stru *)puc_msg;
    hmac_user_stru          *pst_hmac_user = OAL_PTR_NULL;
    mac_vap_stru            *pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    if (pst_asoc_rsp->en_result_code == HMAC_MGMT_SUCCESS) {
        hmac_handle_assoc_rsp_succ_sta(pst_hmac_vap, pst_asoc_rsp);
    } else {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_handle_asoc_rsp_sta::asoc fail[%d],assoc_cnt[%d]}",
                         pst_asoc_rsp->en_result_code, pst_hmac_vap->uc_asoc_cnt);

        if (pst_hmac_vap->uc_asoc_cnt >= MAX_ASOC_CNT) {
            /* 获取用户指针 */
            pst_hmac_user = mac_res_get_hmac_user((oal_uint16)pst_hmac_vap->st_vap_base_info.uc_assoc_vap_id);
            if (pst_hmac_user != NULL) {
                /* 发送去认证帧到AP */
                hmac_mgmt_send_deauth_frame(&pst_hmac_vap->st_vap_base_info,
                    pst_hmac_user->st_user_base_info.auc_user_mac_addr, WLAN_MAC_ADDR_LEN,
                    MAC_AUTH_NOT_VALID, OAL_FALSE);

                /* 删除对应用户 */
                hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
            } else {
                /* 设置状态为FAKE UP */
                hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "asoc:pst_hmac_user[%d] NULL.",
                                 pst_mac_vap->uc_assoc_vap_id);
            }

            /* 同步DMAC状态 */
            hmac_send_connect_result_to_dmac_sta(pst_hmac_vap, OAL_FAIL);

#ifdef _PRE_WLAN_1102A_CHR
            hmac_chr_connect_fail_query_and_report(pst_hmac_vap, pst_asoc_rsp->en_status_code);
#endif
            /* 上报关联失败到wpa_supplicant */
            hmac_report_connect_failed_result(pst_hmac_vap, pst_asoc_rsp->en_status_code,
                pst_hmac_vap->st_vap_base_info.auc_bssid);
        } else {
            /* 需要将状态机设置为 */
            hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_AUTH_COMP);

            /* 发起ASOC的次数 */
            pst_hmac_vap->uc_asoc_cnt++;

            /* 重新发起关联动作 */
            hmac_sta_initiate_asoc(&pst_hmac_vap->st_vap_base_info);
        }
    }
}


oal_void  hmac_send_rsp_to_sme_sta(hmac_vap_stru *pst_hmac_vap, hmac_sme_rsp_enum_uint8 en_type,
                                   oal_uint8 *puc_msg)
{
    g_handle_rsp_func_sta[en_type](pst_hmac_vap, puc_msg);
}


oal_void  hmac_send_rsp_to_sme_ap(hmac_vap_stru *pst_hmac_vap, hmac_ap_sme_rsp_enum_uint8 en_type,
                                  oal_uint8 *puc_msg)
{
    /* 目前只有DMT使用 */
}



oal_void  hmac_send_connect_result_to_dmac_sta(hmac_vap_stru *pst_hmac_vap, oal_uint32 ul_result)
{
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event = OAL_PTR_NULL;

    /* 抛事件到DMAC, 申请事件内存 */
    pst_event_mem = frw_event_alloc_m(OAL_SIZEOF(oal_uint32));
    if (pst_event_mem == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "pst_event_mem null.");
        return;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    frw_event_hdr_init(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_CONN_RESULT,
                       OAL_SIZEOF(oal_uint32),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    *((oal_uint32 *)(pst_event->auc_event_data)) = ul_result;

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    frw_event_free_m(pst_event_mem);

    return;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_sta_initiate_scan);
oal_module_symbol(hmac_sta_initiate_join);
oal_module_symbol(hmac_cfg80211_start_scan_sta);
oal_module_symbol(hmac_cfg80211_start_sched_scan);
oal_module_symbol(hmac_cfg80211_stop_sched_scan);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0))
oal_module_symbol(hmac_cfg80211_dump_survey);
#endif
/*lint +e578*//*lint +e19*/

