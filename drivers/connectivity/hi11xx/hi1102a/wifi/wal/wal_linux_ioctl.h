

#ifndef __WAL_LINUX_IOCTL_H__
#define __WAL_LINUX_IOCTL_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_config.h"
#include "hmac_scan.h"
#include "hmac_statistic_data_flow.h"

#ifdef CONFIG_DOZE_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif /* CONFIG_DOZE_FILTER */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define WAL_HIPRIV_CMD_MAX_LEN       (WLAN_MEM_LOCAL_SIZE2 - 4)     /* 私有配置命令字符串最大长度，对应本地内存池一级大小 */

#define WAL_HIPRIV_CMD_NAME_MAX_LEN  80                             /* 字符串中每个单词的最大长度(原20) */
#define WAL_HIPRIV_CMD_VALUE_MAX_LEN 10                             /* 字符串中某个对应变量取值的最大位数 */

#define WAL_HIPRIV_PROC_ENTRY_NAME   "hipriv"

#define WAL_SIOCDEVPRIVATE              0x89F0  /* SIOCDEVPRIVATE */

#define WAL_HIPRIV_HT_MCS_MIN           0
#define WAL_HIPRIV_HT_MCS_MAX           32
#define WAL_HIPRIV_VHT_MCS_MIN          0
#define WAL_HIPRIV_VHT_MCS_MAX          9
#define WAL_HIPRIV_NSS_MIN              1
#define WAL_HIPRIV_NSS_MAX              4
#define WAL_HIPRIV_CH_NUM               4

#define WAL_HIPRIV_BOOL_NIM             0
#define WAL_HIPRIV_BOOL_MAX             1
#define WAL_HIPRIV_FREQ_SKEW_ARG_NUM    8

#define WAL_HIPRIV_MS_TO_S                   1000   /* ms和s之间倍数差 */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MIN    5000   /* 受默认老化计数器出发时间所限制 */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MAX    0xffff /* timer间隔时间限制所致(oal_uin16) */


/* IOCTL私有配置命令宏定义 */
#define WAL_IOCTL_PRIV_SETPARAM          (OAL_SIOCIWFIRSTPRIV + 0)
#define WAL_IOCTL_PRIV_GETPARAM          (OAL_SIOCIWFIRSTPRIV + 1)
#define WAL_IOCTL_PRIV_SET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 3)
#define WAL_IOCTL_PRIV_GET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 5)
#define WAL_IOCTL_PRIV_SET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 8)
#define WAL_IOCTL_PRIV_GET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 9)

#define WAL_IOCTL_PRIV_GET_MODE     (OAL_SIOCIWFIRSTPRIV + 17)      /* 读取模式 */
#define WAL_IOCTL_PRIV_SET_MODE     (OAL_SIOCIWFIRSTPRIV + 18)      /* 设置模式 包括协议 频段 带宽 */

#define WAL_IOCTL_PRIV_AP_GET_STA_LIST               (OAL_SIOCIWFIRSTPRIV + 21)
#define WAL_IOCTL_PRIV_AP_MAC_FLTR	                 (OAL_SIOCIWFIRSTPRIV + 22)
/* netd将此配置命令作为GET方式下发，get方式命令用奇数，set用偶数 */
#define WAL_IOCTL_PRIV_SET_AP_CFG                    (OAL_SIOCIWFIRSTPRIV + 23)
#define WAL_IOCTL_PRIV_AP_STA_DISASSOC		         (OAL_SIOCIWFIRSTPRIV + 24)

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
#define WAL_IOCTL_PRIV_GET_BLACKLIST        (OAL_SIOCIWFIRSTPRIV + 27)
#define WAL_IOCTL_PRIV_SET_ISOLATION        (OAL_SIOCIWFIRSTPRIV + 28)
#endif
#define WAL_IOCTL_PRIV_SUBCMD_MAX_LEN          20
#define WAL_CALI_MASK_CLOSEALL                 0x2000
typedef oal_uint32  (*wal_hipriv_cmd_func)(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum {
    WAL_DSCR_PARAM_FREQ_BANDWIDTH_MODE = 0,      /* 频带模式 */
    WAL_DSCR_PARAM_PA_GAIN_LEVEL,      /* pa增益等级 */
    WAL_DSCR_PARAM_MICRO_TX_POWER_GAIN_LEVEL,
    WAL_DSCR_PARAM_SMART_ANTENNA_ENABLE,
    WAL_DSCR_PARAM_TXRTS_ANTENNA,
    WAL_DSCR_PARAM_RXCTRL_ANTENNA,
    WAL_DSCR_PARAM_CHANNAL_CODE,
    WAL_DSCR_PARAM_DATA_RATE0,
    WAL_DSCR_PARAM_DATA_RATE1,
    WAL_DSCR_PARAM_DATA_RATE2,
    WAL_DSCR_PARAM_DATA_RATE3,
    WAL_DSCR_PARAM_POWER,
    WAL_DSCR_PARAM_SHORTGI,
    WAL_DSCR_PARAM_PREAMBLE_MODE,
    WAL_DSCR_PARAM_RTSCTS,
    WAL_DSCR_PARAM_LSIGTXOP,
    WAL_DSCR_PARAM_SMOOTH,
    WAL_DSCR_PARAM_SOUNDING,
    WAL_DSCR_PARAM_TXBF,
    WAL_DSCR_PARAM_STBC,
    WAL_DSCR_PARAM_GET_ESS,
    WAL_DSCR_PARAM_DYN_BW,
    WAL_DSCR_PARAM_DYN_BW_EXIST,
    WAL_DSCR_PARAM_CH_BW_EXIST,

    WAL_DSCR_PARAM_BUTT
}wal_dscr_param_enum;
typedef oal_uint8 wal_dscr_param_enum_uint8;


/* rx ip数据包过滤功能和上层协定(格式)结构体，TBD 上层接口尚未明确，需澄清后修改 */
#ifdef CONFIG_DOZE_FILTER
typedef hw_wifi_filter_item wal_hw_wifi_filter_item;
typedef struct hw_wlan_filter_ops wal_hw_wlan_filter_ops;
#else
typedef struct {
    unsigned short protocol;  // 协议类型
    unsigned short port;      // 目的端口号
    unsigned int   filter_cnt;    // 过滤报文数
}wal_hw_wifi_filter_item;

typedef struct {
    int (*set_filter_enable)(int);
    int (*set_filter_enable_ex)(int, int);
    int (*add_filter_items)(wal_hw_wifi_filter_item*, int);
    int (*clear_filters)(void);
    int (*get_filter_pkg_stat)(wal_hw_wifi_filter_item*, int, int*);
}wal_hw_wlan_filter_ops;
#endif

typedef enum {
    WAL_TX_POW_PARAM_SET_RF_REG_CTL = 0,
    WAL_TX_POW_PARAM_SET_MAG_LEVEL,
    WAL_TX_POW_PARAM_SET_CTL_LEVEL,
    WAL_TX_POW_PARAM_SHOW_LOG,

    WAL_TX_POW_PARAM_BUTT
}wal_tx_pow_param_enum;
typedef oal_uint8 wal_tx_pow_param_enum_uint8;

/* wifi 能力开关状态枚举值，通知到上层使用 */
typedef enum {
    WAL_WIFI_FEATURE_SUPPORT_11K                   = 0,
    WAL_WIFI_FEATURE_SUPPORT_11V                   = 1,
    WAL_WIFI_FEATURE_SUPPORT_11R                   = 2,
    WAL_WIFI_FEATURE_SUPPORT_VOWIFI_NAT_KEEP_ALIVE = 3,
    WAL_WIFI_FEATURE_SUPPORT_BUTT
}wal_wifi_feature_capbility_enum;
/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_int32 g_wlan_cal_disable_1102a;
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_mutex_stru g_st_dfr_mutex;
#endif
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_uint8 g_sar_params[CUS_NUM_OF_SAR_LVL][CUS_SAR_NUM];
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0))
extern oal_ieee80211_supported_band g_hi1151_band_2ghz;
#endif
extern oal_uint8 g_sk_pacing_shift;
extern OAL_CONST oal_int8 *g_pauc_tx_dscr_param_name[];
extern oal_iw_handler_def_stru g_st_iw_handler_def;
extern oal_net_device_ops_stru g_st_wal_net_dev_ops;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) && (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_ethtool_ops_stru g_st_wal_ethtool_ops;
#endif
extern oal_bool_enum_uint8 g_aen_tas_switch_en[WITP_RF_CHANNEL_NUMS];

/*****************************************************************************
  5 消息头定义
*****************************************************************************/
/*****************************************************************************
  6 消息定义
*****************************************************************************/
/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/* 私有命令入口结构定义 */
typedef struct {
    oal_int8           *pc_cmd_name;    /* 命令字符串 */
    wal_hipriv_cmd_func p_func;         /* 命令对应处理函数 */
}wal_hipriv_cmd_entry_stru;
typedef struct {
    int8_t *pc_name;                    /* 配置命令字符串 */
    wlan_cfgid_enum_uint16 en_tlv_cfg_id; /* 配置命令对应的枚举值 */
    uint8_t auc_resv[2];                /* 2实现字节对齐 */
} wal_ioctl_tlv_stru;

#ifdef _PRE_WLAN_FEATURE_VOWIFI
/* VoWiFi命令的转换结构体 */
typedef struct {
    oal_int8                 *pc_vowifi_cmd_name;    /* 命令字符串 */
    mac_vowifi_cmd_enum_uint8 en_vowifi_cmd;         /* 命令对应类型 */
    oal_uint8                 auc_resv[3];
}wal_vowifi_cmd_stru;
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

/* 协议模式与字符串映射 */
typedef struct {
    oal_int8                           *pc_name;        /* 模式名字符串 */
    wlan_protocol_enum_uint8            en_mode;        /* 协议模式 */
    wlan_channel_band_enum_uint8        en_band;        /* 频段 */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;   /* 带宽 */
    oal_uint8                           auc_resv[1];
}wal_ioctl_mode_map_stru;

/* 算法参数配置结构体 */
typedef struct {
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    mac_alg_cfg_enum_uint8              en_alg_cfg;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[3];    /* 字节对齐 */
}wal_ioctl_alg_cfg_stru;
/* 1102 使用wpa_supplicant 下发命令 */
typedef struct wal_wifi_priv_cmd {
    oal_uint8    *puc_buf;
    oal_uint32    ul_total_len;
    oal_uint32    ul_used_len;
} wal_wifi_priv_cmd_stru;

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
/* dyn cali 参数配置结构体 */
typedef struct {
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    mac_dyn_cali_cfg_enum_uint8         en_dyn_cali_cfg;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[3];    /* 字节对齐 */
}wal_ioctl_dyn_cali_stru;
#endif
#ifdef _PRE_WLAN_FEATURE_NRCOEX
typedef struct {
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    wlan_nrcoex_cmd_enum_uint8          en_nrcoex_cmd;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[3];    /* 字节对齐 */
}wal_ioctl_nrcoex_cmd_stru;
#endif
extern OAL_CONST wal_ioctl_alg_cfg_stru g_ast_alg_cfg_map[];
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern OAL_CONST wal_ioctl_dyn_cali_stru g_ast_dyn_cali_cfg_map[];
#endif
/*****************************************************************************
  8 UNION定义
*****************************************************************************/
/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint8 wlan_pm_get_fast_check_cnt(void);
extern void wlan_pm_set_fast_check_cnt(oal_uint8 fast_check_cnt);
#if (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) && (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
extern oal_void frw_event_process_all_event(oal_uint ui_data);
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
extern oal_void chr_dev_cali_excp_trigger_dfr(oal_void);
extern oal_void wlan_dfr_custom_cali(oal_void);
#endif
extern oal_bool_enum_uint8 hwifi_get_fact_cali_completed(oal_void);
extern oal_void hwifi_set_fact_cali_completed(oal_bool_enum_uint8 fact_cali_completed);
extern oal_uint32  wal_hipriv_set_rate(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_11D
extern oal_int32  wal_regdomain_update(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country, oal_uint8 len);
#endif
extern oal_uint32  wal_hipriv_set_mcs(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_set_mcsac(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_vap_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32  wal_hipriv_set_ip_filter(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif //_PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32  wal_hipriv_create_proc(oal_void *p_proc_arg);
extern oal_int32   wal_netdev_stop(oal_net_device_stru *pst_net_dev);
extern oal_int32   wal_netdev_open(oal_net_device_stru *pst_net_dev);
extern oal_uint32  wal_hipriv_del_vap(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_remove_proc(oal_void);
extern oal_uint32  wal_alloc_cfg_event(oal_net_device_stru *pst_net_dev, frw_event_mem_stru **ppst_event_mem,
    oal_void *pst_resp_addr, wal_msg_stru **ppst_cfg_msg, oal_uint16 us_len);
extern oal_int32   wal_send_cfg_event(
                                   oal_net_device_stru      *pst_net_dev,
                                   wal_msg_type_enum_uint8   en_msg_type,
                                   oal_uint16                us_len,
                                   oal_uint8                *puc_param,
                                   oal_bool_enum_uint8       en_need_rsp,
                                   wal_msg_stru            **ppst_rsp_msg);

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
extern oal_int32 wal_start_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32  wal_stop_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_netdev_stop_ap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_deinit_wlan_vap(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_netdev(oal_wiphy_stru *pst_wiphy, char *dev_name);
extern oal_int32  wal_setup_ap(oal_net_device_stru *pst_net_dev);
extern oal_int32  wal_host_dev_init(oal_net_device_stru *pst_net_dev);
extern oal_int32  wal_host_dev_exit(oal_net_device_stru *pst_net_dev);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
extern oal_uint32  wal_regdomain_update_sta(oal_uint8 uc_vap_id);
oal_int32 wal_regdomain_update_country_code(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_int32  wal_set_random_mac_to_mib(oal_net_device_stru *pst_net_dev);
#endif

wlan_p2p_mode_enum_uint8 wal_wireless_iftype_to_mac_p2p_mode(enum nl80211_iftype en_iftype);

extern oal_int32  wal_cfg_vap_h2d_event(oal_net_device_stru *pst_net_dev);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_uint32 hwifi_config_init_dts_main(oal_net_device_stru *pst_cfg_net_dev);
extern oal_int32 wal_set_custom_process_func(custom_cali_func p_cus_fun, priv_ini_config_func p_priv_func);
extern oal_uint32 hwifi_config_init_nvram_main(oal_net_device_stru *pst_cfg_net_dev);

extern oal_uint32 wal_custom_cali(oal_void);
extern oal_uint32 wal_priv_init_config(oal_void);
extern oal_void hwifi_config_init_force(oal_void);
extern oal_int32 hwifi_config_init_ini_priv(oal_net_device_stru *pst_cfg_net_dev);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
extern oal_uint32  wal_hipriv_set_essid(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
extern oal_uint32  wal_get_cmd_one_arg(
    oal_int8 *pc_cmd, oal_int8 *pc_arg, oal_uint32 ul_arg_len, oal_uint32 *pul_cmd_offset);
#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
extern oal_uint32  wal_hipriv_send_cfg_uint32_data(oal_net_device_stru *pst_net_dev,
    oal_int8 *pc_param, wlan_cfgid_enum_uint16 cfgid);
#endif
#ifdef _PRE_WLAN_CFGID_DEBUG
extern oal_uint32 wal_hipriv_get_debug_cmd_size(oal_void);
#endif
extern oal_uint32  wal_hipriv_alg_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_parse_cmd(oal_int8 *pc_cmd);

extern oal_int32 wal_set_ip_filter_enable(oal_int32 l_on);
extern oal_int32 wal_set_assigned_filter_enable(oal_int32 l_filter_id, oal_int32 l_switch);
extern oal_int32 wal_add_ip_filter_items(wal_hw_wifi_filter_item *pst_items, oal_int32 l_count);
extern oal_int32 wal_clear_ip_filter(void);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_int32 wal_register_ip_filter(wal_hw_wlan_filter_ops *pg_st_ip_filter_ops);
extern oal_int32 wal_unregister_ip_filter(void);
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_uint32  wal_hipriv_dyn_cali_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *puc_param);
#endif
oal_int32 wal_ioctl_get_psm_info(oal_net_device_stru *pst_net_dev,
    mac_psm_query_type_enum_uint8 en_query_type, wal_wifi_priv_cmd_stru *priv_cmd);
#ifdef _PRE_WLAN_CHBA_MGMT
int32_t wal_setup_chba(oal_net_device_stru *net_dev);
int32_t wal_init_chba_netdev(oal_wiphy_stru *pst_wiphy, const char *dev_name);
#endif
oal_int8 *wal_get_wlan_name(void);
oal_int8 *wal_get_p2p_name(void);
oal_int32 wal_ioctl_set_sae_pwe(oal_net_device_stru *pst_net_dev, oal_uint8 sae_pwe);
#ifdef _PRE_WLAN_NARROW_BAND
oal_uint32 wal_hipriv_narrow_bw(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_nb_report_info_read(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_nokey_mode_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_hitalk_key_set(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
oal_uint32 wal_hipriv_set_mode(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of wal_linux_ioctl.h */

