

#ifndef __OAL_MAIN_H__
#define __OAL_MAIN_H__

/* 其他头文件包含 */
#include "oal_ext_if.h"
#include "oal_types.h"

/* 宏定义 */
#define WLAN_INIT_DEVICE_RADIO_CAP 3 /* WLAN使能2G_5G */
#define NARROW_BAND_ON_MASK        0x1
#define NBFH_ON_MASK               0x2

/* 全局变量声明 */
extern oal_uint8 g_wlan_service_device_per_chip[WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP];
extern oal_uint8 g_hitalk_status; /* 记录当前固件类型 */
#ifdef _PRE_WLAN_NARROW_BAND
extern oal_uint8 g_narrow_bw;
#endif
extern struct genl_family *g_nl80211_fam;
extern struct genl_multicast_group *g_nl80211_mlme_mcgrp;

/* 函数声明 */
extern oal_int32 oal_main_init(oal_void);
extern oal_void oal_main_exit(oal_void);
extern oal_uint8 oal_chip_get_device_num(oal_uint32 ul_chip_ver);
extern oal_uint8 oal_board_get_service_vap_start_id(oal_void);

#endif /* end of oal_main */
