

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "oal_net.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif
#include "mac_frame.h"
#include "mac_data.h"
#include "hmac_rx_data.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_ext_if.h"
#include "hmac_frag.h"
#include "hmac_11i.h"
#include "mac_vap.h"
#include "securec.h"
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
#include "hmac_custom_security.h"
#endif
#include "hmac_blockack.h"
#include "hmac_tcp_opt.h"

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif
#include "hmac_statistic_data_flow.h"
#include "hmac_tcp_ack_filter.h"

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
#include <linux/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <linux/ieee80211.h>
#include <linux/ipv6.h>
#endif

#ifdef CONFIG_HUAWEI_DUBAI
#include <chipset_common/dubai/dubai.h>
#endif
#ifdef _PRE_WLAN_FEATURE_SNIFFER
#include <hwnet/ipv4/sysctl_sniffer.h>
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RX_DATA_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
hmac_packet_check_rx_info_stru g_st_pkt_check_rx_info;
/*****************************************************************************
  3 函数实现
*****************************************************************************/
hmac_packet_check_rx_info_stru *hmac_get_pkt_check_rx_info_addr(oal_void)
{
    return &g_st_pkt_check_rx_info;
}

#ifdef _PRE_WLAN_DFT_DUMP_FRAME
oal_void hmac_rx_report_eth_frame(mac_vap_stru *pst_mac_vap,
                                  oal_netbuf_stru *pst_netbuf)
{
    oal_uint16 us_user_idx = 0;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint8 auc_user_macaddr[WLAN_MAC_ADDR_LEN] = { 0 };
    oal_switch_enum_uint8 en_eth_switch = 0;

    if (oal_unlikely(pst_netbuf == OAL_PTR_NULL)) {
        return;
    }

    /* 将skb的data指针指向以太网的帧头 */
    oal_netbuf_push(pst_netbuf, ETHER_HDR_LEN);

    /* 增加统计信息 */
    /* 获取目的用户资源池id */
    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (oal_unlikely(pst_ether_hdr == OAL_PTR_NULL)) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{hmac_rx_report_eth_frame::pst_ether_hdr null.}");
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
            pst_ether_hdr->auc_ether_shost, WLAN_MAC_ADDR_LEN, &us_user_idx);
        if (ul_ret == OAL_ERR_CODE_PTR_NULL) {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{hmac_rx_report_eth_frame::ul_ret null.}");
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        if (ul_ret == OAL_FAIL) {
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            return;
        }

        oal_set_mac_addr(auc_user_macaddr, pst_ether_hdr->auc_ether_shost);
    } else if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (pst_mac_vap->us_user_nums == 0) {
            oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
            /* SUCC , return */
            return;
        }

        us_user_idx = pst_mac_vap->uc_assoc_vap_id;
        oal_set_mac_addr(auc_user_macaddr, pst_mac_vap->auc_bssid);
    }

    ul_ret = oam_report_eth_frame_get_switch(us_user_idx, OAM_OTA_FRAME_DIRECTION_TYPE_RX, &en_eth_switch);
    if (ul_ret != OAL_SUCC) {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                         "{hmac_rx_report_eth_frame::oam_report_eth_frame_get_switch failed[%d].}", ul_ret);
        oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
        return;
    }

    if (en_eth_switch == OAL_SWITCH_ON) {
        /* 将要送往以太网的帧上报 */
        ul_ret = oam_report_eth_frame(auc_user_macaddr, WLAN_MAC_ADDR_LEN,
            oal_netbuf_data(pst_netbuf), (oal_uint16)oal_netbuf_len(pst_netbuf),
            OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        if (ul_ret != OAL_SUCC) {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                             "{hmac_rx_report_eth_frame::oam_report_eth_frame return err: 0x%x.}\r\n", ul_ret);
        }
    }

    oal_netbuf_pull(pst_netbuf, ETHER_HDR_LEN);
}
#endif


OAL_STATIC OAL_INLINE oal_void hmac_rx_frame_80211_to_eth(oal_netbuf_stru *pst_netbuf,
                                                          const unsigned char *puc_da,
                                                          const unsigned char *puc_sa)
{
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;
    mac_llc_snap_stru *pst_snap = NULL;
    oal_uint16 us_ether_type;
    if (oal_unlikely(oal_netbuf_len(pst_netbuf) < sizeof(mac_llc_snap_stru))) {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "hmac_rx_frame_80211_to_eth::netbuf len[%d]", oal_netbuf_len(pst_netbuf));
        return;
    }

    pst_snap = (mac_llc_snap_stru *)oal_netbuf_data(pst_netbuf);
    us_ether_type = pst_snap->us_ether_type;

    /* 将payload向前扩充6个字节，加上后面8个字节的snap头空间，构成以太网头的14字节空间 */
    oal_netbuf_push(pst_netbuf, HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);

    pst_ether_hdr->us_ether_type = us_ether_type;
    oal_set_mac_addr(pst_ether_hdr->auc_ether_shost, puc_sa);
    oal_set_mac_addr(pst_ether_hdr->auc_ether_dhost, puc_da);
}


oal_void hmac_rx_free_netbuf(oal_netbuf_stru *pst_netbuf, oal_uint16 us_nums)
{
    oal_netbuf_stru *pst_netbuf_temp = OAL_PTR_NULL;
    oal_uint16 us_netbuf_num;

    if (oal_unlikely(pst_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_free_netbuf::pst_netbuf null.}\r\n");
        return;
    }

    for (us_netbuf_num = us_nums; us_netbuf_num > 0; us_netbuf_num--) {
        pst_netbuf_temp = oal_netbuf_next(pst_netbuf);

        /* 减少netbuf对应的user引用计数 */
        oal_netbuf_free(pst_netbuf);

        pst_netbuf = pst_netbuf_temp;

        if (pst_netbuf == OAL_PTR_NULL) {
            if (oal_unlikely(us_netbuf_num != 1)) {
                oam_error_log2(0, OAM_SF_RX, "{hmac_rx_free_netbuf::pst_netbuf list broken,netbuf_num[%d]us_nums[%d].}",
                               us_netbuf_num, us_nums);
                return;
            }
            break;
        }
    }
}


oal_void hmac_rx_free_netbuf_list(oal_netbuf_head_stru *pst_netbuf_hdr, oal_uint16 uc_num_buf)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    oal_uint16 us_idx;

    if (oal_unlikely(pst_netbuf_hdr == OAL_PTR_NULL)) {
        oam_info_log0(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::pst_netbuf null.}");
        return;
    }

    oam_info_log1(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::free [%d].}", uc_num_buf);

    for (us_idx = uc_num_buf; us_idx > 0; us_idx--) {
        pst_netbuf = oal_netbuf_delist(pst_netbuf_hdr);
        if (pst_netbuf != OAL_PTR_NULL) {
            oam_info_log0(0, OAM_SF_RX, "{hmac_rx_free_netbuf_list::pst_netbuf null.}");
            oal_netbuf_free(pst_netbuf);
        }
    }
}


OAL_STATIC oal_uint32 hmac_rx_transmit_to_wlan(frw_event_hdr_stru *pst_event_hdr,
                                               oal_netbuf_head_stru *pst_netbuf_head)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL; /* 从netbuf链上取下来的指向netbuf的指针 */
    oal_uint32 ul_netbuf_num;
    oal_uint32 ul_ret;
    oal_netbuf_stru *pst_buf_tmp = OAL_PTR_NULL; /* 暂存netbuf指针，用于while循环 */
    mac_tx_ctl_stru *pst_tx_ctl = OAL_PTR_NULL;
    mac_vap_stru *pst_mac_vap = OAL_PTR_NULL;

    if (oal_unlikely((pst_event_hdr == OAL_PTR_NULL) || (pst_netbuf_head == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_transmit_to_wlan::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取链头的net buffer */
    pst_netbuf = oal_netbuf_peek(pst_netbuf_head);

    /* 获取mac vap 结构 */
    ul_ret = hmac_tx_get_mac_vap(pst_event_hdr->uc_vap_id, &pst_mac_vap);
    if (oal_unlikely(ul_ret != OAL_SUCC)) {
        ul_netbuf_num = oal_netbuf_list_len(pst_netbuf_head);
        hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)ul_netbuf_num);
        oam_warning_log3(pst_event_hdr->uc_vap_id, OAM_SF_RX,
                         "{hmac_rx_transmit_to_wlan::find vap [%d] failed[%d], free [%d] netbuffer.}",
                         pst_event_hdr->uc_vap_id, ul_ret, ul_netbuf_num);
        return ul_ret;
    }

    /* 循环处理每一个netbuf，按照以太网帧的方式处理 */
    while (pst_netbuf != OAL_PTR_NULL) {
        pst_buf_tmp = oal_netbuf_next(pst_netbuf);

        oal_netbuf_next(pst_netbuf) = OAL_PTR_NULL;
        oal_netbuf_prev(pst_netbuf) = OAL_PTR_NULL;

        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        memset_s(pst_tx_ctl, OAL_SIZEOF(mac_tx_ctl_stru), 0, OAL_SIZEOF(mac_tx_ctl_stru));

        pst_tx_ctl->en_event_type = FRW_EVENT_TYPE_WLAN_DTX;
        pst_tx_ctl->uc_event_sub_type = DMAC_TX_WLAN_DTX;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* set the queue map id when wlan to wlan */
        oal_skb_set_queue_mapping(pst_netbuf, WLAN_NORMAL_QUEUE);
#endif

        ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_netbuf);
        /* 调用失败，自己调用自己释放netbuff内存 */
        if (ul_ret != OAL_SUCC) {
            hmac_free_netbuf_list(pst_netbuf);
        }

        pst_netbuf = pst_buf_tmp;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_rx_free_amsdu_netbuf(oal_netbuf_stru *pst_netbuf)
{
    oal_netbuf_stru *pst_netbuf_next = OAL_PTR_NULL;
    while (pst_netbuf != OAL_PTR_NULL) {
        pst_netbuf_next = oal_get_netbuf_next(pst_netbuf);
        oal_netbuf_free(pst_netbuf);
        pst_netbuf = pst_netbuf_next;
    }
}


OAL_STATIC oal_void hmac_rx_clear_amsdu_last_netbuf_pointer(oal_netbuf_stru *pst_netbuf, oal_uint8 uc_num_buf)
{
    if (uc_num_buf == 0) {
        pst_netbuf->next = OAL_PTR_NULL;
        return;
    }

    while (pst_netbuf != OAL_PTR_NULL) {
        uc_num_buf--;
        if (uc_num_buf == 0) {
            pst_netbuf->next = OAL_PTR_NULL;
            break;
        }
        pst_netbuf = oal_get_netbuf_next(pst_netbuf);
    }
}


OAL_STATIC oal_void hmac_rx_init_amsdu_state(oal_netbuf_stru *pst_netbuf,
                                             dmac_msdu_proc_state_stru *pst_msdu_state)
{
    mac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
    oal_uint32 ul_need_pull_len;

    if ((pst_msdu_state->uc_procd_netbuf_nums == 0) &&
        (pst_msdu_state->uc_procd_msdu_in_netbuf == 0)) {
        pst_msdu_state->pst_curr_netbuf = pst_netbuf;

        /* AMSDU时，首个netbuf的中包含802.11头，对应的payload需要偏移 */
        pst_rx_ctrl = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_msdu_state->pst_curr_netbuf);

        pst_msdu_state->puc_curr_netbuf_data = (oal_uint8 *)mac_get_rx_cb_mac_hdr(pst_rx_ctrl) +
                                               pst_rx_ctrl->uc_mac_header_len;
        pst_msdu_state->uc_netbuf_nums_in_mpdu = pst_rx_ctrl->bit_buff_nums;
        pst_msdu_state->uc_msdu_nums_in_netbuf = pst_rx_ctrl->uc_msdu_in_buffer;
        pst_msdu_state->us_submsdu_offset = 0;
        pst_msdu_state->is_first_buffer = pst_rx_ctrl->bit_is_first_buffer;

        /* 使netbuf 指向amsdu 帧头 */
        ul_need_pull_len = (oal_uint32)(pst_msdu_state->puc_curr_netbuf_data - oal_netbuf_payload(pst_netbuf));
        oal_netbuf_pull(pst_msdu_state->pst_curr_netbuf, ul_need_pull_len);
    }
}


OAL_STATIC oal_uint32 hmac_rx_amsdu_check_frame(dmac_msdu_proc_state_stru *pst_msdu_state,
                                                oal_uint16 us_submsdu_len)
{
    mac_llc_snap_stru *pst_llc_snap = OAL_PTR_NULL;

    if ((pst_msdu_state->pst_curr_netbuf == OAL_PTR_NULL) ||
        (pst_msdu_state->puc_curr_netbuf_data == OAL_PTR_NULL)) {
        return OAL_FAIL;
    }
    if (pst_msdu_state->us_submsdu_offset + MAC_SUBMSDU_HEADER_LEN + us_submsdu_len >
        oal_netbuf_len(pst_msdu_state->pst_curr_netbuf)) {
        oam_warning_log3(0, OAM_SF_RX, "hmac_rx_amsdu_check_frame_len:us_submsdu_len=%d,offset=%d,netbuf_len=%d",
                         us_submsdu_len, pst_msdu_state->us_submsdu_offset,
                         oal_netbuf_len(pst_msdu_state->pst_curr_netbuf));
        return OAL_FAIL;
    }
    pst_llc_snap = (mac_llc_snap_stru *)(pst_msdu_state->puc_curr_netbuf_data + pst_msdu_state->us_submsdu_offset +
                                         MAC_SUBMSDU_HEADER_LEN);
    if ((pst_llc_snap->uc_llc_dsap != 0xAA) ||
        (pst_llc_snap->uc_llc_ssap != 0xAA) ||
        (pst_llc_snap->uc_control != 0x03)) {
        oam_warning_log3(0, OAM_SF_RX, "hmac_rx_amsdu_check_frame_len:dsap=0x%02x,ssap=0x%02x,control=0x%02x",
                         pst_llc_snap->uc_llc_dsap, pst_llc_snap->uc_llc_ssap, pst_llc_snap->uc_control);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC uint32_t hmac_rx_amsdu_is_first_sub_msdu_valid(dmac_msdu_proc_state_stru *msdu_state,
    oal_uint8 *dst_addr, oal_uint8 dst_addr_len)
{
    oal_uint8 mac_addr_snap_header[WLAN_MAC_ADDR_LEN] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00};
    if (!msdu_state->is_first_buffer) {
        return OAL_SUCC;
    }
    if (oal_memcmp(dst_addr, mac_addr_snap_header, WLAN_MAC_ADDR_LEN) == 0) {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_uint32 hmac_rx_parse_amsdu(oal_netbuf_stru *pst_netbuf,
                               dmac_msdu_stru *pst_msdu,
                               dmac_msdu_proc_state_stru *pst_msdu_state,
                               mac_msdu_proc_status_enum_uint8 *pen_proc_state)
{
    mac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;    /* MPDU的控制信息 */
    oal_uint8 *puc_buffer_data_addr = OAL_PTR_NULL; /* 指向netbuf数据域的指针 */
    oal_uint16 us_offset;                       /* submsdu相对于data指针的偏移 */
    oal_uint16 us_submsdu_len = 0;                  /* submsdu的长度 */
    oal_uint8 uc_submsdu_pad_len = 0;               /* submsdu的填充长度 */
    oal_uint8 *puc_submsdu_hdr = OAL_PTR_NULL;      /* 指向submsdu头部的指针 */
    oal_netbuf_stru *pst_netbuf_prev = OAL_PTR_NULL;
    oal_bool_enum_uint8 b_need_free_netbuf = OAL_FALSE;

    /* 首次进入该函数解析AMSDU */
    hmac_rx_init_amsdu_state(pst_netbuf, pst_msdu_state);

    /* 获取submsdu的头指针 */
    puc_buffer_data_addr = pst_msdu_state->puc_curr_netbuf_data;
    us_offset = pst_msdu_state->us_submsdu_offset;
    puc_submsdu_hdr = puc_buffer_data_addr + us_offset;

    /* 1个netbuf 只包含一个msdu */
    if (pst_msdu_state->uc_msdu_nums_in_netbuf == 1) {
        mac_get_submsdu_len(puc_submsdu_hdr, &us_submsdu_len);

        oal_set_mac_addr(pst_msdu->auc_sa, (puc_submsdu_hdr + MAC_SUBMSDU_SA_OFFSET));
        oal_set_mac_addr(pst_msdu->auc_da, (puc_submsdu_hdr + MAC_SUBMSDU_DA_OFFSET));

        if (hmac_rx_amsdu_is_first_sub_msdu_valid(pst_msdu_state, pst_msdu->auc_da, WLAN_MAC_ADDR_LEN) != OAL_SUCC) {
            *pen_proc_state = MAC_PROC_ERROR;
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            oam_warning_log0(0, OAM_SF_RX,
                "{hmac_rx_parse_amsdu::first sub-msdu DA is snap llc header!.}");
            return OAL_FAIL;
        }
        pst_msdu_state->is_first_buffer = OAL_FALSE;

        /* 指向amsdu帧体 */
        oal_netbuf_pull(pst_msdu_state->pst_curr_netbuf, MAC_SUBMSDU_HEADER_LEN);

        if (us_submsdu_len > oal_netbuf_len(pst_msdu_state->pst_curr_netbuf)) {
            *pen_proc_state = MAC_PROC_ERROR;
            oam_warning_log2(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::us_submsdu_len %d is not valid netbuf len=%d.}",
                             us_submsdu_len, oal_netbuf_len(pst_msdu_state->pst_curr_netbuf));
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            return OAL_FAIL;
        }

        oal_netbuf_trim(pst_msdu_state->pst_curr_netbuf, oal_netbuf_len(pst_msdu_state->pst_curr_netbuf));

        oal_netbuf_put(pst_msdu_state->pst_curr_netbuf, us_submsdu_len);

        /* 直接使用该netbuf上报给内核 免去一次netbuf申请和拷贝 */
        b_need_free_netbuf = OAL_FALSE;
        pst_msdu->pst_netbuf = pst_msdu_state->pst_curr_netbuf;
    } else {
        /* 获取submsdu的相关信息 */
        mac_get_submsdu_len(puc_submsdu_hdr, &us_submsdu_len);
        if (hmac_rx_amsdu_check_frame(pst_msdu_state, us_submsdu_len) != OAL_SUCC) {
            oam_stat_vap_incr(0, rx_no_buff_dropped, 1);
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            return OAL_FAIL;
        }
        mac_get_submsdu_pad_len(MAC_SUBMSDU_HEADER_LEN + us_submsdu_len, &uc_submsdu_pad_len);
        oal_set_mac_addr(pst_msdu->auc_sa, (puc_submsdu_hdr + MAC_SUBMSDU_SA_OFFSET));
        oal_set_mac_addr(pst_msdu->auc_da, (puc_submsdu_hdr + MAC_SUBMSDU_DA_OFFSET));

        if (hmac_rx_amsdu_is_first_sub_msdu_valid(pst_msdu_state, pst_msdu->auc_da, WLAN_MAC_ADDR_LEN) != OAL_SUCC) {
            *pen_proc_state = MAC_PROC_ERROR;
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            oam_warning_log0(0, OAM_SF_RX,
                "{hmac_rx_parse_amsdu::first sub-msdu DA is snap llc header!.}");
            return OAL_FAIL;
        }
        pst_msdu_state->is_first_buffer = OAL_FALSE;

        /* 针对当前的netbuf，申请新的subnetbuf，并设置对应的netbuf的信息，赋值给对应的msdu */
        pst_msdu->pst_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF,
            (MAC_SUBMSDU_HEADER_LEN + us_submsdu_len + uc_submsdu_pad_len), OAL_NETBUF_PRIORITY_MID);
        if (pst_msdu->pst_netbuf == OAL_PTR_NULL) {
            oam_error_log4(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::netbuf null,submsdu_len[%d],submsdu_pad_len[%d],\
                procd_netbuf_nums[%d], procd_msdu_nums_in_mpdu[%d].}", us_submsdu_len, uc_submsdu_pad_len,
                pst_msdu_state->uc_procd_netbuf_nums, pst_msdu_state->uc_procd_msdu_nums_in_mpdu);
            oam_error_log4(0, OAM_SF_RX, "{submsdu_offset[%d]msdu_nums_in_netbuf[%d], procd_msdu_in_netbuf[%d],\
                netbuf_nums_in_mpdu[%d]}", pst_msdu_state->us_submsdu_offset, pst_msdu_state->uc_msdu_nums_in_netbuf,
                pst_msdu_state->uc_procd_msdu_in_netbuf, pst_msdu_state->uc_netbuf_nums_in_mpdu);

            oam_stat_vap_incr(0, rx_no_buff_dropped, 1);
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            return OAL_FAIL;
        }

        oal_mem_netbuf_trace(pst_msdu->pst_netbuf, OAL_TRUE);

        /* 针对每一个子msdu，修改netbuf的end、data、tail、len指针 */
        oal_netbuf_put(pst_msdu->pst_netbuf, us_submsdu_len + HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
        oal_netbuf_pull(pst_msdu->pst_netbuf, HMAC_RX_DATA_ETHER_OFFSET_LENGTH);
        if (memcpy_s(pst_msdu->pst_netbuf->data, us_submsdu_len + uc_submsdu_pad_len,
            (puc_submsdu_hdr + MAC_SUBMSDU_HEADER_LEN), us_submsdu_len) != EOK) {
            OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::memcpy fail.}");
        }

        b_need_free_netbuf = OAL_TRUE;
    }

    /* 增加当前已处理的msdu的个数 */
    pst_msdu_state->uc_procd_msdu_in_netbuf++;

    /* 获取当前的netbuf中的下一个msdu进行处理 */
    if (pst_msdu_state->uc_procd_msdu_in_netbuf < pst_msdu_state->uc_msdu_nums_in_netbuf) {
        pst_msdu_state->us_submsdu_offset += us_submsdu_len + uc_submsdu_pad_len + MAC_SUBMSDU_HEADER_LEN;
    } else if (pst_msdu_state->uc_procd_msdu_in_netbuf == pst_msdu_state->uc_msdu_nums_in_netbuf) {
        pst_msdu_state->uc_procd_netbuf_nums++;

        pst_netbuf_prev = pst_msdu_state->pst_curr_netbuf;

        /* 获取该MPDU对应的下一个netbuf的内容 */
        if (pst_msdu_state->uc_procd_netbuf_nums < pst_msdu_state->uc_netbuf_nums_in_mpdu) {
            pst_msdu_state->pst_curr_netbuf = oal_netbuf_next(pst_msdu_state->pst_curr_netbuf);
            pst_msdu_state->puc_curr_netbuf_data = oal_netbuf_data(pst_msdu_state->pst_curr_netbuf);

            pst_rx_ctrl = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_msdu_state->pst_curr_netbuf);

            pst_msdu_state->uc_msdu_nums_in_netbuf = pst_rx_ctrl->uc_msdu_in_buffer;
            pst_msdu_state->us_submsdu_offset = 0;
            pst_msdu_state->uc_procd_msdu_in_netbuf = 0;

            /* amsdu 第二个netbuf len是0, 先put到最大size */
            oal_netbuf_put(pst_msdu_state->pst_curr_netbuf, WLAN_MEM_NETBUF_SIZE2);
        } else if (pst_msdu_state->uc_procd_netbuf_nums == pst_msdu_state->uc_netbuf_nums_in_mpdu) {
            *pen_proc_state = MAC_PROC_LAST_MSDU;
            if (b_need_free_netbuf) {
                oal_netbuf_free(pst_netbuf_prev);
            }
            return OAL_SUCC;
        } else {
            *pen_proc_state = MAC_PROC_ERROR;
            oam_warning_log0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::proc is err for proc_nums>netbuf_nums_in_mpdul.}");
            hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
            return OAL_FAIL;
        }
        if (b_need_free_netbuf) {
            oal_netbuf_free(pst_netbuf_prev);
        }
    } else {
        *pen_proc_state = MAC_PROC_ERROR;
        oam_warning_log0(0, OAM_SF_RX, "{hmac_rx_parse_amsdu::pen_proc is err for proc_nums>netbuf_nums_in_mpdul.}");
        hmac_rx_free_amsdu_netbuf(pst_msdu_state->pst_curr_netbuf);
        return OAL_FAIL;
    }

    *pen_proc_state = MAC_PROC_MORE_MSDU;

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_rx_prepare_msdu_list_to_wlan(
    hmac_vap_stru *pst_vap, oal_netbuf_head_stru *pst_netbuf_header, oal_netbuf_stru *pst_netbuf,
    mac_ieee80211_frame_stru *pst_frame_hdr)
{
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;                     /* 指向MPDU控制块信息的指针 */
    dmac_msdu_stru st_msdu;                                           /* 保存解析出来的每一个MSDU */
    mac_msdu_proc_status_enum_uint8 en_process_state = MAC_PROC_BUTT; /* 解析AMSDU的状态 */
    dmac_msdu_proc_state_stru st_msdu_state = { 0 };                  /* 记录MPDU的处理信息 */
    oal_uint8 *puc_addr = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    oal_uint8 auc_sa[WLAN_MAC_ADDR_LEN];
    oal_uint8 auc_da[WLAN_MAC_ADDR_LEN];
#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    mac_ether_header_stru *pst_ether_hdr;
#endif
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
    oal_uint8 *puc_payload = OAL_PTR_NULL;

    if (oal_unlikely(pst_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_prepare_msdu_list_to_wlan::pst_netbuf null.}");
        return;
    }

    /* 解析MPDU-->MSDU */ /* 将MSDU组成netbuf链 */
    oal_mem_netbuf_trace(pst_netbuf, OAL_TRUE);

    /* 获取该MPDU的控制信息 */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    memset_s(&st_msdu, OAL_SIZEOF(dmac_msdu_stru), 0, OAL_SIZEOF(dmac_msdu_stru));

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_rx_ctrl->st_rx_info.us_ta_user_idx);
    if (oal_unlikely(pst_hmac_user == OAL_PTR_NULL)) {
        if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
            /* 打印此net buf相关信息 */
            puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);
            oam_report_80211_frame(BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN,
                                   (oal_uint8 *)(pst_frame_hdr),
                                   MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_rx_ctrl->st_rx_info)),
                                   puc_payload,
                                   pst_rx_ctrl->st_rx_info.us_frame_len,
                                   OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        }
        return;
    }
    /* 情况一:不是AMSDU聚合，则该MPDU对应一个MSDU，同时对应一个NETBUF,将MSDU还原
       成以太网格式帧以后直接加入到netbuf链表最后
 */
    if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
        pst_netbuf = hmac_defrag_process(pst_hmac_user, pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            return;
        }

        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

        /* 从MAC头中获取源地址和目的地址 */
        mac_rx_get_sa(pst_frame_hdr, &puc_addr);
        oal_set_mac_addr(auc_sa, puc_addr);

        mac_rx_get_da(pst_frame_hdr, &puc_addr);
        oal_set_mac_addr(auc_da, puc_addr);

        /* 将netbuf的data指针指向mac frame的payload处，也就是指向了8字节的snap头 */
        oal_netbuf_pull(pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);

        /* 将MSDU转化为以太网格式的帧 */
        hmac_rx_frame_80211_to_eth(pst_netbuf, auc_da, auc_sa);

        memset_s(oal_netbuf_cb(pst_netbuf), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
        pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
        if (hmac_11i_ether_type_filter(pst_vap, &pst_hmac_user->st_user_base_info,
                                       pst_ether_hdr->us_ether_type) != OAL_SUCC) { /* 接收安全数据过滤 */
            /* ,us_ether_type为0导致企业级加密失败,添加维测信息看是否报文解析异常 */
            oam_report_eth_frame(auc_da, WLAN_MAC_ADDR_LEN, (oal_uint8 *)pst_ether_hdr,
                (oal_uint16)oal_netbuf_len(pst_netbuf), OAM_OTA_FRAME_DIRECTION_TYPE_RX);

            oal_netbuf_free(pst_netbuf);
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
            return;
        } else
#endif
        {
            /* 将MSDU加入到netbuf链的最后 */
            oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);
        }
    } else { /* 情况二:AMSDU聚合 */
        st_msdu_state.uc_procd_netbuf_nums = 0;
        st_msdu_state.uc_procd_msdu_in_netbuf = 0;

        /* amsdu 最后一个netbuf next指针设为 NULL 出错时方便释放amsdu netbuf */
        hmac_rx_clear_amsdu_last_netbuf_pointer(pst_netbuf, pst_rx_ctrl->st_rx_info.bit_buff_nums);

        do {
            /* 获取下一个要转发的msdu */
            ul_ret = hmac_rx_parse_amsdu(pst_netbuf, &st_msdu, &st_msdu_state, &en_process_state);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                                 "{hmac_rx_prepare_msdu_list_to_wlan::hmac_rx_parse_amsdu failed[%d].}", ul_ret);
                return;
            }

            /* 将MSDU转化为以太网格式的帧 */
            hmac_rx_frame_80211_to_eth(st_msdu.pst_netbuf, st_msdu.auc_da, st_msdu.auc_sa);

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
            pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(st_msdu.pst_netbuf);
            if (hmac_11i_ether_type_filter(pst_vap, &pst_hmac_user->st_user_base_info,
                                           pst_ether_hdr->us_ether_type) != OAL_SUCC) {
                /* 接收安全数据过滤 */
                /* ,us_ether_type为0导致企业级加密失败,添加维测信息看是否报文解析异常 */
                oam_report_eth_frame(st_msdu.auc_da, WLAN_MAC_ADDR_LEN, (oal_uint8 *)pst_ether_hdr,
                    (oal_uint16)oal_netbuf_len(pst_netbuf), OAM_OTA_FRAME_DIRECTION_TYPE_RX);

                oal_netbuf_free(st_msdu.pst_netbuf);
                oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
                continue;
            } else
#endif
            {
                /* 将MSDU加入到netbuf链的最后 */
                oal_netbuf_add_to_list_tail(st_msdu.pst_netbuf, pst_netbuf_header);
            }
        } while (en_process_state != MAC_PROC_LAST_MSDU);
    }

    return;
}

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
static void hmac_parse_ipv4_packet_ipprot_udp(const struct iphdr *iph, oal_uint32 iphdr_len, oal_uint32 netbuf_len)
{
    struct udphdr *uh = NULL;
    if (netbuf_len < iphdr_len + sizeof(struct udphdr)) {
        oam_error_log2(0, OAM_SF_M2U, "{ipv4::netbuf_len[%d], protocol[%d]}", netbuf_len, iph->protocol);
        return;
    }
    uh = (struct udphdr *)((uint8_t *)iph + iphdr_len);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "UDP packet, src port:%d, dst port:%d.\n",
        oal_ntoh_16(uh->source), oal_ntoh_16(uh->dest));
#ifdef CONFIG_HUAWEI_DUBAI
    dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP_UDP_V4", "port", oal_ntoh_16(uh->dest));
#endif
}

static void hmac_parse_ipv4_packet_ipprot_tcp(const struct iphdr *iph, oal_uint32 iphdr_len, oal_uint32 netbuf_len)
{
    struct tcphdr *th = NULL;
    if (netbuf_len < iphdr_len + sizeof(struct tcphdr)) {
        oam_error_log2(0, OAM_SF_M2U, "{ipv4::netbuf_len[%d], protocol[%d]}", netbuf_len, iph->protocol);
        return;
    }
    th = (struct tcphdr *)((uint8_t *)iph + iphdr_len);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "TCP packet, src port:%d, dst port:%d.\n",
        oal_ntoh_16(th->source), oal_ntoh_16(th->dest));
#ifdef CONFIG_HUAWEI_DUBAI
    dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP_TCP_V4", "port", oal_ntoh_16(th->dest));
#endif
}

static void hmac_parse_ipv4_packet_ipprot_icmp(const struct iphdr *iph, oal_uint32 iphdr_len, oal_uint32 netbuf_len)
{
    struct icmphdr *icmph = NULL;
    if (netbuf_len < iphdr_len + sizeof(struct icmphdr)) {
        oam_error_log2(0, OAM_SF_M2U, "{ipv4::netbuf_len[%d], protocol[%d]}", netbuf_len, iph->protocol);
        return;
    }
    icmph = (struct icmphdr *)((uint8_t *)iph + iphdr_len);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ICMP packet, type(%d):%s, code:%d.\n", icmph->type,
        ((icmph->type == 0) ? "ping reply" : ((icmph->type == 8) ? "ping request" : "other icmp pkt")), icmph->code);
#ifdef CONFIG_HUAWEI_DUBAI
    dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP", "protocol", (int32_t)iph->protocol);
#endif
}


OAL_STATIC oal_void hmac_parse_ipv4_packet(oal_void *pst_eth, oal_uint32 netbuf_len)
{
    const struct iphdr *iph = OAL_PTR_NULL;
    oal_uint32 iphdr_len;

    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ipv4 packet.\n");
    /* iphdr: 最小长度为 20 */
    if (netbuf_len < 20) {
        OAM_ERROR_LOG1(0, OAM_SF_M2U, "{hmac_parse_ipv4_packet::netbuf_len[%d]}", netbuf_len);
        return;
    }
    iph = (struct iphdr *)((mac_ether_header_stru *)pst_eth + 1);
    iphdr_len = iph->ihl * 4; /* iph->ihl表示IP层头部包含多少个4字节 */

    OAL_IO_PRINT(WIFI_WAKESRC_TAG "src ip:%d.x.x.%d, dst ip:%d.x.x.%d\n", ipaddr(iph->saddr), ipaddr(iph->daddr));
    if (iph->protocol == IPPROTO_UDP) {
        hmac_parse_ipv4_packet_ipprot_udp(iph, iphdr_len, netbuf_len);
    } else if (iph->protocol == IPPROTO_TCP) {
        hmac_parse_ipv4_packet_ipprot_tcp(iph, iphdr_len, netbuf_len);
    } else if (iph->protocol == IPPROTO_ICMP) {
        hmac_parse_ipv4_packet_ipprot_icmp(iph, iphdr_len, netbuf_len);
    } else if (iph->protocol == IPPROTO_IGMP) {
        OAL_IO_PRINT(WIFI_WAKESRC_TAG "IGMP packet.\n");
#ifdef CONFIG_HUAWEI_DUBAI
        dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP", "protocol", (int32_t)iph->protocol);
#endif
    } else {
        OAL_IO_PRINT(WIFI_WAKESRC_TAG "other IPv4 packet.\n");
#ifdef CONFIG_HUAWEI_DUBAI
        dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP", "protocol", (int32_t)iph->protocol);
#endif
    }

    return;
}


OAL_STATIC oal_void hmac_parse_ipv6_packet(oal_void *pst_eth, oal_uint32 buf_len)
{
    struct ipv6hdr *ipv6h = OAL_PTR_NULL;
    oal_icmp6hdr_stru *icmph = NULL;
    if (buf_len < sizeof(struct ipv6hdr)) {
        oam_error_log2(0, OAM_SF_ANY, "{hmac_parse_ipv6_packet::buf_len[%d], ipv6hdr[%d]}",
            buf_len, sizeof(struct ipv6hdr));
        return;
    }
    buf_len -= sizeof(struct ipv6hdr);

    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ipv6 packet.\n");
    ipv6h = (struct ipv6hdr *)((mac_ether_header_stru *)pst_eth + 1);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "version: %d, payload length: %d, nh->nexthdr: %d. \n", ipv6h->version,
                 oal_ntoh_16(ipv6h->payload_len), ipv6h->nexthdr);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ipv6 src addr:%04x:x:x:x:x:x:x:%04x \n", ipaddr6(ipv6h->saddr));
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ipv6 dst addr:%04x:x:x:x:x:x:x:%04x \n", ipaddr6(ipv6h->daddr));
    if (ipv6h->nexthdr == OAL_IPPROTO_ICMPV6) {
        if (buf_len < sizeof(oal_icmp6hdr_stru)) {
            oam_error_log2(0, OAM_SF_ANY, "{hmac_parse_ipv6_packet::buf_len[%d] icmp6hdr[%d]}",
                buf_len, sizeof(oal_icmp6hdr_stru));
            return;
        }
        icmph = (oal_icmp6hdr_stru *)(ipv6h + 1);
        OAL_IO_PRINT(WIFI_WAKESRC_TAG"ipv6 nd type: %d. \n", icmph->icmp6_type);
    }
#ifdef CONFIG_HUAWEI_DUBAI
    dubai_log_packet_wakeup_stats("DUBAI_TAG_PACKET_WAKEUP", "protocol", IPPROTO_IPV6);
#endif

    return;
}


OAL_STATIC oal_void hmac_parse_arp_packet(oal_void *pst_eth, oal_uint32 buf_len)
{
    struct arphdr *arp = NULL;

    if (buf_len < sizeof(struct arphdr)) {
        OAM_ERROR_LOG1(0, 0, "{hmac_parse_arp_packet::buf_len[%d].}", buf_len);
        return;
    }

    /* 以太网帧格式：跳过 目的地址6 源地址6 帧类型2，到arp头 */
    arp = (struct arphdr *)((mac_ether_header_stru *)pst_eth + 1);
    OAL_IO_PRINT(WIFI_WAKESRC_TAG "ARP packet, hardware type:%d, protocol type:%d, opcode:%d.\n",
                 oal_ntoh_16(arp->ar_hrd), oal_ntoh_16(arp->ar_pro), oal_ntoh_16(arp->ar_op));

    return;
}


OAL_STATIC oal_void hmac_parse_8021x_packet(oal_void *pst_eth, oal_uint32 buf_len)
{
    struct ieee8021x_hdr *hdr = (struct ieee8021x_hdr *)((mac_ether_header_stru *)pst_eth + 1);
    if (buf_len < sizeof(struct ieee8021x_hdr)) {
        OAM_ERROR_LOG1(0, 0, "{hmac_parse_packet::buf_len}", buf_len);
        return;
    }

    OAL_IO_PRINT(WIFI_WAKESRC_TAG "802.1x frame: version:%d, type:%d, length:%d\n", hdr->version, hdr->type,
                 oal_ntoh_16(hdr->length));

    return;
}


oal_void hmac_parse_packet(oal_netbuf_stru *pst_netbuf_eth)
{
    oal_uint16 us_type;
    mac_ether_header_stru *pst_ether_hdr = NULL;
    oal_uint32 buf_len;

    buf_len = oal_netbuf_len(pst_netbuf_eth);
    if (buf_len < sizeof(mac_ether_header_stru)) {
        OAM_ERROR_LOG1(0, 0, "{hmac_parse_packet::buf_len}", buf_len);
        return;
    }
    buf_len -= sizeof(mac_ether_header_stru);

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf_eth);
    if (oal_unlikely(pst_ether_hdr == OAL_PTR_NULL)) {
        OAL_IO_PRINT(WIFI_WAKESRC_TAG "ether header is null.\n");
        return;
    }

    us_type = pst_ether_hdr->us_ether_type;

    if (us_type == OAL_HOST2NET_SHORT(ETHER_TYPE_IP)) {
        hmac_parse_ipv4_packet((oal_void*)pst_ether_hdr, buf_len);
    } else if (us_type == OAL_HOST2NET_SHORT(ETHER_TYPE_IPV6)) {
        hmac_parse_ipv6_packet((oal_void*)pst_ether_hdr, buf_len);
    } else if (us_type == OAL_HOST2NET_SHORT(ETHER_TYPE_ARP)) {
        hmac_parse_arp_packet((oal_void*)pst_ether_hdr, buf_len);
    } else if (us_type == OAL_HOST2NET_SHORT(ETHER_TYPE_PAE)) {
        hmac_parse_8021x_packet((oal_void*)pst_ether_hdr, buf_len);
    } else {
        OAL_IO_PRINT(WIFI_WAKESRC_TAG "receive protocol type:0x%04x\n", oal_ntoh_16(us_type));
    }

    return;
}

#endif


OAL_STATIC oal_void hmac_rx_transmit_msdu_to_lan(hmac_vap_stru *pst_vap, hmac_user_stru *pst_hmac_user,
                                                 dmac_msdu_stru *pst_msdu)
{
    oal_net_device_stru *pst_device = OAL_PTR_NULL;
    oal_netbuf_stru *pst_netbuf;
    mac_ether_header_stru *pst_ether_hdr = OAL_PTR_NULL;

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    oal_uint8 *puc_mac_addr;
#endif
    mac_vap_stru *pst_mac_vap = &(pst_vap->st_vap_base_info);
#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    hmac_user_stru *pst_huser = OAL_PTR_NULL;
    mac_ip_header_stru *pst_ip = OAL_PTR_NULL;
    oal_uint16 us_assoc_id = 0xffff;
#endif

    /* 获取netbuf，该netbuf的data指针已经指向payload处 */
    pst_netbuf = pst_msdu->pst_netbuf;

    oal_netbuf_prev(pst_netbuf) = OAL_PTR_NULL;
    oal_netbuf_next(pst_netbuf) = OAL_PTR_NULL;

    hmac_rx_frame_80211_to_eth(pst_netbuf, pst_msdu->auc_da, pst_msdu->auc_sa);

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);
    if (oal_unlikely(pst_ether_hdr == OAL_PTR_NULL)) {
        oal_netbuf_free(pst_netbuf);
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_transmit_msdu_to_lan::pst_ether_hdr null.}");
        return;
    }

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if (wlan_pm_wkup_src_debug_get() == OAL_TRUE) {
        OAL_IO_PRINT(WIFI_WAKESRC_TAG "rx: hmac_parse_packet_etc!\n");
        hmac_parse_packet(pst_netbuf);
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
    }
#endif

#if defined(_PRE_WLAN_FEATURE_WPA) || defined(_PRE_WLAN_FEATURE_WPA2)
    puc_mac_addr = pst_msdu->auc_ta;
    if (hmac_11i_ether_type_filter(pst_vap, &pst_hmac_user->st_user_base_info, pst_ether_hdr->us_ether_type) !=
        OAL_SUCC) {
        /* 接收安全数据过滤 */
        /* ,us_ether_type为0导致企业级加密失败,添加维测信息看是否报文解析异常 */
        oam_report_eth_frame(puc_mac_addr, WLAN_MAC_ADDR_LEN, (oal_uint8 *)pst_ether_hdr,
            (oal_uint16)oal_netbuf_len(pst_netbuf), OAM_OTA_FRAME_DIRECTION_TYPE_RX);

        oal_netbuf_free(pst_netbuf);
        oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_portvalid_check_fail_dropped, 1);
        return;
    }
#endif

    /* 获取net device hmac创建的时候，需要记录netdevice指针 */
    pst_device = pst_vap->pst_net_device;

    /* 对protocol模式赋值 */
    oal_netbuf_protocol(pst_netbuf) = oal_eth_type_trans(pst_netbuf, pst_device);

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    if (ether_is_multicast(pst_msdu->auc_da) == OAL_FALSE) {
        oal_atomic_inc(&(pst_hmac_user->st_hmac_user_btcoex.st_hmac_btcoex_arp_req_process.ul_rx_unicast_pkt_to_lan));
    }
#endif

    /* 信息统计与帧上报分离 */
    /* 增加统计信息 */
    hmac_vap_dft_stats_pkt_incr(pst_vap->st_query_stats.ul_rx_pkt_to_lan, 1);
    hmac_vap_dft_stats_pkt_incr(pst_vap->st_query_stats.ul_rx_bytes_to_lan, oal_netbuf_len(pst_netbuf));
    oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_pkt_to_lan, 1); /* 增加发往LAN的帧的数目 */
    oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_bytes_to_lan,
                      oal_netbuf_len(pst_netbuf)); /* 增加发送LAN的字节数 */

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) && (pst_vap->uc_edca_opt_flag_ap == OAL_TRUE)) {
        /*lint -e778*/
        if (OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == pst_ether_hdr->us_ether_type) {
            if (mac_vap_find_user_by_macaddr(pst_mac_vap, pst_ether_hdr->auc_ether_shost,
                WLAN_MAC_ADDR_LEN, &us_assoc_id) != OAL_SUCC) {
                oam_warning_log4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2U,
                                 "{hmac_rx_transmit_msdu_to_lan::find_user_by_macaddr[%02x:XX:XX:%02x:%02x:%02x]fail}",
                                 (oal_uint32)(pst_ether_hdr->auc_ether_shost[0]),
                                 (oal_uint32)(pst_ether_hdr->auc_ether_shost[3]), /* auc_ether_shost第3byte打印输出 */
                                 (oal_uint32)(pst_ether_hdr->auc_ether_shost[4]), /* auc_ether_shost第4byte打印输出 */
                                 (oal_uint32)(pst_ether_hdr->auc_ether_shost[5])); /* auc_ether_shost第5byte打印输出 */
                oal_netbuf_free(pst_netbuf);
                return;
            }
            pst_huser = (hmac_user_stru *)mac_res_get_hmac_user(us_assoc_id);
            if (pst_huser == OAL_PTR_NULL) {
                OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                               "{hmac_rx_transmit_msdu_to_lan::mac_res_get_hmac_user fail. assoc_id: %u}", us_assoc_id);
                oal_netbuf_free(pst_netbuf);
                return;
            }

            pst_ip = (mac_ip_header_stru *)(pst_ether_hdr + 1);

            /* mips优化:解决开启业务统计性能差10M问题 */
            if (((pst_ip->uc_protocol == MAC_UDP_PROTOCAL) &&
                 /* 接收UDP data数小于平均每毫秒报文个数加10 */
                 (pst_huser->aaul_txrx_data_stat[WLAN_WME_AC_BE][WLAN_RX_UDP_DATA] < (HMAC_EDCA_OPT_PKT_NUM + 10))) ||
                ((pst_ip->uc_protocol == MAC_TCP_PROTOCAL) &&
                 /* 接收TCP data数小于平均每毫秒报文个数加10 */
                 (pst_huser->aaul_txrx_data_stat[WLAN_WME_AC_BE][WLAN_RX_TCP_DATA] < (HMAC_EDCA_OPT_PKT_NUM + 10)))) {
                hmac_edca_opt_rx_pkts_stat(us_assoc_id, WLAN_TIDNO_BEST_EFFORT, pst_ip);
            }
        }
        /*lint +e778*/
    }
#endif

    oal_mem_netbuf_trace(pst_netbuf, OAL_TRUE);
    memset_s(oal_netbuf_cb(pst_netbuf), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

    /* 将skb转发给桥 */
    if (hmac_get_rxthread_enable() == OAL_TRUE) {
        hmac_rxdata_netbuf_enqueue(pst_netbuf);

        hmac_rxdata_sched();
    } else {
        oal_netif_rx_ni(pst_netbuf);
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
    /* 4.11以上内核版本net_device结构体中没有last_rx字段 */
#else
    /* 置位net_dev->jiffies变量 */
    oal_netdevice_last_rx(pst_device) = OAL_TIME_JIFFY;
#endif
}
void hmac_get_user_fail_in_classify(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf,
    mac_ieee80211_frame_stru *pst_frame_hdr, hmac_rx_ctl_stru *pst_rx_ctrl)
{
#ifdef _PRE_WLAN_DFT_DUMP_FRAME
    oal_uint8 *puc_payload;
#endif
    OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                   "{hmac_rx_lan_frame_classify::pst_hmac_user null, user_idx=%d.}",
                   pst_rx_ctrl->st_rx_info.us_ta_user_idx);

    /* 打印此net buf相关信息 */
    oam_error_log4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                   "{hmac_rx_lan_frame_classify::info in cb, vap id=%d mac_hdr_len=%d,\
                   us_frame_len=%d mac_hdr_start_addr=0x%08x.}",
                   pst_rx_ctrl->st_rx_info.bit_vap_id,
                   pst_rx_ctrl->st_rx_info.uc_mac_header_len,
                   pst_rx_ctrl->st_rx_info.us_frame_len,
                   (uintptr_t)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr);
    oam_error_log2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                   "{hmac_rx_lan_frame_classify::net_buf ptr addr=0x%08x, cb ptr addr=0x%08x.}",
                   (uintptr_t)pst_netbuf, (uintptr_t)pst_rx_ctrl);
#ifdef _PRE_WLAN_DFT_DUMP_FRAME
    puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);
    oam_report_80211_frame(BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN,
                           (oal_uint8 *)(pst_frame_hdr),
                           MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_rx_ctrl->st_rx_info)),
                           puc_payload,
                           pst_rx_ctrl->st_rx_info.us_frame_len,
                           OAM_OTA_FRAME_DIRECTION_TYPE_RX);
#endif
}
oal_uint32 hmac_wapi_rx_handle(hmac_vap_stru *pst_vap, oal_netbuf_stru **ppst_netbuf,
    mac_ieee80211_frame_stru *pst_frame_hdr, hmac_rx_ctl_stru **ppst_rx_ctrl, hmac_user_stru *pst_hmac_user)
{
#ifdef _PRE_WLAN_FEATURE_WAPI
    hmac_wapi_stru *pst_wapi = OAL_PTR_NULL;
    oal_bool_enum_uint8 en_is_mcast;

    /*lint -e730*/
    en_is_mcast = ether_is_multicast(pst_frame_hdr->auc_address1);

    pst_wapi = hmac_user_get_wapi_ptr(&pst_vap->st_vap_base_info,
                                      !en_is_mcast,
                                      pst_hmac_user->st_user_base_info.us_assoc_id);
    /*lint +e730*/
    if (pst_wapi == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_WPA, "{hmac_rx_lan_frame_classify:: get pst_wapi Err!.}");
        hmac_user_stats_pkt_incr(pst_hmac_user->ul_rx_pkt_drop, 1);
        return OAL_FAIL;
    }

    if ((wapi_is_port_valid(pst_wapi) == OAL_TRUE) && (pst_wapi->wapi_netbuff_rxhandle != OAL_PTR_NULL)) {
        (*ppst_netbuf) = pst_wapi->wapi_netbuff_rxhandle(pst_wapi, (*ppst_netbuf));
        if ((*ppst_netbuf) == OAL_PTR_NULL) {
            oam_warning_log0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                             "{hmac_rx_lan_frame_classify:: wapi decrypt FAIL!}");
            hmac_user_stats_pkt_incr(pst_hmac_user->ul_rx_pkt_drop, 1);
            return OAL_FAIL;
        }

        /* 重新获取该MPDU的控制信息 */
        (*ppst_rx_ctrl) = (hmac_rx_ctl_stru *)oal_netbuf_cb((*ppst_netbuf));
    }
#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */
    return OAL_SUCC;
}
void hmac_process_no_amsdu(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf, hmac_user_stru *pst_hmac_user,
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_netbuf_head_stru *pst_w2w_netbuf_hdr,
#endif
    dmac_msdu_stru *pst_msdu)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint16 us_user_dix = MAC_INVALID_USER_ID;
#endif
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
    oal_uint8 uc_datatype;
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;
    oal_uint8 *puc_addr = OAL_PTR_NULL;

    /* 重新获取该MPDU的控制信息 */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    /*  去分片有可能释放netbuf，需要重新获取frame hdr */
    pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

    /* 打印出关键帧(dhcp)信息 */
    uc_datatype = pst_rx_ctrl->st_rx_info.bit_data_frame_type;
    if ((OAL_IS_VIP_FRAME(uc_datatype)) && (uc_datatype != MAC_DATA_ARP_REQ)) {
        oam_warning_log4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                         "{hmac_rx_lan_frame_classify::user[%d],\
                         datatype==%u, len==%u, rx_drop_cnt==%u}[1:dhcp 2:arp_req 3:arp_rsp 4:eapol]",
                         pst_rx_ctrl->st_rx_info.us_ta_user_idx,
                         uc_datatype,
                         pst_rx_ctrl->st_rx_info.us_frame_len,
                         pst_hmac_user->ul_rx_pkt_drop);
    }

    /* 对当前的msdu进行赋值 */
    pst_msdu->pst_netbuf = pst_netbuf;

    /* 获取源地址和目的地址 */
    mac_rx_get_sa(pst_frame_hdr, &puc_addr);
    oal_set_mac_addr(pst_msdu->auc_sa, puc_addr);

    mac_rx_get_da(pst_frame_hdr, &puc_addr);
    oal_set_mac_addr(pst_msdu->auc_da, puc_addr);

    /* 将netbuf的data指针指向mac frame的payload处 */
    oal_netbuf_pull(pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);

    /*  当msdu.da在vap下,进行wlan_to_wlan转发,防止to_lan到内核不进行转发 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if ((pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (mac_vap_find_user_by_macaddr(&pst_vap->st_vap_base_info, pst_msdu->auc_da,
        WLAN_MAC_ADDR_LEN, &us_user_dix) == OAL_SUCC)) {
        /* 将MSDU转化为以太网格式的帧 */
        hmac_rx_frame_80211_to_eth(pst_msdu->pst_netbuf, pst_msdu->auc_da, pst_msdu->auc_sa);
        /* 将MSDU加入到netbuf链的最后 */
        oal_netbuf_add_to_list_tail(pst_msdu->pst_netbuf, pst_w2w_netbuf_hdr);
    } else
#endif
    {
        /* 将MSDU转发到LAN */
        hmac_rx_transmit_msdu_to_lan(pst_vap, pst_hmac_user, pst_msdu);
    }
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
void hmac_process_wlan2wlan(hmac_vap_stru *pst_vap, frw_event_hdr_stru *pst_event_hdr,
    oal_netbuf_head_stru *pst_w2w_netbuf_hdr)
{
    if (pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /*  当msdu.da在vap下,进行wlan_to_wlan转发,防止to_lan到内核不进行转发 */
        oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_TO_LAN);
        /* 将MSDU链表交给发送流程处理 */
        if ((oal_netbuf_list_empty(pst_w2w_netbuf_hdr) == OAL_FALSE) &&
            (oal_netbuf_tail(pst_w2w_netbuf_hdr) != OAL_PTR_NULL) &&
            (oal_netbuf_peek(pst_w2w_netbuf_hdr) != OAL_PTR_NULL)) {
            pst_event_hdr->uc_chip_id = pst_vap->st_vap_base_info.uc_chip_id;
            pst_event_hdr->uc_device_id = pst_vap->st_vap_base_info.uc_device_id;
            pst_event_hdr->uc_vap_id = pst_vap->st_vap_base_info.uc_vap_id;

            oal_netbuf_next((oal_netbuf_tail(pst_w2w_netbuf_hdr))) = OAL_PTR_NULL;
            oal_netbuf_prev((oal_netbuf_peek(pst_w2w_netbuf_hdr))) = OAL_PTR_NULL;

            hmac_rx_transmit_to_wlan(pst_event_hdr, pst_w2w_netbuf_hdr);
        }
        oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_END);
    }
}
#endif

oal_void hmac_rx_lan_frame_classify(hmac_vap_stru *pst_vap,
                                    oal_netbuf_stru *pst_netbuf,
                                    mac_ieee80211_frame_stru *pst_frame_hdr)
{
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;                     /* 指向MPDU控制块信息的指针 */
    dmac_msdu_stru st_msdu;                                           /* 保存解析出来的每一个MSDU */
    mac_msdu_proc_status_enum_uint8 en_process_state = MAC_PROC_BUTT; /* 解析AMSDU的状态 */
    dmac_msdu_proc_state_stru st_msdu_state = { 0 };                  /* 记录MPDU的处理信息 */
    oal_uint8 *puc_addr = OAL_PTR_NULL;
    oal_uint32 ul_ret;
    hmac_user_stru *pst_hmac_user = OAL_PTR_NULL;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    frw_event_hdr_stru st_event_hdr;
    oal_netbuf_head_stru st_w2w_netbuf_hdr;
    oal_uint16 us_user_dix = MAC_INVALID_USER_ID;
#endif

    if (oal_unlikely((pst_vap == OAL_PTR_NULL) || (pst_netbuf == OAL_PTR_NULL) || (pst_frame_hdr == OAL_PTR_NULL))) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_lan_frame_classify::params null.}");
        return;
    }

    memset_s(&st_msdu, OAL_SIZEOF(dmac_msdu_stru), 0, OAL_SIZEOF(dmac_msdu_stru));

    mac_get_transmit_addr(pst_frame_hdr, &puc_addr);

    oal_set_mac_addr(st_msdu.auc_ta, puc_addr);

    /* 获取该MPDU的控制信息 */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_rx_ctrl->st_rx_info.us_ta_user_idx);
    if (oal_unlikely(pst_hmac_user == OAL_PTR_NULL)) {
        hmac_get_user_fail_in_classify(pst_vap, pst_netbuf, pst_frame_hdr, pst_rx_ctrl);
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* offload 下amsdu帧可能分成多个seq相同的netbuf上报 只有在last amsdu buffer才能置位
    否则容易导致重排序缓冲区在弱信号下强制移窗后收到ba start之前seq的amsdu帧丢帧
 */
    if ((pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) ||
        (pst_rx_ctrl->st_rx_info.bit_is_last_buffer == OAL_TRUE)) {
        hmac_ba_update_rx_bitmap(pst_hmac_user, pst_frame_hdr);
    }

    oal_netbuf_list_head_init(&st_w2w_netbuf_hdr);
#else
    hmac_ba_update_rx_bitmap(pst_hmac_user, pst_frame_hdr);
#endif

#ifdef _PRE_WLAN_FEATURE_SNIFFER
    proc_sniffer_write_file(NULL, 0, (oal_uint8 *)oal_netbuf_payload(pst_netbuf), pst_rx_ctrl->st_rx_info.us_frame_len,
                            0);
#endif

    /* 情况一:不是AMSDU聚合，则该MPDU对应一个MSDU，同时对应一个NETBUF */
    if (pst_rx_ctrl->st_rx_info.bit_amsdu_enable == OAL_FALSE) {
        if (hmac_wapi_rx_handle(pst_vap, &pst_netbuf, pst_frame_hdr, &pst_rx_ctrl, pst_hmac_user) != OAL_SUCC) {
            return;
        }
        pst_netbuf = hmac_defrag_process(pst_hmac_user, pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_header_len);
        if (pst_netbuf == OAL_PTR_NULL) {
            return;
        }

        hmac_process_no_amsdu(pst_vap, pst_netbuf, pst_hmac_user,
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
                              &st_w2w_netbuf_hdr,
#endif
                              &st_msdu);
    } else { /* 情况二:AMSDU聚合 */
        st_msdu_state.uc_procd_netbuf_nums = 0;
        st_msdu_state.uc_procd_msdu_in_netbuf = 0;

        /* amsdu 最后一个netbuf next指针设为 NULL 出错时方便释放amsdu netbuf */
        hmac_rx_clear_amsdu_last_netbuf_pointer(pst_netbuf, pst_rx_ctrl->st_rx_info.bit_buff_nums);

        hmac_wifi_statistic_rx_amsdu(oal_netbuf_len(pst_netbuf) - pst_rx_ctrl->st_rx_info.uc_mac_header_len);

        do {
            /* 获取下一个要转发的msdu */
            ul_ret = hmac_rx_parse_amsdu(pst_netbuf, &st_msdu, &st_msdu_state, &en_process_state);
            if (ul_ret != OAL_SUCC) {
                OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                                 "{hmac_rx_lan_frame_classify::hmac_rx_parse_amsdu failed[%d].}", ul_ret);
                return;
            }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            if ((pst_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
                (mac_vap_find_user_by_macaddr(&pst_vap->st_vap_base_info, st_msdu.auc_da,
                WLAN_MAC_ADDR_LEN, &us_user_dix) == OAL_SUCC)) {
                /*  当msdu.da在vap下,进行wlan_to_wlan转发,防止to_lan到内核不进行转发 */
                /* 将MSDU转化为以太网格式的帧 */
                hmac_rx_frame_80211_to_eth(st_msdu.pst_netbuf, st_msdu.auc_da, st_msdu.auc_sa);
                /* 将MSDU加入到netbuf链的最后 */
                oal_netbuf_add_to_list_tail(st_msdu.pst_netbuf, &st_w2w_netbuf_hdr);
            } else
#endif
            {
                /* 将每一个MSDU转发到LAN */
                hmac_rx_transmit_msdu_to_lan(pst_vap, pst_hmac_user, &st_msdu);
            }
        } while (en_process_state != MAC_PROC_LAST_MSDU);
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hmac_process_wlan2wlan(pst_vap, &st_event_hdr, &st_w2w_netbuf_hdr);
#endif
}


oal_uint32 hmac_rx_copy_netbuff(oal_netbuf_stru **ppst_dest_netbuf, oal_netbuf_stru *pst_src_netbuf,
                                oal_uint8 uc_vap_id, mac_ieee80211_frame_stru **ppul_mac_hdr_start_addr)
{
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
    oal_int32 l_ret;

    if (pst_src_netbuf == OAL_PTR_NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    *ppst_dest_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (oal_unlikely(*ppst_dest_netbuf == OAL_PTR_NULL)) {
        oam_warning_log0(uc_vap_id, OAM_SF_RX, "{hmac_rx_copy_netbuff::pst_netbuf_copy null.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 信息复制 */
    l_ret = memcpy_s(oal_netbuf_cb(*ppst_dest_netbuf), OAL_SIZEOF(hmac_rx_ctl_stru),
                     oal_netbuf_cb(pst_src_netbuf), OAL_SIZEOF(hmac_rx_ctl_stru));  // modify src bug
    l_ret += memcpy_s(oal_netbuf_data(*ppst_dest_netbuf), oal_netbuf_len(pst_src_netbuf),
                      oal_netbuf_data(pst_src_netbuf), oal_netbuf_len(pst_src_netbuf));
    if (l_ret != EOK) {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_RX, "hmac_rx_copy_netbuff::memcpy fail!");
        oal_netbuf_free(*ppst_dest_netbuf);
        return OAL_FAIL;
    }

    /* 设置netbuf长度、TAIL指针 */
    oal_netbuf_put(*ppst_dest_netbuf, oal_netbuf_get_len(pst_src_netbuf));

    /* 调整MAC帧头的指针copy后，对应的mac header的头已经发生变化) */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(*ppst_dest_netbuf);
    pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr = (oal_uint32 *)oal_netbuf_data(*ppst_dest_netbuf);
    *ppul_mac_hdr_start_addr = (mac_ieee80211_frame_stru *)oal_netbuf_data(*ppst_dest_netbuf);

    return OAL_SUCC;
}


oal_void hmac_rx_process_data_filter(oal_netbuf_head_stru *pst_netbuf_header, oal_netbuf_stru *pst_temp_netbuf,
                                     oal_uint16 us_netbuf_num)
{
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_AMPDU
    hmac_user_stru *pst_hmac_user;
    mac_ieee80211_frame_stru *pst_frame_hdr;
#endif
    oal_uint8 uc_buf_nums;
    mac_vap_stru *pst_vap = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_AMPDU
    oal_uint32 ul_ret = OAL_SUCC;
#endif
    oal_bool_enum_uint8 en_is_ba_buf;
    oal_uint8 uc_netbuf_num;

    while (us_netbuf_num != 0) {
        en_is_ba_buf = OAL_FALSE;
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_filter::us_netbuf_num = %d}", us_netbuf_num);
            break;
        }

        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

        uc_buf_nums = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        /* 获取下一个要处理的MPDU */
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);
        us_netbuf_num = oal_sub(us_netbuf_num, uc_buf_nums);

        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            oam_warning_log0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX, "{hmac_rx_process_data_filter::vap null.}");
            continue;
        }

        /*  start, modified: 2015/04/08 */
        /* 双芯片下，0和1都是配置vap id，因此这里需要采用业务vap 其实id和整板最大vap mac num值来做判断 */
        if ((pst_vap->uc_vap_id < WLAN_SERVICE_VAP_START_ID_PER_BOARD) ||
            (pst_vap->uc_vap_id > WLAN_VAP_SUPPORT_MAX_NUM_LIMIT)) {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_filter::Invalid vap_id[%u]}", pst_vap->uc_vap_id);
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            continue;
        }
        /*  end */
#ifdef _PRE_WLAN_FEATURE_AMPDU
        pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctrl->st_rx_info)));
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
        if (pst_hmac_user == OAL_PTR_NULL) {
            OAM_WARNING_LOG1(0, OAM_SF_RX,
                             "{hmac_rx_process_data_filter::pst_hmac_user is null.index[%d]}",
                             MAC_GET_RX_CB_TA_USER_IDX(&(pst_rx_ctrl->st_rx_info)));
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            continue;
        }

        ul_ret = hmac_ba_filter_serv(pst_vap, pst_hmac_user, pst_rx_ctrl, pst_frame_hdr,
                                     pst_netbuf_header, &en_is_ba_buf);
        if (ul_ret != OAL_SUCC) {
            hmac_rx_free_netbuf_list(pst_netbuf_header, uc_buf_nums);
            continue;
        }
#endif

        if (en_is_ba_buf == OAL_TRUE) {
            continue;
        }

        /* 如果不buff进reorder队列，则重新挂到链表尾，保序 */
        for (uc_netbuf_num = 0; uc_netbuf_num < uc_buf_nums; uc_netbuf_num++) {
            pst_netbuf = oal_netbuf_delist_nolock(pst_netbuf_header);
            if (oal_likely(pst_netbuf != OAL_PTR_NULL)) {
                oal_netbuf_list_tail_nolock(pst_netbuf_header, pst_netbuf);
            } else {
                oam_warning_log0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX, "{hmac_rx_process_data_filter:no buf}");
            }
        }
    }
}

#ifdef _PRE_WLAN_TCP_OPT
OAL_STATIC oal_bool_enum_uint8 hmac_transfer_rx_handler(hmac_device_stru *pst_hmac_device, hmac_vap_stru *hmac_vap,
                                                        oal_netbuf_stru *netbuf)
{
#ifndef WIN32
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL; /* 指向MPDU控制块信息的指针 */
    mac_llc_snap_stru* pst_mac_llc_snap_netbuf = NULL;
    oal_uint32 buf_len = oal_netbuf_len(netbuf);

    if (pst_hmac_device->sys_tcp_rx_ack_opt_enable == OAL_TRUE) {
        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
        if (buf_len < pst_rx_ctrl->st_rx_info.uc_mac_header_len) {
            OAM_ERROR_LOG1(0, OAM_SF_TX, "{hmac_transfer_rx_tcp_ack_handler::buf_len[%d].}", buf_len);
            return OAL_FALSE;
        }
        buf_len -= pst_rx_ctrl->st_rx_info.uc_mac_header_len;
        pst_mac_llc_snap_netbuf = (mac_llc_snap_stru*)(netbuf->data + pst_rx_ctrl->st_rx_info.uc_mac_header_len);

        if (hmac_judge_rx_netbuf_classify(pst_mac_llc_snap_netbuf, buf_len) == OAL_TRUE) {
            oal_spin_lock_bh(&hmac_vap->ast_hmac_tcp_ack[HCC_RX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            oal_netbuf_list_tail_nolock(&hmac_vap->ast_hmac_tcp_ack[HCC_RX].data_queue[HMAC_TCP_ACK_QUEUE], netbuf);
            oal_spin_unlock_bh(&hmac_vap->ast_hmac_tcp_ack[HCC_RX].data_queue_lock[HMAC_TCP_ACK_QUEUE]);
            hmac_sched_transfer();
            return OAL_TRUE;
        }
    }
#endif
    return OAL_FALSE;
}

#endif


oal_uint32 hmac_rx_check_data(hmac_device_stru *pst_hmac_device, oal_uint8 *puc_payload, oal_uint16 us_data_len)
{
    oal_uint16 us_index = 0;
    oal_uint8 uc_type;
    hmac_packet_check_rx_info_stru *pst_pkt_check_rx_info = hmac_get_pkt_check_rx_info_addr();

    uc_type = puc_payload[us_index];
    if ((uc_type != WLAN_PACKET_CHECK_DATA_TYPE0) &&
        (uc_type != WLAN_PACKET_CHECK_DATA_TYPE1) &&
        (uc_type != WLAN_PACKET_CHECK_DATA_TYPE2) &&
        (uc_type != WLAN_PACKET_CHECK_DATA_TYPE3)) {
        return OAL_FAIL;
    }

    for (; us_index < us_data_len; us_index++) {
        if (puc_payload[us_index] != uc_type) {
            break;
        }
    }
    if (us_index != us_data_len) {
        oam_error_log2(0, OAM_SF_RX, "{hmac_rx_check_data::index = %d, len = %d.}", us_index, us_data_len);
        return OAL_FAIL;
    }
    switch (uc_type) {
        case WLAN_PACKET_CHECK_DATA_TYPE0:
            pst_pkt_check_rx_info->uc_pkt_check_num0++;
            break;
        case WLAN_PACKET_CHECK_DATA_TYPE1:
            pst_pkt_check_rx_info->uc_pkt_check_num1++;
            break;
        case WLAN_PACKET_CHECK_DATA_TYPE2:
            pst_pkt_check_rx_info->uc_pkt_check_num2++;
            break;
        case WLAN_PACKET_CHECK_DATA_TYPE3:
            pst_pkt_check_rx_info->uc_pkt_check_num3++;
            break;
        default:
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_check_data:check data type error![%d]}", uc_type);
            return OAL_FAIL;
    }
    return OAL_SUCC;
}


oal_uint32 hmac_rx_check_snap(oal_uint8 *puc_payload)
{
    mac_llc_snap_stru *pst_snap = (mac_llc_snap_stru *)puc_payload;

    if ((pst_snap->uc_llc_dsap != SNAP_LLC_LSAP) ||
        (pst_snap->uc_llc_ssap != SNAP_LLC_LSAP) ||
        (pst_snap->uc_control != LLC_UI)) {
        return OAL_FAIL;
    }

    if ((pst_snap->auc_org_code[0] != SNAP_RFC1042_ORGCODE_0) ||
        (pst_snap->auc_org_code[1] != SNAP_RFC1042_ORGCODE_1) ||
        (pst_snap->auc_org_code[2] != SNAP_RFC1042_ORGCODE_2)) { /* 判断auc_org_code0、1、2byte是否非零 */
        return OAL_FAIL;
    }

    if (pst_snap->us_ether_type != oal_ntoh_16(ETHER_TYPE_PACKET_CHECK)) {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

oal_void hmac_packet_check_rsp(hmac_vap_stru *pst_vap, hmac_device_stru *pst_hmac_device,
                               oal_bool_enum_uint8 check_result)
{
    /* 唤醒wal_hipriv_packet_check,反馈检查结果 */
    pst_hmac_device->st_packet_check.en_pkt_check_completed = OAL_TRUE;
    pst_hmac_device->st_packet_check.en_pkt_check_result = check_result;
    oal_wait_queue_wake_up_interrupt(&(pst_hmac_device->st_packet_check.st_check_wait_q));
}


oal_bool_enum_uint8 hmac_rx_need_check_payload(hmac_vap_stru *pst_vap,
                                               mac_ieee80211_frame_stru *pst_frame_hdr, oal_uint16 us_frame_len)
{
    /* 根据长度、bssid、sa、da确定接收到特定的广播帧 */
    if (OAL_MEMCMP(pst_frame_hdr->auc_address3, mac_mib_get_StationID(&pst_vap->st_vap_base_info),
                   WLAN_MAC_ADDR_LEN) != 0) {
        return OAL_FALSE;
    }
    if (OAL_MEMCMP(pst_frame_hdr->auc_address1, BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN) != 0) {
        return OAL_FALSE;
    }
    if (OAL_MEMCMP(pst_frame_hdr->auc_address2, pst_vap->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN) != 0) {
        return OAL_FALSE;
    }
    if (us_frame_len < (WLAN_PACKET_CHECK_DATA_MIN_LEN + MAC_80211_FRAME_LEN)) {
        return OAL_FALSE;
    }
    return OAL_TRUE;
}


oal_bool_enum_uint8 hmac_rx_verification_filter(hmac_vap_stru *pst_vap, oal_netbuf_stru *pst_netbuf,
                                                mac_ieee80211_frame_stru *pst_frame_hdr)
{
    oal_uint16 us_data_len;
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
    oal_uint8 *puc_payload = OAL_PTR_NULL;
    hmac_packet_check_rx_info_stru *pst_pkt_check_rx_info = hmac_get_pkt_check_rx_info_addr();

    pst_hmac_device = hmac_res_get_mac_dev(pst_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_verification_filter::hmac_res_get_mac_dev null. device_id:%u}",
                       pst_vap->st_vap_base_info.uc_device_id);
        return OAL_FALSE;
        ;
    }
    if (pst_hmac_device->st_packet_check.en_pkt_check_on == OAL_FALSE) {
        return OAL_FALSE;
    }

    /* 获取该MPDU的控制信息 */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctrl->st_rx_info), pst_netbuf);

    if (hmac_rx_need_check_payload(pst_vap, pst_frame_hdr, pst_rx_ctrl->st_rx_info.us_frame_len) == OAL_FALSE) {
        return OAL_FALSE;
    }

    if (hmac_rx_check_snap(puc_payload) != OAL_SUCC) {
        return OAL_FALSE;
    }

    /* 将指针指向data */
    puc_payload += SNAP_LLC_FRAME_LEN;
    us_data_len = pst_rx_ctrl->st_rx_info.us_frame_len - MAC_80211_FRAME_LEN - SNAP_LLC_FRAME_LEN;

    if (hmac_rx_check_data(pst_hmac_device, puc_payload, us_data_len) != OAL_SUCC) {
        hmac_packet_check_rsp(pst_vap, pst_hmac_device, OAL_FALSE);
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_verification_filter::hmac_rx_check_data failed.}");

        oam_report_80211_frame(BROADCAST_MACADDR, WLAN_MAC_ADDR_LEN,
                               (oal_uint8 *)(pst_frame_hdr),
                               MAC_GET_RX_CB_MAC_HEADER_LEN(&(pst_rx_ctrl->st_rx_info)),
                               puc_payload,
                               pst_rx_ctrl->st_rx_info.us_frame_len,
                               OAM_OTA_FRAME_DIRECTION_TYPE_RX);
        return OAL_FALSE;
    }

    /* 所有发出的包都收到,且帧内容都正确 */
    if ((pst_pkt_check_rx_info->uc_pkt_check_num0 + pst_pkt_check_rx_info->uc_pkt_check_num1 +
         pst_pkt_check_rx_info->uc_pkt_check_num2 + pst_pkt_check_rx_info->uc_pkt_check_num3) ==
        (pst_pkt_check_rx_info->us_pkt_check_send_num * WLAN_PACKET_CHECK_TYPE_NUM)) {
        hmac_packet_check_rsp(pst_vap, pst_hmac_device, OAL_TRUE);
    }
    oam_warning_log4(0, OAM_SF_RX,
                     "{hmac_rx_verification_filter::check_num0=%d,check_num1=%d,check_num2=%d,check_num3 = %d.}",
                     pst_pkt_check_rx_info->uc_pkt_check_num0, pst_pkt_check_rx_info->uc_pkt_check_num1,
                     pst_pkt_check_rx_info->uc_pkt_check_num2, pst_pkt_check_rx_info->uc_pkt_check_num3);
    return OAL_TRUE;
}


oal_void hmac_rx_lan_frame(oal_netbuf_head_stru *pst_netbuf_header)
{
    oal_uint32 ul_netbuf_num;
    oal_netbuf_stru *pst_temp_netbuf;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    oal_uint8 uc_buf_nums;
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;
    hmac_vap_stru *pst_vap = OAL_PTR_NULL;

    ul_netbuf_num = oal_netbuf_get_buf_num(pst_netbuf_header);
    pst_temp_netbuf = oal_netbuf_peek(pst_netbuf_header);

    while (ul_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == NULL) {
            break;
        }

        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
        uc_buf_nums = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        ul_netbuf_num = oal_sub(ul_netbuf_num, uc_buf_nums);
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);

        pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (pst_vap == OAL_PTR_NULL) {
            OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_lan_frame::mac_res_get_hmac_vap null. vap_id:%u}",
                           pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
            continue;
        }

        if (hmac_rx_verification_filter(pst_vap, pst_netbuf, pst_frame_hdr) == OAL_TRUE) {
            oal_netbuf_free(pst_netbuf);
            continue;
        }
        hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr);
    }

    return;
}

oal_uint32 hmac_rx_process_data_ap(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event = OAL_PTR_NULL;
    frw_event_hdr_stru *pst_event_hdr = OAL_PTR_NULL;
    dmac_wlan_drx_event_stru *pst_wlan_rx_event = OAL_PTR_NULL;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;      /* 用于保存当前处理的MPDU的第一个netbuf指针 */
    oal_netbuf_stru *pst_temp_netbuf = OAL_PTR_NULL; /* 用于临时保存下一个需要处理的netbuf指针 */
    oal_uint16 us_netbuf_num;                        /* netbuf链表的个数 */
    oal_netbuf_head_stru st_netbuf_header;           /* 存储上报给网络层的数据 */
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_stru st_temp_header;
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
#endif
#ifdef _PRE_WLAN_CHBA_MGMT
    hmac_user_stru *pst_hmac_user = NULL;
    mac_rx_ctl_stru *cb = NULL;
#endif

    if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_START);

    /* 获取事件头和事件结构体指针 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr = &(pst_event->st_event_hdr);
    pst_wlan_rx_event = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
    pst_temp_netbuf = pst_wlan_rx_event->pst_netbuf;
    us_netbuf_num = pst_wlan_rx_event->us_netbuf_num;

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_BASE_INFO);

    if (oal_unlikely(pst_temp_netbuf == OAL_PTR_NULL)) {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_rx_process_data_ap::us_netbuf_num = %d.}", us_netbuf_num);
        return OAL_SUCC; /* 这个是事件处理函数，为了防止51的UT挂掉 返回 true */
    }
#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_rx_process_data_ap::pst_hmac_device null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_hmac_vap null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 暂时规避mib_info 指针为空的问题，
      If mib info is null ptr,release the netbuf */
    if (pst_hmac_vap->st_vap_base_info.pst_mib_info == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_rx_process_data_ap::pst_mib_info null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_SUCC;
    }
#endif

    hmac_rx_process_tcp_ack_record(pst_hmac_vap, pst_temp_netbuf);

    /* 将所有netbuff全部入链表 */
    oal_netbuf_list_head_init(&st_netbuf_header);
    while (us_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_temp_netbuf = oal_netbuf_next(pst_netbuf);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* 统计收包的总数，计入全局变量 */
        hmac_wifi_statistic_rx_packets(oal_netbuf_len(pst_netbuf));
#endif
#ifdef _PRE_WLAN_CHBA_MGMT
        cb = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(MAC_GET_RX_CB_TA_USER_IDX(cb));
        hmac_stat_user_rx_netbuf(pst_hmac_user, pst_netbuf);
#endif
        oal_netbuf_list_tail_nolock(&st_netbuf_header, pst_netbuf);
        us_netbuf_num--;
    }

    if (us_netbuf_num != 0) {
        oam_error_log2(0, OAM_SF_RX, "{hmac_rx_process_data_ap::us_netbuf_num[%d], event_buf_num[%d].}",
                       us_netbuf_num, pst_wlan_rx_event->us_netbuf_num);
    }

    /* 将Dmac上报的帧进入reorder队列过滤一下 */
    hmac_rx_process_data_filter(&st_netbuf_header, pst_wlan_rx_event->pst_netbuf, pst_wlan_rx_event->us_netbuf_num);

#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_init(&st_temp_header);

    while (!!(pst_temp_netbuf = oal_netbuf_delist(&st_netbuf_header))) {
        if (!hmac_transfer_rx_handler(pst_hmac_device, pst_hmac_vap, pst_temp_netbuf)) {
            oal_netbuf_list_tail(&st_temp_header, pst_temp_netbuf);
        }
    }
    /*lint -e522*/
    oal_warn_on(!oal_netbuf_list_empty(&st_netbuf_header));
    /*lint +e522*/
    oal_netbuf_splice_init(&st_temp_header, &st_netbuf_header);
#endif

    hmac_rx_process_data_ap_tcp_ack_opt(pst_hmac_vap, &st_netbuf_header);
    return OAL_SUCC;
}
OAL_STATIC oal_void hmac_find_user_fail(hmac_vap_stru *pst_vap, frw_event_hdr_stru *pst_event_hdr,
    mac_ieee80211_frame_stru *pst_frame_hdr, oal_netbuf_stru *pst_netbuf)
{
    oam_info_log0(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{hmac_rx_lan_frame_classify:frame is a unique frame.}");
    /* 目的用户不在AP的用户表中，调用wlan_to_lan转发接口 */
    hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr);
}
OAL_STATIC oal_void hmac_get_da_user_fail(frw_event_hdr_stru *pst_event_hdr, oal_uint16 us_user_dix)
{
    OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_RX, "{hmac_rx_lan_frame_classify::hmac_da_user[%d] null.}",
                     us_user_dix);
}
OAL_STATIC oal_void hmac_user_assoc_state_wrong(frw_event_hdr_stru *pst_event_hdr)
{
    oam_warning_log0(pst_event_hdr->uc_vap_id, OAM_SF_RX,
                     "{hmac_rx_lan_frame_classify::the station is not associated with ap.}");
}
OAL_STATIC oal_void hmac_put_msdu_to_wlan(frw_event_hdr_stru *pst_event_hdr,
    oal_netbuf_head_stru *pst_w2w_netbuf_hdr)
{
    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_TO_LAN);

    /* 将MSDU链表交给发送流程处理 */
    if ((oal_netbuf_list_empty(pst_w2w_netbuf_hdr) == OAL_FALSE) &&
        (oal_netbuf_tail(pst_w2w_netbuf_hdr) != OAL_PTR_NULL) &&
        (oal_netbuf_peek(pst_w2w_netbuf_hdr) != OAL_PTR_NULL)) {
        oal_netbuf_next((oal_netbuf_tail(pst_w2w_netbuf_hdr))) = OAL_PTR_NULL;
        oal_netbuf_prev((oal_netbuf_peek(pst_w2w_netbuf_hdr))) = OAL_PTR_NULL;

        hmac_rx_transmit_to_wlan(pst_event_hdr, pst_w2w_netbuf_hdr);
    }
    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_TO_WLAN);

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_END);
}

oal_void hmac_rx_process_data_ap_tcp_ack_opt(hmac_vap_stru *pst_vap, oal_netbuf_head_stru *pst_netbuf_header)
{
    frw_event_hdr_stru st_event_hdr;
    mac_ieee80211_frame_stru *pst_frame_hdr = OAL_PTR_NULL;      /* 保存mac帧的指针 */
    mac_ieee80211_frame_stru *pst_copy_frame_hdr = OAL_PTR_NULL; /* 保存mac帧的指针 */
    oal_uint8 *puc_da = OAL_PTR_NULL;                            /* 保存用户目的地址的指针 */
    hmac_user_stru *pst_hmac_da_user = OAL_PTR_NULL;
    oal_uint32 ul_rslt;
    oal_uint16 us_user_dix;
    hmac_rx_ctl_stru *pst_rx_ctrl = OAL_PTR_NULL;    /* 每一个MPDU的控制信息 */
    oal_uint16 us_netbuf_num;                        /* netbuf链表的个数 */
    oal_uint8 uc_buf_nums;                           /* 每个mpdu占有buf的个数 */
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;      /* 用于保存当前处理的MPDU的第一个netbuf指针 */
    oal_netbuf_stru *pst_temp_netbuf = OAL_PTR_NULL; /* 用于临时保存下一个需要处理的netbuf指针 */
    oal_netbuf_stru *pst_netbuf_copy = OAL_PTR_NULL; /* 用于保存组播帧copy */
    oal_netbuf_head_stru st_w2w_netbuf_hdr;          /* 保存wlan to wlan的netbuf链表的头 */
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    cs_isolation_forward_enum en_forward;
#endif

    /* 循环收到的每一个MPDU，处情况如下:
        1、组播帧时，调用WLAN TO WLAN和WLAN TO LAN接口
        2、其他，根据实际情况，调用WLAN TO LAN接口或者WLAN TO WLAN接口 */
    oal_netbuf_list_head_init(&st_w2w_netbuf_hdr);
    pst_temp_netbuf = oal_netbuf_peek(pst_netbuf_header);
    us_netbuf_num = (oal_uint16)oal_netbuf_get_buf_num(pst_netbuf_header);
    st_event_hdr.uc_chip_id = pst_vap->st_vap_base_info.uc_chip_id;
    st_event_hdr.uc_device_id = pst_vap->st_vap_base_info.uc_device_id;
    st_event_hdr.uc_vap_id = pst_vap->st_vap_base_info.uc_vap_id;

    while (us_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

        /* 获取帧头信息 */
        pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

        /* 获取当前MPDU占用的netbuf数目 */
        uc_buf_nums = pst_rx_ctrl->st_rx_info.bit_buff_nums;

        /* 获取下一个要处理的MPDU */
        oal_netbuf_get_appointed_netbuf(pst_netbuf, uc_buf_nums, &pst_temp_netbuf);
        us_netbuf_num = oal_sub(us_netbuf_num, uc_buf_nums);

        pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_rx_ctrl->st_rx_info.uc_mac_vap_id);
        if (oal_unlikely(pst_vap == OAL_PTR_NULL)) {
            oam_warning_log0(pst_rx_ctrl->st_rx_info.bit_vap_id, OAM_SF_RX, "{hmac_rx_process_data_ap::pst_vap null.}");
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            continue;
        }

        /* 获取接收端地址 */
        mac_rx_get_da(pst_frame_hdr, &puc_da);

        /* 目的地址为组播地址时，进行WLAN_TO_WLAN和WLAN_TO_LAN的转发 */
        if (ether_is_multicast(puc_da)) {
            oam_info_log0(st_event_hdr.uc_vap_id, OAM_SF_RX, "{hmac_rx_lan_frame_classify::frame is a group frame.}");
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_mcast_cnt, 1);

            if (hmac_rx_copy_netbuff(&pst_netbuf_copy, pst_netbuf, pst_rx_ctrl->st_rx_info.uc_mac_vap_id,
                                     &pst_copy_frame_hdr) != OAL_SUCC) {
                oam_warning_log0(st_event_hdr.uc_vap_id, OAM_SF_RX, "{hmac_rx_process_data_ap::send mcast pkt fail.}");

                oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_no_buff_dropped, 1);
                continue;
            }

            hmac_rx_lan_frame_classify(pst_vap, pst_netbuf, pst_frame_hdr);  // 上报网络层

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
            pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf_copy);

            /* 获取帧头信息 */
            pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
            mac_rx_get_da(pst_frame_hdr, &puc_da);

            en_forward = hmac_isolation_filter(&pst_vap->st_vap_base_info, puc_da);
            if (en_forward == CS_ISOLATION_FORWORD_DROP) {
                /* 释放当前处理的MPDU占用的netbuf. 2014.7.29 cause memory leak bug fixed */
                /* OAL_IO_PRINT("isolation drop %d-%d\n",uc_netbuf_num,uc_buf_nums);1-1 */
                hmac_rx_free_netbuf(pst_netbuf_copy, (oal_uint16)uc_buf_nums);
                continue;
            }
#endif

            /* 将MPDU解析成单个MSDU，把所有的MSDU组成一个netbuf链 */
            hmac_rx_prepare_msdu_list_to_wlan(pst_vap, &st_w2w_netbuf_hdr, pst_netbuf_copy, pst_copy_frame_hdr);
            continue;
        }

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
        en_forward = hmac_isolation_filter(&pst_vap->st_vap_base_info, puc_da);
        if (en_forward == CS_ISOLATION_FORWORD_DROP) {
            /* 释放当前处理的MPDU占用的netbuf. 2014.7.29 cause memory leak bug fixed */
            /* OAL_IO_PRINT("isolation drop %d-%d\n",uc_netbuf_num,uc_buf_nums);1-1 */
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            /* return OAL_SUCC; bug fixed */
            continue;
        }
#endif

        /* 获取目的地址对应的用户指针 */
        ul_rslt = mac_vap_find_user_by_macaddr(&pst_vap->st_vap_base_info, puc_da, WLAN_MAC_ADDR_LEN, &us_user_dix);
        if (ul_rslt == OAL_ERR_CODE_PTR_NULL) {
            /* 释放当前处理的MPDU占用的netbuf */
            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);

            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);
            continue;
        }

        /* 没有找到对应的用户 */
        if (ul_rslt != OAL_SUCC) {
            hmac_find_user_fail(pst_vap, &st_event_hdr, pst_frame_hdr, pst_netbuf);
            continue;
        }

        /* 目的用户已在AP的用户表中，进行WLAN_TO_WLAN转发 */
        pst_hmac_da_user = (hmac_user_stru *)mac_res_get_hmac_user(us_user_dix);
        if (pst_hmac_da_user == OAL_PTR_NULL) {
            hmac_get_da_user_fail(&st_event_hdr, us_user_dix);
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);

            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            continue;
        }

        if (pst_hmac_da_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
            hmac_user_assoc_state_wrong(&st_event_hdr);
            oam_stat_vap_incr(pst_vap->st_vap_base_info.uc_vap_id, rx_da_check_dropped, 1);

            hmac_rx_free_netbuf(pst_netbuf, (oal_uint16)uc_buf_nums);
            hmac_mgmt_send_deauth_frame(&pst_vap->st_vap_base_info, puc_da, WLAN_MAC_ADDR_LEN,
                MAC_NOT_AUTHED, OAL_FALSE);

            continue;
        }

        /* 将MPDU解析成单个MSDU，把所有的MSDU组成一个netbuf链 */
        hmac_rx_prepare_msdu_list_to_wlan(pst_vap, &st_w2w_netbuf_hdr, pst_netbuf, pst_frame_hdr);
    }

    hmac_put_msdu_to_wlan(&st_event_hdr, &st_w2w_netbuf_hdr);
}


oal_uint32 hmac_rx_process_data_sta(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event = OAL_PTR_NULL;
    frw_event_hdr_stru *pst_event_hdr = OAL_PTR_NULL;
    dmac_wlan_drx_event_stru *pst_wlan_rx_event = OAL_PTR_NULL;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL; /* 用于临时保存下一个需要处理的netbuf指针 */
    oal_uint16 us_netbuf_num;                   /* netbuf链表的个数 */
    oal_netbuf_head_stru st_netbuf_header;      /* 存储上报给网络层的数据 */
    oal_netbuf_stru *pst_temp_netbuf = OAL_PTR_NULL;
    hmac_vap_stru *pst_hmac_vap = OAL_PTR_NULL;
#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_stru st_temp_header;
    hmac_device_stru *pst_hmac_device = OAL_PTR_NULL;
#endif

    if (oal_unlikely(pst_event_mem == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_sta::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_START);

    /* 获取事件头和事件结构体指针 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_event_hdr = &(pst_event->st_event_hdr);
    pst_wlan_rx_event = (dmac_wlan_drx_event_stru *)(pst_event->auc_event_data);
    pst_temp_netbuf = pst_wlan_rx_event->pst_netbuf;
    us_netbuf_num = pst_wlan_rx_event->us_netbuf_num;

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_BASE_INFO);

#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_device = hmac_res_get_mac_dev(pst_event_hdr->uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_rx_process_data_sta::pst_hmac_device null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }
#endif

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (pst_hmac_vap == OAL_PTR_NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_data_sta::pst_hmac_vap null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* If mib info is null ptr,release the netbuf */
    if (pst_hmac_vap->st_vap_base_info.pst_mib_info == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_rx_process_data_sta::pst_mib_info null.}");
        hmac_rx_free_netbuf(pst_temp_netbuf, us_netbuf_num);
        return OAL_SUCC;
    }
#endif

    hmac_rx_process_tcp_ack_record(pst_hmac_vap, pst_temp_netbuf);

    /* 将所有netbuff全部入链表 */
    oal_netbuf_list_head_init(&st_netbuf_header);
    while (us_netbuf_num != 0) {
        pst_netbuf = pst_temp_netbuf;
        if (pst_netbuf == OAL_PTR_NULL) {
            break;
        }

        pst_temp_netbuf = oal_netbuf_next(pst_netbuf);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        hmac_wifi_statistic_rx_packets(oal_netbuf_len(pst_netbuf));
#endif

        oal_netbuf_list_tail_nolock(&st_netbuf_header, pst_netbuf);
        us_netbuf_num--;
    }

    if (us_netbuf_num != 0) {
        oam_error_log2(0, OAM_SF_RX, "{hmac_rx_process_data_sta::us_netbuf_num[%d], event_buf_num[%d].}",
                       us_netbuf_num, pst_wlan_rx_event->us_netbuf_num);
    }

    hmac_rx_process_data_filter(&st_netbuf_header, pst_wlan_rx_event->pst_netbuf, pst_wlan_rx_event->us_netbuf_num);

#ifdef _PRE_WLAN_TCP_OPT
    oal_netbuf_head_init(&st_temp_header);
    while (!!(pst_temp_netbuf = oal_netbuf_delist(&st_netbuf_header))) {
        if (hmac_transfer_rx_handler(pst_hmac_device, pst_hmac_vap, pst_temp_netbuf) == OAL_FALSE) {
            oal_netbuf_list_tail(&st_temp_header, pst_temp_netbuf);
        }
    }
    /*lint -e522*/
    oal_warn_on(!oal_netbuf_list_empty(&st_netbuf_header));
    /*lint +e522*/
    oal_netbuf_splice_init(&st_temp_header, &st_netbuf_header);
#endif

    hmac_rx_process_data_sta_tcp_ack_opt(pst_hmac_vap, &st_netbuf_header);
    return OAL_SUCC;
}


oal_void hmac_rx_process_data_sta_tcp_ack_opt(hmac_vap_stru *pst_vap, oal_netbuf_head_stru *pst_netbuf_header)
{
    /* 将需要上报的帧逐一出队处理 */
    hmac_rx_lan_frame(pst_netbuf_header);

    oam_profiling_rx_statistic(OAM_PROFILING_FUNC_RX_HMAC_END);
    return;
}


