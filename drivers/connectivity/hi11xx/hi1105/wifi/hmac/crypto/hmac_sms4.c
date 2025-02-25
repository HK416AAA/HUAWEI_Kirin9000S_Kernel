/*
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 * 功能说明 : sms4加解密算法
 * 作    者 : wifi
 * 创建日期 : 2015年5月20日
 */
#include "hmac_sms4.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SMS4_C

#define SBOX_LEN    256
#define CK_LEN      32
#define SBOX_NUM    4

#if (defined(_PRE_WLAN_FEATURE_WAPI) || defined(_PRE_WLAN_FEATURE_PWL))
OAL_STATIC uint32_t g_aul_sbox[SBOX_LEN] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

OAL_STATIC uint32_t g_aul_ck[CK_LEN] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)
/*
 * 功能描述 : SMS4算法小端转换函数，根据sms4算法和上下文传入的数组x长度一定是SBOX_NUM，不会出现数组越界操作
 * 日   期 : 2020年8月5日
 * 作   者 : wifi
 * 修改内容 : 新生成函数
 */
OAL_STATIC void hmac_sms4_little_endian_convert(uint32_t *x, uint8_t len)
{
    x[BYTE_OFFSET_0] = rotl(x[BYTE_OFFSET_0], BIT_OFFSET_16);
    x[BYTE_OFFSET_0] = ((x[BYTE_OFFSET_0] & 0x00FF00FF) << BIT_OFFSET_8) ^
                        ((x[BYTE_OFFSET_0] & 0xFF00FF00) >> BIT_OFFSET_8);
    x[BYTE_OFFSET_1] = rotl(x[BYTE_OFFSET_1], BIT_OFFSET_16);
    x[BYTE_OFFSET_1] = ((x[BYTE_OFFSET_1] & 0x00FF00FF) << BIT_OFFSET_8) ^
                        ((x[BYTE_OFFSET_1] & 0xFF00FF00) >> BIT_OFFSET_8);
    x[BYTE_OFFSET_2] = rotl(x[BYTE_OFFSET_2], BIT_OFFSET_16);
    x[BYTE_OFFSET_2] = ((x[BYTE_OFFSET_2] & 0x00FF00FF) << BIT_OFFSET_8) ^
                        ((x[BYTE_OFFSET_2] & 0xFF00FF00) >> BIT_OFFSET_8);
    x[BYTE_OFFSET_3] = rotl(x[BYTE_OFFSET_3], BIT_OFFSET_16);
    x[BYTE_OFFSET_3] = ((x[BYTE_OFFSET_3] & 0x00FF00FF) << BIT_OFFSET_8) ^
                        ((x[BYTE_OFFSET_3] & 0xFF00FF00) >> BIT_OFFSET_8);
    return;
}
#endif
/*
 * 函 数 名 : hmac_sms4_crypt
 * 功能描述 : SMS4的加解密函数
 * 日   期 : 2012年5月3日
 * 作   者 : wifi
 * 修改内容 : 新生成函数
 */
void hmac_sms4_crypt(uint8_t *puc_input, uint8_t *puc_output, uint32_t *puc_rk, uint32_t rk_len)
{
    uint32_t r;
    uint32_t mid;
    uint32_t x[SBOX_NUM];
    uint32_t *p = NULL;

    if (memcpy_s(x, sizeof(x), puc_input, sizeof(x)) != EOK) {
        return;
    }
#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)
    hmac_sms4_little_endian_convert(x, SBOX_NUM);
#endif

    for (r = 0; r < rk_len / sizeof(uint32_t); r += sizeof(uint32_t)) {
        mid = x[BYTE_OFFSET_1] ^ x[BYTE_OFFSET_2] ^ x[BYTE_OFFSET_3] ^ puc_rk[r + BYTE_OFFSET_0];
        mid = bytesub(g_aul_sbox, mid);
        x[BYTE_OFFSET_0] ^= l1(mid);
        mid = x[BYTE_OFFSET_2] ^ x[BYTE_OFFSET_3] ^ x[BYTE_OFFSET_0] ^ puc_rk[r + BYTE_OFFSET_1];
        mid = bytesub(g_aul_sbox, mid);
        x[BYTE_OFFSET_1] ^= l1(mid);
        mid = x[BYTE_OFFSET_3] ^ x[BYTE_OFFSET_0] ^ x[BYTE_OFFSET_1] ^ puc_rk[r + BYTE_OFFSET_2];
        mid = bytesub(g_aul_sbox, mid);
        x[BYTE_OFFSET_2] ^= l1(mid);
        mid = x[BYTE_OFFSET_0] ^ x[BYTE_OFFSET_1] ^ x[BYTE_OFFSET_2] ^ puc_rk[r + BYTE_OFFSET_3];
        mid = bytesub(g_aul_sbox, mid);
        x[BYTE_OFFSET_3] ^= l1(mid);
    }

#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)
    hmac_sms4_little_endian_convert(x, SBOX_NUM);
#endif

    p = (uint32_t *)puc_output;
    p[BYTE_OFFSET_0] = x[BYTE_OFFSET_3];
    p[BYTE_OFFSET_1] = x[BYTE_OFFSET_2];
    p[BYTE_OFFSET_2] = x[BYTE_OFFSET_1];
    p[BYTE_OFFSET_3] = x[BYTE_OFFSET_0];

    return;
}

/*
 * 函 数 名 : hmac_sms4_keyext
 * 功能描述 : SMS4的密钥扩展算法
 * 日   期 : 2012年5月3日
 * 作   者 : wifi
 * 修改内容 : 新生成函数
 */
void hmac_sms4_keyext(uint8_t *puc_key, uint32_t *puc_rk, uint32_t rk_len)
{
    uint32_t r;
    uint32_t mid;
    uint32_t x[SBOX_NUM];

    if (memcpy_s(x, sizeof(x), puc_key, sizeof(x)) != EOK) {
        return;
    } /* 注意输入参数地址对齐问题 */
#if (_PRE_LITTLE_CPU_ENDIAN == _PRE_CPU_ENDIAN)
    hmac_sms4_little_endian_convert(x, SBOX_NUM);
#endif

    x[BYTE_OFFSET_0] ^= 0xa3b1bac6;
    x[BYTE_OFFSET_1] ^= 0x56aa3350;
    x[BYTE_OFFSET_2] ^= 0x677d9197;
    x[BYTE_OFFSET_3] ^= 0xb27022dc;

    for (r = 0; r < rk_len / sizeof(uint32_t); r += sizeof(uint32_t)) {
        mid = x[BYTE_OFFSET_1] ^ x[BYTE_OFFSET_2] ^ x[BYTE_OFFSET_3] ^ g_aul_ck[r + BYTE_OFFSET_0];
        mid = bytesub(g_aul_sbox, mid);
        puc_rk[r + BYTE_OFFSET_0] = x[BYTE_OFFSET_0] ^= l2(mid);
        mid = x[BYTE_OFFSET_2] ^ x[BYTE_OFFSET_3] ^ x[BYTE_OFFSET_0] ^ g_aul_ck[r + BYTE_OFFSET_1];
        mid = bytesub(g_aul_sbox, mid);
        puc_rk[r + BYTE_OFFSET_1] = x[BYTE_OFFSET_1] ^= l2(mid);
        mid = x[BYTE_OFFSET_3] ^ x[BYTE_OFFSET_0] ^ x[BYTE_OFFSET_1] ^ g_aul_ck[r + BYTE_OFFSET_2];
        mid = bytesub(g_aul_sbox, mid);
        puc_rk[r + BYTE_OFFSET_2] = x[BYTE_OFFSET_2] ^= l2(mid);
        mid = x[BYTE_OFFSET_0] ^ x[BYTE_OFFSET_1] ^ x[BYTE_OFFSET_2] ^ g_aul_ck[r + BYTE_OFFSET_3];
        mid = bytesub(g_aul_sbox, mid);
        puc_rk[r + BYTE_OFFSET_3] = x[BYTE_OFFSET_3] ^= l2(mid);
    }
}

#endif /* #ifdef _PRE_WLAN_FEATURE_WAPI */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
