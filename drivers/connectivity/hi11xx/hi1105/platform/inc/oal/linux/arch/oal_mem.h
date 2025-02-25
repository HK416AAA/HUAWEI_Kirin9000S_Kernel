/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: oal_mem.c header file
 * Author: @CompanyNameTag
 */

#ifndef OAL_LINUX_MEM_H
#define OAL_LINUX_MEM_H

/* 宏定义 */
#define OAL_TX_CB_LEN    48
#define OAL_MAX_CB_LEN   19
#define MAX_MAC_HEAD_LEN WLAN_MAX_MAC_HDR_LEN

/* 对于enhanced类型的申请接口与释放接口，每一个内存块都包含一个4字节的头部， */
/* 用来指向内存块管理结构体oal_mem_struc，整个内存块的结构如下所示。           */
/*                                                                           */
/* +-------------------+------------------------------------------+---------+ */
/* | oal_mem_stru addr |                    payload               | dog tag | */
/* +-------------------+------------------------------------------+---------+ */
/* |      4/8 byte       |                                          | 4 byte  | */
/* +-------------------+------------------------------------------+---------+ */
#ifdef CONFIG_ARM64
/* when 64 os, c pointer take 8 bytes */
#define OAL_MEM_INFO_SIZE 8
#else
#define OAL_MEM_INFO_SIZE 4
#endif

#define OAL_IORESOURCE_IO  IORESOURCE_IO
#define OAL_IORESOURCE_MEM IORESOURCE_MEM
#ifdef IORESOURCE_REG
#define OAL_IORESOURCE_REG IORESOURCE_REG
#else
#define OAL_IORESOURCE_REG 0x00000300
#endif

#endif /* end of oal_mm.h */
