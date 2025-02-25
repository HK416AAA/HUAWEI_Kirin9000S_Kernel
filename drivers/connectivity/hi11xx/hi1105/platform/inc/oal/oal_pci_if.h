/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: oal_pci_if.c header file
 * Author: @CompanyNameTag
 */

#ifndef OAL_PCI_IF_H
#define OAL_PCI_IF_H

/* 其他头文件包含 */
#include "oal_types.h"
#include "oal_util.h"
#include "arch/oal_pci_if.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_PCI_IF_H

/*
 * 枚举名  : oal_pci_bar_idx_enum_uint8
 * 枚举说明:
 */
typedef enum {
    OAL_PCI_BAR_0 = 0,
    OAL_PCI_BAR_1,
    OAL_PCI_BAR_2,
    OAL_PCI_BAR_3,
    OAL_PCI_BAR_4,
    OAL_PCI_BAR_5,

    OAL_PCI_BAR_BUTT
} oal_pci_bar_idx_enum;
typedef uint8_t oal_pci_bar_idx_enum_uint8;

typedef enum {
    OAL_PCI_GEN_1_0 = 0,
    OAL_PCI_GEN_2_0,
    OAL_PCI_GEN_3_0,

    OAL_PCI_GEN_BUTT
} oal_pci_gen_enum;
typedef uint8_t oal_pci_gen_enum_uint8;

#endif /* end of oal_pci_if.h */
