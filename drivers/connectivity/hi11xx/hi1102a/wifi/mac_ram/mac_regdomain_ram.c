


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_regdomain.h"
#include "mac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_REGDOMAIN_RAM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_CONST mac_supp_mode_table_stru   g_bw_mode_table_2g[] = {
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } }, /* 1  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } }, /* 2  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } }, /* 3  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } }, /* 4  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } }, /* 5  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } }, /* 6  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } }, /* 7  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } }, /* 8  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } }, /* 9  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } }, /* 10  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } }, /* 11  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } }, /* 12  */
    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } }, /* 13  */
    { 1, { WLAN_BAND_WIDTH_20M } }, /* 14  */
};
// see http://support.huawei.com/ecommunity/bbs/10212257.html
OAL_CONST mac_supp_mode_table_stru   g_bw_mode_table_5g[] = {
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } }, /* 36  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } }, /* 40  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } }, /* 44  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 48  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } }, /* 52  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } }, /* 56  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } }, /* 60  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 64  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } }, /* 100  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } }, /* 104  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } }, /* 108  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 112  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } }, /* 116  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } }, /* 120  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } }, /* 124  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 128  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS} }, /* 132  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS} }, /* 136  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS} }, /* 140  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 144  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } }, /* 149  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } }, /* 153  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } }, /* 157  */
    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} }, /* 161  */
    { 1, { WLAN_BAND_WIDTH_20M } }, /* 165  */
};

/*****************************************************************************
  3 函数实现
*****************************************************************************/
/*lint -save -e662 */

oal_uint8   mac_regdomain_get_channel_to_bw_mode_idx(oal_uint8 uc_channel_number)
{
    oal_uint8   uc_idx = 0;

    if (uc_channel_number == 0) {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_regdomain_get_channel_to_bw_mode_idx::unknow channel number=%d",
                       uc_channel_number);
        return uc_idx;
    }
    if (uc_channel_number <= 14) { /* 信道号0-14之间 */
        uc_idx =  uc_channel_number - 1;
    } else {
        if (uc_channel_number <= 64) { /* 信道号36-64之间，信道号减36再右移2位得到索引值[0,7] */
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 36) >> 2);
        } else if (uc_channel_number <= 144) { /* 信道号100-144之间，信道号减100再右移2位，最后偏移8得到索引值[8, 19] */
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 100) >> 2) + 8;
        } else if (uc_channel_number <= 165) { /* 信道号149-165之间，信道号减149再右移2位，最后偏移20得到索引值[20, 24] */
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 149) >> 2) + 20;
        } else if (uc_channel_number <= 196) { /* 信道号184-196之间，信道号减184再右移2位，最后偏移25得到索引值[25, 28] */
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 184) >> 2) + 25;
        } else {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,"{mac_regdomain_get_channel_to_bw_mode_idx::unknow channel=%d, \
                force uc_idx = chan 36", uc_channel_number);
            uc_idx =  0;
        }
    }
    return uc_idx;
}


oal_bool_enum mac_regdomain_channel_is_support_bw(wlan_channel_bandwidth_enum_uint8 en_cfg_bw, oal_uint8 uc_channel)
{
    oal_uint8                               uc_idx;
    oal_uint8                               uc_bw_loop;
    mac_supp_mode_table_stru                st_supp_mode_table;
    wlan_channel_band_enum_uint8            en_channel_band;

    if (uc_channel == 0) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_regdomain_channel_is_support_bw::channel not set yet!");
        return OAL_FALSE;
    }

    if (en_cfg_bw == WLAN_BAND_WIDTH_20M) {
        return OAL_TRUE;
    }

    en_channel_band = mac_get_band_by_channel_num(uc_channel);
    /*lint -save -e661 */
    uc_idx = mac_regdomain_get_channel_to_bw_mode_idx(uc_channel);
    if ((en_channel_band == WLAN_BAND_2G) &&
        (uc_idx < OAL_SIZEOF(g_bw_mode_table_2g) / OAL_SIZEOF(g_bw_mode_table_2g[0]))) {
        st_supp_mode_table = g_bw_mode_table_2g[uc_idx];
    } else if ((en_channel_band == WLAN_BAND_5G) &&
              (uc_idx < OAL_SIZEOF(g_bw_mode_table_5g) / OAL_SIZEOF(g_bw_mode_table_5g[0]))) {
        st_supp_mode_table = g_bw_mode_table_5g[uc_idx];
    } else {
        return OAL_FALSE;
    }

    for (uc_bw_loop = 0; uc_bw_loop < st_supp_mode_table.uc_cnt ; uc_bw_loop++) {
        if (en_cfg_bw == st_supp_mode_table.aen_supp_bw[uc_bw_loop]) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}


wlan_channel_bandwidth_enum_uint8 mac_regdomain_get_bw_by_channel_bw_cap(uint8_t channel,
    wlan_bw_cap_enum_uint8 bw_cap)
{
    uint8_t idx;
    wlan_channel_bandwidth_enum_uint8 channel_bw = WLAN_BAND_WIDTH_20M;
    idx = mac_regdomain_get_channel_to_bw_mode_idx(channel);

    if (channel <= 14) { /* 14为2G最大信道号 */
        if (idx < (sizeof(g_bw_mode_table_2g) / sizeof(g_bw_mode_table_2g[0]))) {
            channel_bw = g_bw_mode_table_2g[idx].aen_supp_bw[bw_cap];
        }
    } else {
        if (idx < (sizeof(g_bw_mode_table_5g) / sizeof(g_bw_mode_table_5g[0]))) {
            channel_bw = g_bw_mode_table_5g[idx].aen_supp_bw[bw_cap];
        }
    }
    return channel_bw;
}
