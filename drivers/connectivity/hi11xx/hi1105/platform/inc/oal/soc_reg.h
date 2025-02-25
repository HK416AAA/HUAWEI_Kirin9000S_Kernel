/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description:soc_reg.h header file
 * Author: @CompanyNameTag
 */

#ifndef SOC_REG_H
#define SOC_REG_H
#include "oal_hardware.h"
#define hreg_set_ubits(type, varname, baseaddr, value) \
    do {                                               \
        type hreg = { 0 };                             \
        hreg.bits.varname = value;                     \
        oal_writel(hreg.as_dword, baseaddr);           \
    } while (0)
#ifdef _PRE_WINDOWS_SUPPORT
#define hreg_set_val(regname, baseaddr) \
    oal_writel(regname.as_dword, ((uintptr_t)baseaddr))
#define hreg_get_val(regname, baseaddr) \
    regname.as_dword = oal_readl(((uintptr_t)baseaddr))
#else
#define hreg_set_val(regname, baseaddr) \
    oal_writel((regname).as_dword, ((void*)(uintptr_t)(baseaddr)))
#define hreg_get_val(regname, baseaddr) \
    (regname).as_dword = oal_readl(((void*)(uintptr_t)(baseaddr)))
#endif
#endif
