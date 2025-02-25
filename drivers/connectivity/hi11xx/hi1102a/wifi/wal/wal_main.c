

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "hmac_ext_if.h"

#include "wal_main.h"
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#ifdef CONFIG_DOZE_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#include "wal_config.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_cfg80211.h"
#include "wal_linux_flowctl.h"

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#include "oal_kernel_file.h"
#endif

#include "wal_dfx.h"

#include "hmac_vap.h"
#include "hmac_tx_data.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_MAIN_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* HOST CRX子表 */
OAL_STATIC frw_event_sub_table_item_stru g_ast_wal_host_crx_table[WAL_HOST_CRX_SUBTYPE_BUTT];

/* HOST CTX字表 */
OAL_STATIC frw_event_sub_table_item_stru g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_BUTT];

/* HOST DRX子表 */
/* wal对外钩子函数 */
oam_wal_func_hook_stru g_st_wal_drv_func_hook;

oal_wakelock_stru g_st_wal_wakelock;
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
wal_hw_wlan_filter_ops g_st_ip_filter_ops = {
    .set_filter_enable = wal_set_ip_filter_enable,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
    .set_filter_enable_ex = wal_set_assigned_filter_enable,
#endif
    .add_filter_items = wal_add_ip_filter_items,
    .clear_filters = wal_clear_ip_filter,
    .get_filter_pkg_stat = NULL,
};
#else
wal_hw_wlan_filter_ops g_st_ip_filter_ops;
#endif
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_void wal_event_fsm_init(oal_void)
{
    g_ast_wal_host_crx_table[WAL_HOST_CRX_SUBTYPE_CFG].p_func = wal_config_process_pkt;
    frw_event_table_register(FRW_EVENT_TYPE_HOST_CRX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_wal_host_crx_table);

    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA].p_func = wal_scan_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA].p_func = wal_asoc_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_DISASOC_COMP_STA].p_func = wal_disasoc_comp_proc_sta;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_CONNECT_AP].p_func = wal_connect_new_sta_proc_ap;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_DISCONNECT_AP].p_func = wal_disconnect_sta_proc_ap;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_MIC_FAILURE].p_func = wal_mic_failure_proc;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_RX_MGMT].p_func = wal_send_mgmt_to_host;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_LISTEN_EXPIRED].p_func = wal_p2p_listen_timeout;
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_INIT].p_func = wal_cfg80211_init_evt_handle;
#if (_PRE_CONFIG_TARGET_PRODUCT != _PRE_TARGET_PRODUCT_TYPE_E5)
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_MGMT_TX_STATUS].p_func = wal_cfg80211_mgmt_tx_status;
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_ROAM_COMP_STA].p_func = wal_roam_comp_proc_sta;
#endif  // _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_FT_EVENT_STA].p_func = wal_ft_event_proc_sta;
#endif  // _PRE_WLAN_FEATURE_11R
#ifdef _PRE_WLAN_FEATURE_VOWIFI
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_VOWIFI_REPORT].p_func = wal_cfg80211_vowifi_report;
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE)
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_SAMPLE_REPORT].p_func = wal_sample_report2sdt;
#endif
#ifdef _PRE_WLAN_FEATURE_SAE
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_EXT_AUTH_REQ].p_func = wal_report_external_auth_req;
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_TAS_NOTIFY_RSSI].p_func =
        wal_cfg80211_tas_rssi_access_report;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 18, 0))
    g_ast_wal_host_ctx_table[HMAC_HOST_CTX_EVENT_SUB_TYPE_CH_SWITCH_NOTIFY].p_func = wal_report_channel_switch;
#endif
    frw_event_table_register(FRW_EVENT_TYPE_HOST_CTX, FRW_EVENT_PIPELINE_STAGE_0, g_ast_wal_host_ctx_table);
}


oal_void wal_event_fsm_exit(oal_void)
{
    OAL_IO_PRINT("debug2");

    memset_s(g_ast_wal_host_crx_table, OAL_SIZEOF(g_ast_wal_host_crx_table), 0, OAL_SIZEOF(g_ast_wal_host_crx_table));

    memset_s(g_ast_wal_host_ctx_table, OAL_SIZEOF(g_ast_wal_host_ctx_table), 0, OAL_SIZEOF(g_ast_wal_host_ctx_table));

    return;
}

#if defined(_PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

/* debug sysfs */
OAL_STATIC oal_kobject *g_conn_syfs_wal_object = NULL;

oal_int32 wal_wakelock_info_print(char *buf, oal_int32 buf_len)
{
    oal_int32 ret;
    ret = snprintf_s(buf, buf_len, buf_len - 1, "hold %lu locks\n", g_st_wal_wakelock.lock_count);
    return ret;
}

OAL_STATIC ssize_t wal_get_wakelock_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    ret += wal_wakelock_info_print(buf, PAGE_SIZE - ret);

    return ret;
}
OAL_STATIC ssize_t wal_get_packet_statistics_wlan0_info(struct kobject *dev,
                                                        struct kobj_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    oal_net_device_stru *pst_net_dev = OAL_PTR_NULL;
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
#ifdef _PRE_WLAN_ATCMD_SUPPORT
    oal_int32 l_rx_pckg_succ_num;
    oal_int32 l_ret;
#endif
    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    pst_net_dev = oal_dev_get_by_name(wal_get_wlan_name());
    if (pst_net_dev == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_packet_statistics_info_print::oal_dev_get_by_name return null ptr}");
        return ret;
    }
    oal_dev_put(pst_net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    /* 获取VAP结构体 */
    pst_vap = (mac_vap_stru *)oal_net_dev_priv(pst_net_dev);
    /* 如果VAP结构体不存在，返回0 */
    if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_packet_statistics_wlan0_info_print::pst_vap = OAL_PTR_NULL!}\r\n");
        return ret;
    }
    /* 非STA直接返回 */
    if (pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_packet_statistics_wlan0_info_print::vap_mode:%d.}", pst_vap->en_vap_mode);
        return ret;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{wal_packet_statistics_wlan0_info_print::pst_hmac_vap null.}");
        return ret;
    }
#ifdef _PRE_WLAN_ATCMD_SUPPORT
    l_ret = wal_atcmsrv_ioctl_get_rx_pckg(pst_net_dev, &l_rx_pckg_succ_num);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_packet_statistics_info_print::get_rx_pckg failed, failed err:%d}", l_ret);
        return ret;
    }

    ret += snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "rx_packet=%d\r\n", l_rx_pckg_succ_num);
#endif
    return ret;
}

OAL_STATIC struct kobj_attribute g_dev_attr_wakelock =
    __ATTR(wakelock, S_IRUGO, wal_get_wakelock_info, NULL);
OAL_STATIC struct kobj_attribute g_dev_attr_packet_statistics_wlan0 =
    __ATTR(packet_statistics_wlan0, S_IRUGO, wal_get_packet_statistics_wlan0_info, NULL);

oal_int32 wal_msg_queue_info_print(char *buf, oal_int32 buf_len)
{
    oal_int32 ret = 0;

    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "message count:%u\n", wal_get_request_msg_count());

    return ret;
}

OAL_STATIC ssize_t wal_get_msg_queue_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    ret += wal_msg_queue_info_print(buf, PAGE_SIZE - ret);

    return ret;
}
OAL_STATIC ssize_t wal_get_dev_wifi_info_print(char *buf, oal_int32 buf_len)
{
    ssize_t ret = 0;
    oal_net_device_stru *pst_net_dev = oal_dev_get_by_name(wal_get_wlan_name());
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
    oal_station_info_stru *pst_station_info = OAL_PTR_NULL;
    station_info_extend_stru *pst_station_info_extend = OAL_PTR_NULL;

    if (buf == NULL) {
        return 0;
    }

    if (pst_net_dev == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_get_dev_wifi_info_print::oal_dev_get_by_name return null ptr!}\r\n");
        return ret;
    }
    oal_dev_put(pst_net_dev); /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    /* 获取VAP结构体 */
    pst_vap = (mac_vap_stru *)oal_net_dev_priv(pst_net_dev);
    /* 如果VAP结构体不存在，返回0 */
    if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_get_dev_wifi_info_print::pst_vap = OAL_PTR_NULL!}\r\n");
        return ret;
    }
    /* 非STA直接返回 */
    if (pst_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_get_dev_wifi_info_print::vap_mode:%d.}\r\n", pst_vap->en_vap_mode);
        return ret;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{wal_get_dev_wifi_info_print::pst_hmac_vap null.}");
        return ret;
    }
    pst_station_info = &(pst_hmac_vap->station_info);
    pst_station_info_extend = &(pst_hmac_vap->st_station_info_extend);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx_frame_amount:%d", pst_station_info->tx_packets);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx_byte_amount:%d",
        (oal_uint32)pst_station_info->tx_bytes);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx_data_frame_error_amount:%d",
        pst_station_info->tx_failed);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tx_retrans_amount:%d",
        pst_station_info->tx_retries);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "rx_frame_amount:%d", pst_station_info->rx_packets);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "rx_byte_amount:%d",
        (oal_uint32)pst_station_info->rx_bytes);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "rx_beacon_from_assoc_ap:%d",
        pst_station_info_extend->ul_bcn_cnt);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "ap_distance:%d",
        pst_station_info_extend->uc_distance);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "disturbing_degree:%d",
        pst_station_info_extend->uc_cca_intr);
    ret += snprintf_s(buf + ret, buf_len - ret, buf_len - ret - 1, "tbtt_cnt%d", pst_station_info_extend->ul_tbtt_cnt);

    return ret;
}
OAL_STATIC ssize_t wal_get_dev_wifi_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    int ret = 0;
    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    ret += wal_get_dev_wifi_info_print(buf, PAGE_SIZE - ret);

    return ret;
}
OAL_STATIC struct kobj_attribute g_dev_attr_dev_wifi_info =
    __ATTR(dev_wifi_info, S_IRUGO, wal_get_dev_wifi_info, NULL);

OAL_STATIC struct kobj_attribute g_dev_attr_msg_queue =
    __ATTR(msg_queue, S_IRUGO, wal_get_msg_queue_info, NULL);

OAL_STATIC struct attribute *g_wal_sysfs_entries[] = {
    &g_dev_attr_wakelock.attr,
    &g_dev_attr_msg_queue.attr,
    &g_dev_attr_packet_statistics_wlan0.attr,
    &g_dev_attr_dev_wifi_info.attr,
    NULL
};

OAL_STATIC struct attribute_group g_wal_attribute_group = {
    // .name = "vap",
    .attrs = g_wal_sysfs_entries,
};

OAL_STATIC oal_void wal_sysfs_entry_init(oal_void)
{
    oal_int32 ret;
    oal_kobject *pst_root_object;
    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::get sysfs root object failed!}");
        return;
    }

    g_conn_syfs_wal_object = kobject_create_and_add("wal", pst_root_object);
    if (g_conn_syfs_wal_object == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::create wal object failed!}");
        return;
    }

    ret = sysfs_create_group(g_conn_syfs_wal_object, &g_wal_attribute_group);
    if (ret) {
        kobject_put(g_conn_syfs_wal_object);
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sysfs_entry_init::sysfs create group failed!}");
        return;
    }
}

OAL_STATIC oal_void wal_sysfs_entry_exit(oal_void)
{
    if (g_conn_syfs_wal_object) {
        sysfs_remove_group(g_conn_syfs_wal_object, &g_wal_attribute_group);
        kobject_put(g_conn_syfs_wal_object);
    }
}
#endif
#ifdef _PRE_WLAN_NARROW_BAND
struct nb_globals g_nb_handler_data = {0};
void nb_netlink_rev(oal_netbuf_stru *skb)
{
    oal_netbuf_stru *pst_skb;
    oal_nlmsghdr_stru *pst_nlh;
    struct nb_msg_hdr_stru st_msg_hdr;
    oal_uint32 ul_len;
    oal_int32 ret;
    oal_net_device_stru *net_dev;
    mac_vap_stru *vap;
    hmac_vap_stru *hmac_vap;

    if (skb == NULL) {
        OAL_IO_PRINT("WIFI nb:para fail\n");
        return;
    }

    OAL_IO_PRINT("WIFI nb:dev_kernel_netlink_recv.\n");

    pst_skb = OAL_PTR_NULL;
    pst_nlh = OAL_PTR_NULL;

    memset_s(g_nb_handler_data.data, OAL_NB_DATA_BUF_LEN, 0, OAL_NB_DATA_BUF_LEN);
    pst_skb = oal_netbuf_get(skb);
    if (pst_skb->len >= oal_nlmsg_space(0)) {
        pst_nlh = oal_nlmsg_hdr(pst_skb);
        /* 检测报文长度正确性 */
        if (!oal_nlmsg_ok(pst_nlh, (oal_uint32)pst_skb->len)) {
            OAL_IO_PRINT("[ERROR]invaild netlink buff data packge data len = :%u,skb_buff data len = %u\n",
                         pst_nlh->nlmsg_len, pst_skb->len);
            kfree_skb(pst_skb);
            return;
        }
        ul_len = oal_nlmsg_payload(pst_nlh, 0);
        /* 后续需要拷贝sizeof(st_msg_hdr),故判断之 */
        if (ul_len < sizeof(st_msg_hdr)) {
            OAL_IO_PRINT("[ERROR]invaild netlink buff len:%u,max len:%u\n", ul_len, OAL_NB_DATA_BUF_LEN);
            kfree_skb(pst_skb);
            return;
        }

        ret = memcpy_s(g_nb_handler_data.data, OAL_NB_DATA_BUF_LEN, oal_nlmsg_data(pst_nlh), ul_len);
        if (ret != EOK) {
            OAL_IO_PRINT("memcpy_s error, destlen=%u, srclen=%u\n", OAL_NB_DATA_BUF_LEN, ul_len);
            kfree_skb(pst_skb);
            return;
        }

        ret = memcpy_s((void *)&st_msg_hdr, sizeof(st_msg_hdr), g_nb_handler_data.data, sizeof(st_msg_hdr));
        if (ret != EOK) {
            OAL_IO_PRINT("memcpy_s error\n");
            kfree_skb(pst_skb);
            return;
        }
        g_nb_handler_data.usepid = pst_nlh->nlmsg_pid; /* pid of sending process */
        OAL_IO_PRINT("WIFI nb:pid is [%d], msg is [%s]\n",
                         g_nb_handler_data.usepid, &g_nb_handler_data.data[sizeof(st_msg_hdr)]);
        net_dev = oal_dev_get_by_name("wlan0");
        if (net_dev != NULL) {
            /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
            oal_dev_put(net_dev);
        }
        if (st_msg_hdr.cmd == NB_GET_REPORT_INFO) {
            wal_hipriv_nb_report_info_read(net_dev, "nb_report_info");
        } else if (st_msg_hdr.cmd == NB_SEND_CTRL_DATA) {
            vap = (mac_vap_stru *)oal_net_dev_priv(net_dev);
            /* 如果VAP结构体不存在，返回0 */
            if (oal_unlikely(vap == NULL)) {
                oam_warning_log0(0, OAM_SF_ANY, "{nb_netlink_rev::pst_vap = OAL_PTR_NULL!}\r\n");
                kfree_skb(pst_skb);
                return;
            }
            /* 非STA直接返回 */
            if (vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA && vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{nb_netlink_rev::vap_mode:%d.}", vap->en_vap_mode);
                kfree_skb(pst_skb);
                return;
            }
            hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(vap->uc_vap_id);
            if (hmac_vap == OAL_PTR_NULL) {
                OAM_ERROR_LOG0(vap->uc_vap_id, OAM_SF_CFG, "{nb_netlink_rev::pst_hmac_vap null.}");
                kfree_skb(pst_skb);
                return;
            }
            hmac_nb_send_action(hmac_vap, vap, &g_nb_handler_data.data[sizeof(st_msg_hdr)], st_msg_hdr.len);
        } else if (st_msg_hdr.cmd == NB_NOKEY_MODE) {
            wal_hipriv_nokey_mode_set(net_dev, &g_nb_handler_data.data[sizeof(st_msg_hdr)]);
        } else if (st_msg_hdr.cmd == NB_KEY_SET) {
            wal_hipriv_hitalk_key_set(net_dev, &g_nb_handler_data.data[sizeof(st_msg_hdr)]);
        }
    }
    /*  Hi110x bug fix / 2014/1/20 begin */
    kfree_skb(pst_skb);
    /*  Hi110x bug fix / 2014/1/20 end */
    return;
}

oal_int32 nb_netlink_create(void)
{
    g_nb_handler_data.nlsk = oal_netlink_kernel_create(&OAL_INIT_NET, NETLINK_NB_REPORT, 0,
        nb_netlink_rev, OAL_PTR_NULL, OAL_THIS_MODULE);
    if (g_nb_handler_data.nlsk == OAL_PTR_NULL) {
        OAL_IO_PRINT("WIFI nb:fail to create netlink socket \n");
        return -OAL_EFAIL;
    }

    OAL_IO_PRINT("WIFI nb:suceed to create netlink socket %p \n", g_nb_handler_data.nlsk);
    return OAL_SUCC;
}
oal_int32 init_nb_handler(oal_void)
{
    oal_int32 ret;

    OAL_IO_PRINT("nb: into init_nb_enable_handler\n");

    memset_s((oal_uint8 *)&g_nb_handler_data, OAL_SIZEOF(g_nb_handler_data),
             0, OAL_SIZEOF(g_nb_handler_data));

    g_nb_handler_data.data = (oal_uint8 *)kzalloc(OAL_NB_DATA_BUF_LEN, GFP_KERNEL);
    if (g_nb_handler_data.data == OAL_PTR_NULL) {
        OAL_IO_PRINT("nb: alloc g_nb_handler_data.puc_data fail, len = %d.\n", OAL_NB_DATA_BUF_LEN);
        g_nb_handler_data.data = OAL_PTR_NULL;
        return -OAL_EFAIL;
    }

    memset_s(g_nb_handler_data.data, OAL_NB_DATA_BUF_LEN, 0, OAL_NB_DATA_BUF_LEN);

    ret = nb_netlink_create();
    if (ret < 0) {
        kfree(g_nb_handler_data.data);
        OAL_IO_PRINT("init_nb_kernel init is ERR!\n");
        return -OAL_EFAIL;
    }

    OAL_IO_PRINT("nb: init_nb_enable_handler init ok.\n");

    return OAL_SUCC;
}
oal_void deinit_nb_handler(oal_void)
{
    if (g_nb_handler_data.nlsk != OAL_PTR_NULL) {
        oal_netlink_kernel_release(g_nb_handler_data.nlsk);
        g_nb_handler_data.usepid = 0;
    }

    if (g_nb_handler_data.data != OAL_PTR_NULL) {
        kfree(g_nb_handler_data.data);
    }

    OAL_IO_PRINT("nb: deinit ok.\n");

    return;
}
#endif

oal_int32 wal_main_init(oal_void)
{
    oal_uint32 ul_ret;
    frw_init_enum_uint16 en_init_state;

    oal_wake_lock_init(&g_st_wal_wakelock, "wlan_wal_lock");
    wal_msg_queue_init();

    en_init_state = frw_get_init_state();
    /* WAL模块初始化开始时，说明HMAC肯定已经初始化成功 */
    if ((en_init_state == FRW_INIT_STATE_BUTT) || (en_init_state < FRW_INIT_STATE_HMAC_CONFIG_VAP_SUCC)) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::en_init_state has a invalid value [%d]!}\r\n", en_init_state);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

    wal_event_fsm_init();

    /* 创建proc */
    ul_ret = wal_hipriv_create_proc(OAL_PTR_NULL);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::wal_hipriv_create_proc return value [%d]!}", ul_ret);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

#ifdef _PRE_WLAN_FEATURE_DFR
    wal_dfx_init();
#endif  // #ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    wal_set_custom_process_func(wal_custom_cali, wal_priv_init_config);
#endif

    /* 初始化每个device硬件设备对应的wiphy */
    ul_ret = wal_cfg80211_init();
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_main_init::wal_cfg80211_init has a wrong return value [%d]!}", ul_ret);

        frw_timer_delete_all_timer();
        return -OAL_EFAIL;
    }

    /* 在host侧如果WAL初始化成功，即为全部初始化成功 */
    frw_set_init_state(FRW_INIT_STATE_ALL_SUCC);

    /* wal钩子函数初始化 */
    wal_drv_cfg_func_hook_init();

    /* wal层对外钩子函数注册至oam模块 */
    oam_wal_func_fook_register(&g_st_wal_drv_func_hook);

#ifdef _PRE_WLAN_FEATURE_P2P
    /* 初始化cfg80211 删除网络设备工作队列 */
    g_del_virtual_inf_workqueue = oal_create_singlethread_workqueue_m("cfg80211_del_virtual_inf");
    if (!g_del_virtual_inf_workqueue) {
        wal_cfg80211_exit();
        frw_timer_delete_all_timer();
        oam_warning_log0(0, OAM_SF_ANY, "{wal_main_init::Failed to create cfg80211 del virtual infterface workqueue!}");

        return -OAL_EFAIL;
    }
#endif /* _PRE_WLAN_FEATURE_P2P */

#if defined(_PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* debug sysfs */
    wal_sysfs_entry_init();
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    wal_register_ip_filter(&g_st_ip_filter_ops);
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#ifdef _PRE_WLAN_NARROW_BAND
    init_nb_handler();
#endif
    return OAL_SUCC;
}


oal_void wal_destroy_all_vap(oal_void)
{
#if (_PRE_TEST_MODE_UT != _PRE_TEST_MODE)

    oal_uint8 uc_vap_id = 0;
    oal_net_device_stru *pst_net_dev;
    oal_wireless_dev_stru *pst_wdev;
    oal_int8 ac_param[10] = { 0 };
    OAL_IO_PRINT("wal_destroy_all_vap start");
    /* 删除业务vap，双芯片id从2开始，增加编译宏表示板级业务vap起始id 后续业务vap的处理，采用此宏  */
    for (uc_vap_id = WLAN_SERVICE_VAP_START_ID_PER_BOARD; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++) {
        pst_net_dev = hmac_vap_get_net_device(uc_vap_id);
        if (pst_net_dev != OAL_PTR_NULL) {
            oal_net_close_dev(pst_net_dev);

            wal_hipriv_del_vap(pst_net_dev, ac_param);
            frw_event_process_all_event(0);
        }
    }

    pst_net_dev = oal_dev_get_by_name(wal_get_wlan_name());
    if (pst_net_dev != OAL_PTR_NULL) {
        oal_dev_put(pst_net_dev);
        pst_wdev = oal_netdevice_wdev(pst_net_dev);
        oal_net_unregister_netdev(pst_net_dev);
        if (pst_wdev) {
            OAL_IO_PRINT("%s:free wlan0 wdev\n", __func__);
            oal_mem_free_m(pst_wdev, OAL_TRUE);
        }
    }
    pst_net_dev = oal_dev_get_by_name("p2p0");
    if (pst_net_dev != OAL_PTR_NULL) {
        oal_dev_put(pst_net_dev);
        pst_wdev = oal_netdevice_wdev(pst_net_dev);
        oal_net_unregister_netdev(pst_net_dev);
        if (pst_wdev) {
            OAL_IO_PRINT("%s:free p2p0 wdev\n", __func__);
            oal_mem_free_m(pst_wdev, OAL_TRUE);
        }
    }

#endif

    return;
}


oal_void wal_main_exit(oal_void)
{
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    wal_set_custom_process_func(OAL_PTR_NULL, OAL_PTR_NULL);
#endif

#if defined(_PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* debug sysfs */
    wal_sysfs_entry_exit();
#endif
    /* down掉所有的vap */
    wal_destroy_all_vap();

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* 卸载每个device硬件设备对应的wiphy */
    wal_cfg80211_exit();
#endif
    wal_event_fsm_exit();

    /* 删除proc */
    wal_hipriv_remove_proc();

    /* 卸载成功时，将初始化状态置为HMAC初始化成功 */
    frw_set_init_state(FRW_INIT_STATE_HMAC_CONFIG_VAP_SUCC);

    /* wal钩子函数去初始化 */
    wal_drv_cfg_func_hook_deinit();

    /* 去注册钩子函数 */
    oam_wal_func_fook_unregister();
#ifdef _PRE_WLAN_FEATURE_P2P
    /* 删除cfg80211 删除网络设备工作队列 */
    if (g_del_virtual_inf_workqueue) {
        oal_destroy_workqueue(g_del_virtual_inf_workqueue);
        g_del_virtual_inf_workqueue = OAL_PTR_NULL;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
    wal_dfx_exit();
#endif  // #ifdef _PRE_WLAN_FEATURE_DFR

    oal_wake_lock_exit(&g_st_wal_wakelock);

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    wal_unregister_ip_filter();
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */
#ifdef _PRE_WLAN_NARROW_BAND
    deinit_nb_handler();
#endif
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(wal_main_init);
oal_module_symbol(wal_main_exit);

oal_module_license("GPL");
/*lint +e578*//*lint +e19*/

