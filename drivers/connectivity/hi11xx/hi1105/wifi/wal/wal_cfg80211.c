

/* 头文件包含 */
#include "oal_net.h"
#include "oal_cfg80211.h"
#include "oam_wdk.h"
#include "securec.h"
#include "wlan_spec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_CFG80211_C

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static oal_kobj_uevent_env_stru g_env;
#endif

/*
 * 函 数 名  : oal_cfg80211_ready_on_channel
 * 功能描述  : 上报linux 内核已经处于指定信道
 */
void oal_cfg80211_ready_on_channel(oal_wireless_dev_stru *pst_wdev,
                                   uint64_t ull_cookie,
                                   oal_ieee80211_channel_stru *pst_chan,
                                   uint32_t ul_duration,
                                   oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE)
    cfg80211_ready_on_channel(pst_wdev, ull_cookie, pst_chan, ul_duration, en_gfp);
#endif
}
/*
 * 函 数 名  : oal_cfg80211_vowifi_report
 * 功能描述  : 上报linux 内核vowifi/volte逻辑切换申请
 */
void oal_cfg80211_vowifi_report(oal_net_device_stru *pst_netdev,
                                oal_gfp_enum_uint8 en_gfp)
{
#ifdef CONFIG_HW_VOWIFI
    /* 此接口为终端实现的内核接口，定义处用内核宏CONFIG_HW_VOWIFI包裹 */
    cfg80211_drv_vowifi(pst_netdev, en_gfp);
#endif /* CONFIG_HW_VOWIFI */
}

/*
 * 函 数 名  : oal_cfg80211_remain_on_channel_expired
 * 功能描述  : 监听超时上报
 */
void oal_cfg80211_remain_on_channel_expired(oal_wireless_dev_stru *pst_wdev,
                                            uint64_t ull_cookie,
                                            oal_ieee80211_channel_stru *pst_listen_channel,
                                            oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE)
    cfg80211_remain_on_channel_expired(pst_wdev,
                                       ull_cookie,
                                       pst_listen_channel,
                                       GFP_ATOMIC);
#endif
}

void oal_cfg80211_mgmt_tx_status(struct wireless_dev *wdev, uint64_t cookie,
                                 const uint8_t *buf, size_t len,
                                 oal_bool_enum_uint8 ack, oal_gfp_enum_uint8 gfp)
{
#if (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_mgmt_tx_status(wdev, cookie, buf, len, ack, gfp);
#endif
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
/* For ko compile */
#else
/*
 * 函 数 名  : oal_cfg80211_calculate_bitrate
 * 功能描述  : oal_cfg80211_new_sta上报new sta事件获取比特率值(参考内核实现)
 *             如果MCS大于等于32，就返回错误OAL_ERR_CODE_CFG80211_MCS_EXCEED
 * 输入参数  : pst_rate: 速率信息结构
 * 返 回 值  : l_bitrate: 比特率
 */
OAL_STATIC int32_t oal_cfg80211_calculate_bitrate(oal_rate_info_stru *pst_rate)
{
    int32_t l_modulation;
    int32_t l_streams;
    int32_t l_bitrate;

    if ((pst_rate->flags & RATE_INFO_FLAGS_MCS) == 0) {
        return pst_rate->legacy;
    }

    /* the formula below does only work for MCS values smaller than 32 */
    if (pst_rate->mcs >= 32) {
        return -OAL_ERR_CODE_CFG80211_MCS_EXCEED;
    }
    /* 根据MCS来获取对应的比特率 */
    l_modulation = pst_rate->mcs & 7; /* 7 掩码，0x111获取调制方式 */
    l_streams = (pst_rate->mcs >> 3) + 1; /* 右移3bit，加1获取流数 */

    l_bitrate = (pst_rate->flags & RATE_INFO_FLAGS_40_MHZ_WIDTH) ? 13500000 : 6500000; /* 13500000, 6500000 时钟 */

    if (l_modulation < 4) { /* 调制方式小于4 */
        l_bitrate *= (l_modulation + 1);
    } else if (l_modulation == 4) {
        l_bitrate *= (l_modulation + 2); /* 调制方式等于4， 加2 */
    } else {
        l_bitrate *= (l_modulation + 3); /* 调制方式大于4， 加3 */
    }
    l_bitrate *= l_streams;

    if (pst_rate->flags & RATE_INFO_FLAGS_SHORT_GI) {
        l_bitrate = (l_bitrate / 9) * 10; /* SHORT_GI时，bitrate = (clk / 9) * 10 */
    }
    /* do NOT round down here, return (bitrate + 50000) / 100000 */
    return (l_bitrate + 50000) / 100000;
}

/*
 * 函 数 名  : oal_nl80211_send_station
 * 功能描述  : netlink上报send new sta事件进行命令符号和属性值填充
 */
OAL_STATIC uint32_t oal_nl80211_send_station(oal_netbuf_stru *pst_buf, uint32_t ul_pid, uint32_t ul_seq,
                                             int32_t l_flags, oal_net_device_stru *pst_net_dev,
                                             const uint8_t *puc_mac_addr, oal_station_info_stru *pst_station_info)
{
    oal_nlattr_stru *pst_sinfoattr = NULL;
    oal_nlattr_stru *pst_txrate = NULL;
    void *p_hdr = NULL;
    int32_t us_bitrate;

    /* Add generic netlink header to netlink message, returns pointer to user specific header */
    p_hdr = oal_genlmsg_put(pst_buf, ul_pid, ul_seq, NL80211_FAM, l_flags, PRIV_NL80211_CMD_NEW_STATION);
    if (p_hdr == NULL) {
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    oal_nla_put_u32_m(pst_buf, PRIV_NL80211_ATTR_IFINDEX, pst_net_dev->ifindex);
    oal_nla_put_m(pst_buf, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr);

    /* linux-2.6.30内核station_info没有generation成员 */
    pst_sinfoattr = oal_nla_nest_start(pst_buf, PRIV_NL80211_ATTR_STA_INFO);
    if (pst_sinfoattr == NULL) {
        oal_genlmsg_cancel(pst_buf, p_hdr);
        return OAL_ERR_CODE_CFG80211_EMSGSIZE;
    }
    if (pst_station_info->filled & STATION_INFO_INACTIVE_TIME) {
        oal_nla_put_u32_m(pst_buf, PRIV_NL80211_STA_INFO_INACTIVE_TIME, pst_station_info->inactive_time);
    }
    if (pst_station_info->filled & STATION_INFO_RX_BYTES) {
        oal_nla_put_u32_m(pst_buf, PRIV_NL80211_STA_INFO_RX_BYTES, pst_station_info->rx_bytes);
    }
    if (pst_station_info->filled & STATION_INFO_TX_BYTES) {
        oal_nla_put_u32_m(pst_buf, PRIV_NL80211_STA_INFO_TX_BYTES, pst_station_info->tx_bytes);
    }
    if (pst_station_info->filled & STATION_INFO_LLID) {
        oal_nla_put_u16_m(pst_buf, PRIV_NL80211_STA_INFO_LLID, pst_station_info->llid);
    }
    if (pst_station_info->filled & STATION_INFO_PLID) {
        oal_nla_put_u16_m(pst_buf, PRIV_NL80211_STA_INFO_PLID, pst_station_info->plid);
    }
    if (pst_station_info->filled & STATION_INFO_PLINK_STATE) {
        oal_nla_put_u8(pst_buf, PRIV_NL80211_STA_INFO_PLINK_STATE, pst_station_info->plink_state);
    }
    if (pst_station_info->filled & STATION_INFO_SIGNAL) {
        oal_nla_put_u8(pst_buf, PRIV_NL80211_STA_INFO_SIGNAL, pst_station_info->signal);
    }
    if (pst_station_info->filled & STATION_INFO_TX_BITRATE) {
        pst_txrate = oal_nla_nest_start(pst_buf, PRIV_NL80211_STA_INFO_TX_BITRATE);
        if (pst_txrate == NULL) {
            oal_genlmsg_cancel(pst_buf, p_hdr);
            return OAL_ERR_CODE_CFG80211_EMSGSIZE;
        }

        /* cfg80211_calculate_bitrate will return negative for mcs >= 32 */
        us_bitrate = oal_cfg80211_calculate_bitrate(&pst_station_info->txrate);
        if (us_bitrate > 0) {
            oal_nla_put_u16_m(pst_buf, PRIV_NL80211_RATE_INFO_BITRATE, us_bitrate);
        }
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_MCS) {
            oal_nla_put_u8(pst_buf, PRIV_NL80211_RATE_INFO_MCS, pst_station_info->txrate.mcs);
        }
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_40_MHZ_WIDTH) {
            oal_nla_put_flag(pst_buf, PRIV_NL80211_RATE_INFO_40_MHZ_WIDTH);
        }
        if (pst_station_info->txrate.flags & RATE_INFO_FLAGS_SHORT_GI) {
            oal_nla_put_flag(pst_buf, PRIV_NL80211_RATE_INFO_SHORT_GI);
        }

        oal_nla_nest_end(pst_buf, pst_txrate);
    }
    if (pst_station_info->filled & STATION_INFO_RX_PACKETS) {
        oal_nla_put_u32_m(pst_buf, PRIV_NL80211_STA_INFO_RX_PACKETS, pst_station_info->rx_packets);
    }
    if (pst_station_info->filled & STATION_INFO_TX_PACKETS) {
        oal_nla_put_u32_m(pst_buf, PRIV_NL80211_STA_INFO_TX_PACKETS, pst_station_info->tx_packets);
    }

    oal_nla_nest_end(pst_buf, pst_sinfoattr);

    if (oal_genlmsg_end(pst_buf, p_hdr) < 0) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_buf, p_hdr);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}


/*
 * 函 数 名  : oal_nl80211_send_connect_result
 * 功能描述  : 驱动调用内核netlink接口上报关联结构
 */
OAL_STATIC uint32_t oal_nl80211_send_connect_result(oal_netbuf_stru *pst_buf,
                                                    oal_net_device_stru *pst_net_device,
                                                    const uint8_t *puc_bssid,
                                                    const uint8_t *puc_req_ie,
                                                    uint32_t ul_req_ie_len,
                                                    const uint8_t *puc_resp_ie,
                                                    uint32_t ul_resp_ie_len,
                                                    uint16_t us_status,
                                                    oal_gfp_enum_uint8 en_gfp)
{
    void *p_hdr = NULL;
    int32_t ul_let;

    p_hdr = oal_genlmsg_put(pst_buf, 0, 0, NL80211_FAM, 0, PRIV_NL80211_CMD_CONNECT);
    if (p_hdr == NULL) {
        oal_nlmsg_free(pst_buf);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    /* 这个 PRIV_NL80211_ATTR_WIPHY 属性上层没有解析，可以不上报 */
    oal_nla_put_u32_m(pst_buf, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);
    if (puc_bssid != NULL) {
        oal_nla_put_m(pst_buf, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_bssid);
    }
    oal_nla_put_u16_m(pst_buf, PRIV_NL80211_ATTR_STATUS_CODE, us_status);
    if (puc_req_ie != NULL) {
        oal_nla_put_m(pst_buf, PRIV_NL80211_ATTR_REQ_IE, ul_req_ie_len, puc_req_ie);
    }
    if (puc_resp_ie != NULL) {
        oal_nla_put_m(pst_buf, PRIV_NL80211_ATTR_RESP_IE, ul_resp_ie_len, puc_resp_ie);
    }

    if (oal_genlmsg_end(pst_buf, p_hdr) < 0) {
        oal_nlmsg_free(pst_buf);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    ul_let = oal_genlmsg_multicast(pst_buf, 0, NL80211_GID, en_gfp);
    if (ul_let < 0) {
        /* 如果不加载hostapd和wpa_supplicant的话，这个也会失败，这里报fail，影响使用，去掉报错 */
        return OAL_FAIL;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_buf, p_hdr);
    oal_nlmsg_free(pst_buf);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}

OAL_STATIC uint32_t oal_nl80211_send_disconnected(oal_net_device_stru *pst_net_device,
                                                  uint16_t us_reason,
                                                  const uint8_t *puc_ie,
                                                  uint32_t ul_ie_len,
                                                  oal_bool_enum_uint8 from_ap,
                                                  oal_gfp_enum_uint8 en_gfp)
{
    oal_netbuf_stru *pst_msg = NULL;
    void *p_hdr = NULL;
    int32_t ul_let;

    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, NL80211_FAM, 0, PRIV_NL80211_CMD_DISCONNECT);
    if (p_hdr == NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    /* PRIV_NL80211_ATTR_WIPHY 属性上层没有解析，可以不上报 */
    oal_nla_put_u32_m(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);
    if (from_ap && us_reason) {
        oal_nla_put_u16_m(pst_msg, PRIV_NL80211_ATTR_REASON_CODE, us_reason);
    }
    if (from_ap) {
        oal_nla_put_flag(pst_msg, PRIV_NL80211_ATTR_DISCONNECTED_BY_AP);
    }
    if (puc_ie == NULL) {
        oal_nla_put_m(pst_msg, PRIV_NL80211_ATTR_IE, ul_ie_len, puc_ie);
    }

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    ul_let = oal_genlmsg_multicast(pst_msg, 0, NL80211_GID, en_gfp);
    if (ul_let < 0) {
        /* oal_genlmsg_multicast接口内部会释放skb，返回失败不需要手动释放 */
        return OAL_FAIL;
    }

    return OAL_SUCC;

nla_put_failure:
    oal_genlmsg_cancel(pst_msg, p_hdr);
    oal_nlmsg_free(pst_msg);
    return OAL_ERR_CODE_CFG80211_EMSGSIZE;
}
#endif
#ifdef _PRE_WLAN_CHBA_MGMT
void cfg80211_connect_ready_report_chba(oal_net_device_stru *dev,
    const u8 *mac_addr, u8 ntf_id, u16 status, gfp_t gfp)
{
    struct sk_buff *msg = NULL;
    oal_wireless_dev_stru *wdev = NULL;

    if (!dev || !mac_addr) {
        return;
    }
    wdev = oal_netdevice_wdev(dev);
    if (wdev == NULL || wdev->wiphy == NULL) {
        printk("cfg80211_connect_ready_report_chba::wdev or wdev->wiphy is null");
        return;
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
    msg = __cfg80211_alloc_event_skb(wdev->wiphy, wdev, NL80211_CMD_VENDOR,
        NL80211_ATTR_VENDOR_DATA, 0, CHBA_EVENT_ID,
        EVENT_BUF_SIZE + NLMSG_HDRLEN, gfp);
#else
    msg = __cfg80211_alloc_event_skb(wdev->wiphy, wdev, NL80211_CMD_VENDOR,
        NL80211_ATTR_VENDOR_DATA, CHBA_EVENT_ID,
        EVENT_BUF_SIZE + NLMSG_HDRLEN, gfp);
#endif
    if (!msg) {
        return;
    }

    if (nla_put_u8(msg, HID2D_READY2CONN_ATTR_ID, ntf_id) ||
        nla_put_u16(msg, HID2D_READY2CONN_ATTR_STATUS_CODE, status))
        goto nla_put_failure;

    __cfg80211_send_event_skb(msg, gfp);
    return;
 nla_put_failure:
    nlmsg_free(msg);
}

/*
 * 函 数 名  : oal_cfg80211_chba_report
 * 功能描述  : 上报chba netlink消息给WIFI EXT
 */
void oal_cfg80211_chba_report(uint8_t *buf, uint32_t len)
{
#ifdef CONFIG_HW_WIFI_EXT
    cfg80211_drv_hid2d_acs_report(NULL, GFP_KERNEL, buf, len);
#endif
}
uint32_t oal_cfg80211_connect_ready_report(oal_net_device_stru *pst_net_device,
                                           const uint8_t *puc_mac_addr, uint8_t ntf_id,
                                           uint16_t us_status, oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    cfg80211_connect_ready_report_chba(pst_net_device, puc_mac_addr, ntf_id, us_status, en_gfp);
    return OAL_SUCC;
#else
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif
}
#endif
/*
 * 函 数 名  : oal_cfg80211_sched_scan_result
 * 功能描述  : 上报调度扫描结果
 */
void oal_cfg80211_sched_scan_result(oal_wiphy_stru *pst_wiphy)
{
#if (KERNEL_VERSION(4, 12, 0) <= LINUX_VERSION_CODE)
    cfg80211_sched_scan_results(pst_wiphy, 0);
    return;
#elif (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE)
    cfg80211_sched_scan_results(pst_wiphy);
    return;
#else
    /* 51不支持，do nothing */
    return;
#endif
}

void oal_kobject_uevent_env_sta_join(oal_net_device_stru *pst_net_device, const uint8_t *puc_mac_addr)
{
#ifdef _PRE_WINDOWS_SUPPORT
    return;
#else
    memset_s(&g_env, sizeof(g_env), 0, sizeof(g_env));
    /* 上层需要STA_JOIN和mac地址，中间参数无效，但是必须是4个参数 */
    add_uevent_var(&g_env, "SOFTAP=STA_JOIN wlan0 wlan0 %02x:%02x:%02x:%02x:%02x:%02x",
                   puc_mac_addr[BYTE_OFFSET_0], puc_mac_addr[BYTE_OFFSET_1], puc_mac_addr[BYTE_OFFSET_2],
                   puc_mac_addr[BYTE_OFFSET_3], puc_mac_addr[BYTE_OFFSET_4], puc_mac_addr[BYTE_OFFSET_5]);
#if ( KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, g_env.envp);
#else
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, (char **)&g_env);
#endif
#endif
}

void oal_kobject_uevent_env_sta_leave(oal_net_device_stru *pst_net_device, const unsigned char *puc_mac_addr)
{
#ifdef _PRE_WINDOWS_SUPPORT
    return;
#else
    memset_s(&g_env, sizeof(g_env), 0, sizeof(g_env));
    /* 上层需要STA_LEAVE和mac地址，中间参数无效，但是必须是4个参数 */
    add_uevent_var(&g_env, "SOFTAP=STA_LEAVE wlan0 wlan0 %02x:%02x:%02x:%02x:%02x:%02x",
                   puc_mac_addr[BYTE_OFFSET_0], puc_mac_addr[BYTE_OFFSET_1], puc_mac_addr[BYTE_OFFSET_2],
                   puc_mac_addr[BYTE_OFFSET_3], puc_mac_addr[BYTE_OFFSET_4], puc_mac_addr[BYTE_OFFSET_5]);
#if ( KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, g_env.envp);
#else
    kobject_uevent_env(&(pst_net_device->dev.kobj), KOBJ_CHANGE, (char **)&g_env);
#endif
#endif
}

void oal_cfg80211_put_bss(oal_wiphy_stru *pst_wiphy, oal_cfg80211_bss_stru *pst_cfg80211_bss)
{
#if (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE)
    cfg80211_put_bss(pst_wiphy, pst_cfg80211_bss);
#else
    cfg80211_put_bss(pst_cfg80211_bss);
#endif
}

/*
 * 函 数 名  : oal_cfg80211_get_bss
 * 功能描述  : 根据bssid 和ssid 查找内核保存的bss 信息
 */
oal_cfg80211_bss_stru *oal_cfg80211_get_bss(oal_wiphy_stru *pst_wiphy,
                                            oal_ieee80211_channel_stru *pst_channel,
                                            uint8_t *puc_bssid,
                                            uint8_t *puc_ssid,
                                            uint32_t ul_ssid_len)
{
#if ( KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE)
    return cfg80211_get_bss(pst_wiphy, pst_channel, puc_bssid, puc_ssid, ul_ssid_len,
                            IEEE80211_BSS_TYPE_ANY, IEEE80211_PRIVACY_ANY);
#else
    return cfg80211_get_bss(pst_wiphy, pst_channel, puc_bssid, puc_ssid, ul_ssid_len,
                            WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
#endif
}

/*
 * 函 数 名  : oal_cfg80211_unlink_bss
 * 功能描述  : 从kernel删除bss entry项
 */
void oal_cfg80211_unlink_bss(oal_wiphy_stru *pst_wiphy, oal_cfg80211_bss_stru *pst_cfg80211_bss)
{
    cfg80211_unlink_bss(pst_wiphy, pst_cfg80211_bss);
}
oal_cfg80211_bss_stru *oal_cfg80211_inform_bss_frame(oal_wiphy_stru *pst_wiphy,
                                                     oal_ieee80211_channel_stru *pst_ieee80211_channel,
                                                     oal_ieee80211_mgmt_stru *pst_mgmt,
                                                     uint32_t ul_len,
                                                     int32_t l_signal,
                                                     oal_gfp_enum_uint8 en_ftp)
{
    return cfg80211_inform_bss_frame(pst_wiphy, pst_ieee80211_channel, pst_mgmt, ul_len, l_signal, en_ftp);
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0))
void oal_wiphy_ext_feature_set(oal_wiphy_stru *wiphy, enum nl80211_ext_feature_index index)
{
    wiphy_ext_feature_set(wiphy, index);
}
EXPORT_SYMBOL(oal_wiphy_ext_feature_set);
#endif
/*
 * 函 数 名  : oal_cfg80211_scan_done
 * 功能描述  : 上报扫描完成结果
 */
void oal_cfg80211_scan_done(oal_cfg80211_scan_request_stru *pst_cfg80211_scan_request, int8_t c_aborted)
{
#ifdef _PRE_WINDOWS_SUPPORT
    oal_uint32 i;
    if (pst_cfg80211_scan_request == NULL) {
        return;
    }
    for (i = 0; i < WLAN_SCAN_REQ_MAX_SSID; i++) {
        oal_free(pst_cfg80211_scan_request->channels[i]);
    }
    oal_free(pst_cfg80211_scan_request->ssids);
    oal_free(pst_cfg80211_scan_request);
#else
#if (KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE)
    struct cfg80211_scan_info info = {0};
    info.aborted = c_aborted;
    cfg80211_scan_done(pst_cfg80211_scan_request, &info);
#else
    cfg80211_scan_done(pst_cfg80211_scan_request, c_aborted);
#endif
#endif
}

/*
 * 函 数 名  : oal_cfg80211_connect_result
 * 功能描述  : STA上报给关联结果结构体
 */
uint32_t oal_cfg80211_connect_result(oal_net_device_stru *pst_net_device,
                                     const uint8_t *puc_bssid,
                                     const uint8_t *puc_req_ie,
                                     uint32_t ul_req_ie_len,
                                     const uint8_t *puc_resp_ie,
                                     uint32_t ul_resp_ie_len,
                                     uint16_t us_status,
                                     oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_connect_result(pst_net_device, puc_bssid, puc_req_ie, ul_req_ie_len,
                            puc_resp_ie, ul_resp_ie_len, us_status, en_gfp);

    return OAL_SUCC;
#else
    oal_netbuf_stru *pst_msg = NULL;
    oal_wireless_dev_stru *pst_wdev = NULL;

    if (oal_any_null_ptr4(pst_net_device, puc_bssid, puc_resp_ie, puc_req_ie)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 分配一个新的netlink消息 */
    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    pst_wdev = pst_net_device->ieee80211_ptr;
    if (pst_wdev == NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_wdev->iftype != NL80211_IFTYPE_STATION) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /*
     * 如果不加载hostapd和wpa_supplicant的话，这个也会失败，这里报fail，影响使用，去掉报错
     * 成功的话，打印SUCC, 不成功的话，不打印
     */
    return oal_nl80211_send_connect_result(pst_msg, pst_net_device, puc_bssid, puc_req_ie, ul_req_ie_len,
                                           puc_resp_ie, ul_resp_ie_len, us_status, en_gfp);
#endif
}

/*
 * 函 数 名  : oal_cfg80211_disconnected
 * 功能描述  : STA上报给内核去关联结果
 */
uint32_t oal_cfg80211_disconnected(oal_net_device_stru *pst_net_device,
                                   uint16_t us_reason,
                                   uint8_t *puc_ie,
                                   uint32_t ul_ie_len,
                                   oal_gfp_enum_uint8 en_gfp)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
    cfg80211_disconnected(pst_net_device, us_reason, puc_ie, ul_ie_len, 0, en_gfp);

    return OAL_SUCC;
#elif (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_disconnected(pst_net_device, us_reason, puc_ie, ul_ie_len, en_gfp);

    return OAL_SUCC;
#else
    oal_wireless_dev_stru *pst_wdev = NULL;

    if (oal_any_null_ptr1(pst_net_device)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_wdev = pst_net_device->ieee80211_ptr;
    if (pst_wdev == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_wdev->iftype != NL80211_IFTYPE_STATION) {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /*
     * If this disconnect was due to a disassoc, we
     * we might still have an auth BSS around. For
     * the userspace SME that's currently expected,
     * but for the kernel SME (nl80211 CONNECT or
     * wireless extensions) we want to clear up all
     * state.
     */
    return oal_nl80211_send_disconnected(pst_net_device, us_reason, puc_ie,
                                         ul_ie_len, OAL_TRUE, en_gfp);
#endif
}

/*
 * 函 数 名  : oal_cfg80211_roamed
 * 功能描述  : STA上报给内核去关联结果
 */
uint32_t oal_cfg80211_roamed(oal_net_device_stru *pst_net_device,
                             struct ieee80211_channel *pst_channel,
                             const uint8_t *puc_bssid,
                             const uint8_t *puc_req_ie,
                             uint32_t ul_req_ie_len,
                             const uint8_t *puc_resp_ie,
                             uint32_t ul_resp_ie_len,
                             oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(4, 12, 0) <= LINUX_VERSION_CODE)
    struct cfg80211_roam_info info = {0};
    info.channel = pst_channel;
    info.bssid = puc_bssid;
    info.req_ie = puc_req_ie;
    info.req_ie_len = ul_req_ie_len;
    info.resp_ie = puc_resp_ie;
    info.resp_ie_len = ul_resp_ie_len;
    cfg80211_roamed(pst_net_device, &info, en_gfp);
    return OAL_SUCC;
#elif (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    cfg80211_roamed(pst_net_device, pst_channel, puc_bssid,
                    puc_req_ie, ul_req_ie_len,
                    puc_resp_ie, ul_resp_ie_len, en_gfp);

    return OAL_SUCC;

#else
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_ft_event
 * 功能描述  : STA上报给内核ft事件
 */
uint32_t oal_cfg80211_ft_event(oal_net_device_stru *pst_net_device, oal_cfg80211_ft_event_stru *pst_ft_event)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    cfg80211_ft_event(pst_net_device, pst_ft_event);

    return OAL_SUCC;

#else
    return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif
}

/*
 * 函 数 名  : cfg80211_new_sta
 * 功能描述  : AP上报新关联某个STA情况
 */
uint32_t oal_cfg80211_new_sta(oal_net_device_stru *pst_net_device,
                              const uint8_t *puc_mac_addr,
                              oal_station_info_stru *pst_station_info,
                              oal_gfp_enum_uint8 en_gfp)
{
    oal_kobject_uevent_env_sta_join(pst_net_device, puc_mac_addr);

#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_new_sta(pst_net_device, puc_mac_addr, pst_station_info, en_gfp);

    return OAL_SUCC;
#else
    oal_netbuf_stru *pst_msg = NULL;
    uint32_t ul_ret;
    int32_t l_let;

    /* 分配一个新的netlink消息 */
    pst_msg = oal_nlmsg_new(OAL_NLMSG_GOODSIZE, en_gfp);
    if (pst_msg == NULL) {
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    ul_ret = oal_nl80211_send_station(pst_msg, 0, 0, 0, pst_net_device, puc_mac_addr, pst_station_info);
    if (ul_ret != OAL_SUCC) {
        oal_nlmsg_free(pst_msg);
        return ul_ret;
    }

    /* 调用封装的内核netlink广播发送函数，发送成功返回0，失败为负值 */
    l_let = oal_genlmsg_multicast(pst_msg, 0, NL80211_GID, en_gfp);
    if (l_let < 0) {
        return OAL_FAIL;
    }

    return OAL_SUCC;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_mic_failure
 * 功能描述  : 上报mic攻击
 */
void oal_cfg80211_mic_failure(oal_net_device_stru *pst_net_device,
                              const uint8_t *puc_mac_addr,
                              enum nl80211_key_type key_type,
                              int32_t key_id,
                              const uint8_t *puc_tsc,
                              oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    cfg80211_michael_mic_failure(pst_net_device, puc_mac_addr, key_type, key_id, puc_tsc, en_gfp);
#else
    oal_wireless_dev_stru *pst_wdev = NULL;
    oal_cfg80211_registered_device_stru *pst_rdev = NULL;
    oal_netbuf_stru *pst_msg = NULL;
    void *p_hdr = NULL;

    if (oal_any_null_ptr1(pst_net_device)) {
        return;
    }
    pst_wdev = pst_net_device->ieee80211_ptr;
    if ((pst_wdev == NULL) || (pst_wdev->wiphy == NULL)) {
        return;
    }

    pst_rdev = oal_wiphy_to_dev(pst_wdev->wiphy);

    pst_msg = oal_nlmsg_new(OAL_NLMSG_DEFAULT_SIZE, en_gfp);
    if (pst_msg == NULL) {
        return;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, NL80211_FAM, 0, PRIV_NL80211_CMD_MICHAEL_MIC_FAILURE);
    if (p_hdr == NULL) {
        oal_nlmsg_free(pst_msg);
        return;
    }

    /* rdev对应内核core.h中的cfg80211_registered_device结构体，这个属性在上层没有处理 */
    oal_nla_put_u32_m(pst_msg, PRIV_NL80211_ATTR_WIPHY, pst_rdev->wiphy_idx);
    oal_nla_put_u32_m(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex);

    if (puc_mac_addr != NULL) {
        oal_nla_put_m(pst_msg, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr);
    }
    oal_nla_put_u32_m(pst_msg, PRIV_NL80211_ATTR_KEY_TYPE, key_type);
    oal_nla_put_u8(pst_msg, PRIV_NL80211_ATTR_KEY_IDX, key_id);
    if (puc_tsc != NULL) {
        oal_nla_put_m(pst_msg, PRIV_NL80211_ATTR_KEY_SEQ, 6, puc_tsc); /* 6为附加的长度 */
    }

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return;
    }

    if (oal_genlmsg_multicast(pst_msg, 0, NL80211_GID, en_gfp) < 0) {
        return;
    }
    return;
nla_put_failure:
    oal_genlmsg_cancel(pst_msg, p_hdr);
    oal_nlmsg_free(pst_msg);
    return;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_del_sta
 * 功能描述  : AP上报去关联某个STA情况
 */
int32_t oal_cfg80211_del_sta(oal_net_device_stru *pst_net_device,
    const uint8_t *puc_mac_addr, uint8_t len, oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    oal_kobject_uevent_env_sta_leave(pst_net_device, puc_mac_addr);
#endif

#if (KERNEL_VERSION(3, 10, 44) <= LINUX_VERSION_CODE) || (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    cfg80211_del_sta(pst_net_device, puc_mac_addr, en_gfp);

    return OAL_SUCC;
#else

    oal_netbuf_stru *pst_msg = NULL;
    void *p_hdr = NULL;
    int32_t l_let;

    pst_msg = oal_nlmsg_new(OAL_NLMSG_DEFAULT_SIZE, en_gfp);
    if (pst_msg == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    p_hdr = oal_genlmsg_put(pst_msg, 0, 0, NL80211_FAM, 0, PRIV_NL80211_CMD_DEL_STATION);
    if (p_hdr == NULL) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_SKB_MEM_FAIL;
    }

    if (oal_nla_put_u32(pst_msg, PRIV_NL80211_ATTR_IFINDEX, pst_net_device->ifindex) ||
        oal_nla_put(pst_msg, PRIV_NL80211_ATTR_MAC, OAL_ETH_ALEN_SIZE, puc_mac_addr)) {
        oal_genlmsg_cancel(pst_msg, p_hdr);
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_EMSGSIZE;
    }

    if (oal_genlmsg_end(pst_msg, p_hdr) < 0) {
        oal_nlmsg_free(pst_msg);
        return OAL_ERR_CODE_CFG80211_ENOBUFS;
    }

    /*
     * liuux-2.6.30和liuux-2.6.34内核都是从这个函数上,都能达到要求
     * linux-2.6.34内核接着调用genlmsg_multicast_netns(&init_net......)
     * linux-2.6.30内核接着调用nlmsg_multicast(genl_sock......)
     */
    l_let = oal_genlmsg_multicast(pst_msg, 0, NL80211_GID, en_gfp);

    return l_let;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_rx_mgmt
 * 功能描述  : 上报接收到的管理帧
 * 输出参数  : OAL_SUCC 上报成功，其它错误码 上报失败
 */
uint32_t oal_cfg80211_rx_mgmt(oal_net_device_stru *pst_dev,
                              int32_t l_freq,
                              uint8_t uc_rssi,
                              const uint8_t *puc_buf,
                              uint32_t ul_len,
                              oal_gfp_enum_uint8 en_gfp)
{
    oal_wireless_dev_stru *pst_wdev = NULL;
    uint32_t ul_ret;
    oal_bool_enum_uint8 uc_ret;
    if (oal_any_null_ptr2(pst_dev, pst_dev->ieee80211_ptr)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_wdev = pst_dev->ieee80211_ptr;
    uc_ret = cfg80211_rx_mgmt(pst_wdev, l_freq, 0, puc_buf, ul_len, en_gfp);
    /* 返回值适配 */
    (uc_ret == OAL_TRUE) ? (ul_ret = OAL_SUCC) : (ul_ret = OAL_FAIL);
    return ul_ret;
}

/*
 * 函 数 名  : oal_cfg80211_rx_exception
 * 功能描述  : 收到异常后上报上层,私有命令
 */
uint32_t oal_cfg80211_rx_exception(oal_net_device_stru *pst_netdev,
                                   uint8_t *puc_data,
                                   uint32_t ul_data_len)
{
    dev_netlink_send(puc_data, ul_data_len);
    oal_io_print("DFR OAL send[%s] over\n", puc_data);
    return OAL_SUCC;
}

/*
 * 函 数 名  : oal_cfg80211_vendor_cmd_alloc_reply_skb
 * 功能描述  : 申请厂家自定义返回数据
 * 输入参数  : oal_wiphy_stru * pst_wiphy: wiphy 结构
 *             uint32_t     ul_len     : 申请长度
 */
oal_netbuf_stru *oal_cfg80211_vendor_cmd_alloc_reply_skb(oal_wiphy_stru *pst_wiphy, uint32_t ul_len)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    return cfg80211_vendor_cmd_alloc_reply_skb(pst_wiphy, ul_len);
#else
    return NULL;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_vendor_cmd_reply
 * 功能描述  : 厂家自定义数据上报
 * 输入参数  : oal_netbuf_stru *pst_skb: 返回数据
 */
int32_t oal_cfg80211_vendor_cmd_reply(oal_netbuf_stru *pst_skb)
{
#if (KERNEL_VERSION(2, 6, 34) <= LINUX_VERSION_CODE)
    return cfg80211_vendor_cmd_reply(pst_skb);
#else
    return OAL_SUCC;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_m2s_status_report
 * 功能描述  : 上报linux 内核m2s切换结果
 */
void oal_cfg80211_m2s_status_report(oal_net_device_stru *pst_netdev,
                                    oal_gfp_enum_uint8 en_gfp, uint8_t *puc_buf, uint32_t ul_len)
{
#ifdef CONFIG_HW_WIFI_MSS
    /* 此接口为终端实现的内核接口，定义处用内核宏CONFIG_HW_WIFI_MSS包裹 */
    cfg80211_drv_mss_result(pst_netdev, en_gfp, puc_buf, ul_len);
#endif
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
/*
 * 函 数 名  : oal_cfg80211_tas_rssi_access_report
 * 功能描述  : 上报linux 内核TAS天线测量结果
 */
void oal_cfg80211_tas_rssi_access_report(oal_net_device_stru *pst_netdev, oal_gfp_enum_uint8 en_gfp,
                                         uint8_t *puc_buf, uint32_t ul_len)
{
#ifdef CONFIG_HW_WIFI_RSSI
    cfg80211_drv_tas_result(pst_netdev, en_gfp, puc_buf, ul_len);
#endif
}
#endif

#ifdef _PRE_WLAN_FEATURE_HID2D
/*
 * 函 数 名  : oal_cfg80211_hid2d_acs_test
 * 功能描述  : 上报内核acs参数
 */
void oal_cfg80211_hid2d_acs_report(oal_net_device_stru *netdev, oal_gfp_enum_uint8 gfp,
    uint8_t *buf, uint32_t len)
{
#ifdef CONFIG_HW_WIFI_EXT
    cfg80211_drv_hid2d_acs_report(netdev, gfp, buf, len);
#endif
}
#endif
/*
 * 函 数 名  : oal_cfg80211_external_auth_request
 * 功能描述  : 上报内核external_auth 事件
 */
int oal_cfg80211_external_auth_request(oal_net_device_stru *pst_netdev,
                                       oal_cfg80211_external_auth_stru *pst_params,
                                       oal_gfp_enum_uint8 en_gfp)
{
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
    return cfg80211_external_auth_request(pst_netdev, pst_params, en_gfp);
#else
    return -OAL_EFAIL;
#endif
}

/*
 * 函 数 名  : oal_cfg80211_ch_switch_notify
 * 功能描述  : 上报内核ch_switch 事件
 */
void oal_cfg80211_ch_switch_notify(oal_net_device_stru *pst_netdev,
                                   oal_cfg80211_chan_def *pst_chandef)
{
    cfg80211_ch_switch_notify(pst_netdev,
                              pst_chandef);
}

/*
 * 函 数 名  : oal_cfg80211_vendor_event_alloc
 * 功能描述  : 申请vendor事件内存
 */
oal_netbuf_stru *oal_cfg80211_vendor_event_alloc(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    int approxlen, int event_idx, oal_gfp_enum_uint8 gfp)
{
    return cfg80211_vendor_event_alloc(wiphy, wdev, approxlen, event_idx, gfp);
}

/*
 * 函 数 名  : oal_cfg80211_vendor_event
 * 功能描述  : 上报vendor事件
 */
void oal_cfg80211_vendor_event(oal_netbuf_stru *skb, oal_gfp_enum_uint8 gfp)
{
    cfg80211_vendor_event(skb, gfp);
}
#endif

