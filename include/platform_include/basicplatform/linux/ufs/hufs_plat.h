/*
 * hufs_plat.h
 *
 * basic macro and data struct for hufs
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef UFS_HUFS_PLAT_H_
#define UFS_HUFS_PLAT_H_

#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <pmic_interface.h>
#include <linux/platform_drivers/lpcpu_idle_sleep.h>
#include "scsi/ufs/ufshcd.h"
#include "ufs_vendor_cmd.h"

#define UFS_TX_EQUALIZER_35DB

#define UFS_I2C_SLAVE 0x5A
#define UFS_I2C_BUS 3

#define HBRN8_POLL_TOUT_MS 1000
#define TEMP_BUFF_LENGTH 13

/* Device ID */
#define DEV_ID 0x00
#define PEER_DEV_ID 0x01
#define PEER_CPORT_ID 0x00
#define TRAFFIC_CLASS 0x00

/*
 * actrl specific define
 */
#define ACTRL_ISO_EN_GROUP0_PERI_OFFSET 0x000
#define ACTRL_BITMSK_NONSEC_CTRL1_OFFSET 0x400
#define ACTRL_AO_MEM_CTRL2_OFFSET 0x814

/* AO CRG register */
#define AO_CRG_SCCLKDIV5                 0x264
#define AO_CRG_SCPEREN6                  0x1D0
#define AO_CRG_SCPERDIS6                 0x1D4
#define AO_CRG_SCPERRSTEN1               0x20C
#define AO_CRG_SCPERRSTDIS1              0x210
#define AO_CRG_SCPERRSTSTAT1             0x214

/*
 * sctrl specific define
 */
#define SCDEEPSLEEPED_OFFSET 0x008
#define MASK_UFS_MPHY_RHOLD (0x1 << (11 + 16))
#define BIT_UFS_MPHY_RHOLD (0x1 << 11)
#define EFUSE_RHOLD_BIT (0x1 << 22)

#define BIT_SYS_UFS_CLK_TYPE_OFF 0x6
#define BIT_SYS_UFS_CLK_TYPE_MASK (0x3 << BIT_SYS_UFS_CLK_TYPE_OFF)

#define SCTRL_SCEFUSEDATA2_OFFSET 0x00C
#define SCTRL_SCPEREN4_OFFSET 0x1B0
#define SCTRL_SCPERCLKEN4_OFFSET 0x1B8
#define SCTRL_SCPERRSTSTAT1_OFFSET 0x214
#define SCTRL_SCCLKDIV9_OFFSET 0x274
#define SCTRL_SCUFS_AUTODIV_OFFSET 0x278
#define SCTRL_SCPRSTATUS3_OFFSET 0x36C
#define BIT_SCTRL_UFSBUS_DIV_AUTO_REDUCE_BYPASS_LPM3 UFS_BIT(0)
#define BIT_SCTRL_GT_CLK_AUTODIV_SUBSYS_START UFS_BIT(13)
#define BIT_SCTRL_GT_CLK_UFS_SUBSYS_START UFS_BIT(14)
#define BIT_SCTRL_UFS_PSW_MEM_REPAIR_ACK_MASK UFS_BIT(30)

/*
 * pmctrl specific define
 */
#define PPLL3CTRL0_OFFSET 0x048
#define PPLL3CTRL1_OFFSET 0x04C

#define PPLL3_LOCK UFS_BIT(26)
/*
 * pctrl specific define
 */
#define PCTRL_PERI_CTRL3_OFFSET 0x010
#define UFS_TCXO_EN_WITH_MASK (UFS_BIT(0) | UFS_BIT(16))
#define PCTRL_PERI_STAT64_OFFSET 0x18C
#define BIT_UFS_PSW_MEM_REPAIR_ACK_MASK UFS_BIT(21)
/*
 * pericrg specific define
 */
#define PEREN0_OFFSET 0x00
#define PERDIS0_OFFSET 0x04
#define PEREN5_OFFSET 0x050
#define PERDIS5_OFFSET 0x054
#define PERRSTEN0_OFFSET 0x60
#define PERRSTDIS0_OFFSET 0x64
#define PERRSTEN3_OFFSET 0x084
#define PERRSTDIS3_OFFSET 0x088
#define PERRSTSTAT3_OFFSET 0x08C
#define CLKDIV0_OFFSET 0xA8
#define CLKDIV5_OFFSET 0xBC
#define CLKDIV16_OFFSET 0x0E8
#define CLKDIV17_OFFSET 0x0EC
#define CLKDIV21_OFFSET 0x0FC
#define PEREN7_OFFSET 0x420
#define PERDIS7_OFFSET 0x424
#define PERCLKEN7_OFFSET 0x428
#define UFS_ARESET UFS_BIT(7)
#define RST_UFS UFS_BIT(12)
#define CLK_UFSPHY UFS_BIT(12)
#define CLK_UFSIO UFS_BIT(14)
#define CLK_UFS_SUBSYS UFS_BIT(21)
#define CLK_ABB_BACKUP UFS_BIT(22)
/*
 * hsdtcrg specific define
 */
#define SOC_HSDT_CRG_PEREN1 0x010
#define SOC_HSDT_CRG_PERRSTEN0 0x060
#define SOC_HSDT_CRG_PERRSTDIS0 0x064
#define SOC_CRG_GENERAL_SEC_PERCLKEN1 0xE18
#define SOC_CRG_GENERAL_SEC_RSTDIS1 0xD10
#define SOC_CRG_GENERAL_SEC_RSTDIS 0xD04
#define SOC_SCTRL_SCPERRSTEN1 0x20C
#define SOC_PMCTRL_NOC_POWER_IDLEREQ_0 0x380
#define SOC_PMCTRL_NOC_POWER_IDLEACK_0 0x384
#define SOC_PMCTRL_NOC_POWER_IDLE_0 0x388

#ifdef CONFIG_SCSI_UFS_ANTLIA
#define SOC_HSDT_CRG_PERCLKEN4_OFFSET 0x048
#define SOC_HSDT_CRG_PERI_AUTODIV0_OFFSET 0x350
#define UFSBUS_DIV_AUTO_REDUCE_BYPASS_LPM3_START UFS_BIT(0)
#define GT_CLK_AUTODIV_UFS_SUBSYS_START UFS_BIT(13)
#endif

/*
 * hsdtcrg specific define
 */
#define HSDT_SCTRL_UFS_SEL_IN 0x814

#define SYSPMC_PWRD_REQ_HSDT0BUS 3
#define SMMU_TBU_BYPASS_BIT 3
/* clock init */
#define IP_RST_DFA_HOST (0x1 << 13)
#define IP_RST_DFA_POR (0x1 << 12)
#define IP_RST_UFS_DFA_SYS (0x1 << 29)
#define GT_CLK_UFS_DFA_SYS (0x1 << 22)
#define UFS_SELECT_PLL1 0x0C000000
#define UFS_SELECT_PLL0 0x0C000100
#define DFA_SYS_CLK_DIV 0x0FC00080
#define DFA_SYS_CLK_DIV_8 0x0FC00180
#define MPHY_SYS_CLK_DIV 0x001E0006
#define MPHY_SYS_CLK_DIV_8 0x001E000C
#define MPHY_SYS_CLK_EN (0x1 << 0)
#define IP_RST_UFS_CRG (0x1 << 19)


/* USE_HUFS_MPHY_TC */
/* LPMCU: 0xB4000000, A53: 0xF4000000, I2C: 0x0 */
#define BASE_BUS_ADDR 0x0
#define REG_SC_APB_IF (BASE_BUS_ADDR + 0x00023000)
#define REG_SC_APB_IF_V200 (BASE_BUS_ADDR + 0x00046000)
#define SCNOCSTA (REG_SC_APB_IF + 0xa0)
#define NOCSTA (BASE_BUS_ADDR + 0x00026000)
#define SCPPLLCTRL0 (REG_SC_APB_IF + 0x8)
#define SCPPLLCTRL1 (REG_SC_APB_IF + 0xc)
#define SCPPLLSTAT (REG_SC_APB_IF + 0x14)
#define SC_CLKEN (REG_SC_APB_IF + 0x18)
#define SC_RSTEN (REG_SC_APB_IF + 0x30)
#define SC_RSTEN_V200 (REG_SC_APB_IF_V200 + 0x78)
#define SC_RSTDIS (REG_SC_APB_IF + 0x34)
#define SC_RSTDIS_V200 (REG_SC_APB_IF_V200 + 0x7C)
#define SC_RSTSTAT (REG_SC_APB_IF + 0x38)
#define SC_CLK_DIV (REG_SC_APB_IF + 0x84)
#define SC_INT_MASK_EN (REG_SC_APB_IF + 0x70)
#define SC_UFS_REFCLK_RST_PAD (REG_SC_APB_IF + 0x90)

/*
 * ufs sysctrl specific define
 */
#ifdef CONFIG_HUFS_HC
#define UFS_CRG_UFS_CFG 0x00
#define UFS_CRG_UFS_CFG1 0x04
#define UFS_SYS_UFS_RESET_CTRL 0x08
#define UFS_SYS_UFSAXI_W_QOS_LMTR 0x0C
#define UFS_SYS_UFSAXI_R_QOS_LMTR 0x10
#define UFS_SYS_CRG_UFS_STAT 0x14
#define UFS_SYS_MEMORY_CTRL_D1W2R 0x1C
#define UFS_SYS_UFS_CLKRST_BYPASS 0x20
#define UFS_SYS_AO_MEMORY_CTRL 0x24
#define UFS_SYS_MEMORY_CTRL 0x28
#define UFS_SYS_HIUFS_MPHY_CFG 0x2C
#define UFS_SYS_HIUFS_DEBUG 0x30
#define UFS_SYS_MPHYMCU_TEST_POINT 0x34
#define UFS_SYS_UFS_ENHANCE_CTRL 0xF00
#else
#define UFS_SYS_MEMORY_CTRL 0x0
#define PSW_POWER_CTRL 0x04
#define PHY_ISO_EN 0x08
#define HC_LP_CTRL 0x0C
#define PHY_CLK_CTRL 0x10
#define PSW_CLK_CTRL 0x14
#define CLOCK_GATE_BYPASS 0x18
#define RESET_CTRL_EN 0x1C
#define PHY_RESET_STATUS 0x28
#define PHY_MPX_TEST_CTRL 0x30
#define PHY_MPX_TEST_OBSV 0x34
#define UFS_SYS_MK2_CTRL 0x50
#define UFS_SYSCTRL 0x5C
#define UFS_CRG_UFS_CFG 0x7C
#define UFS_APB_ADDR_MASK 0x64
#define UFS_SYS_AXI_W_QOS_LMRT 0x84
#define UFS_SYS_AXI_R_QOS_LMRT 0x88
#define UFS_DEBUG_CTRL 0xAC
#define UFS_PWM_COUNTER 0xB8
#define UFS_IDLE_CONUTER 0xBC
#define UFS_PWM_COUNTER_CURT 0xC4
#define UFS_IDLE_CONUTER_CRUT 0xC8
#define UFS_PWM_COUNTER_CLR 0xD0
#define UFS_PHY_FSM_STATE 0xD8
#define UFS_DEV_TMT_COUNTER 0xDC
#define UFS_DEV_TMT_COUNTER_CURR 0xE0
#define UFS_DEV_TMT_COUNTER_CLR 0xE4
#define UFS_SYS_UFS_POWER_GATING 0xF4
#define UFS_SYS_PHY_SRAM_MEM_CTRL_S 0xEC
#define UFS_SYS_PHY_SRAM_INIT_DONE (1 << 26)
#define UFS_SYS_MEMORY_BP_CTRL 0xF8
#endif
#define UFS_DEVICE_RESET_CTRL 0x60
#define UFS_DEBUG_STAT 0xB0
#define UFS_IDLE_CONUTER_CLR 0xD4

#define BIT_UFS_PSW_ISO_CTRL (1 << 16)
#define BIT_UFS_PSW_MTCMOS_EN (1 << 0)
#define BIT_UFS_REFCLK_ISO_EN (1 << 16)
#define BIT_UFS_PHY_ISO_CTRL (1 << 0)
#define BIT_SYSCTRL_LP_ISOL_EN (1 << 16)
#define BIT_SYSCTRL_LP_PWR_GATE (1 << 0)
#define BIT_SYSCTRL_PWR_READY (1 << 8)
#define BIT_SYSCTRL_REF_CLOCK_EN (1 << 24)
#define MASK_SYSCTRL_REF_CLOCK_SEL (0x3 << 8)
#define SYSCTRL_REF_CLOCK_SEL (0x2 << 8)
#define MASK_SYSCTRL_CFG_CLOCK_FREQ 0xFF
#define UFS_FREQ_CFG_CLK 0x39
#define BIT_SYSCTRL_PSW_CLK_EN (1 << 4)
#define MASK_UFS_CLK_GATE_BYPASS 0x3F
#define BIT_STATUS_LP_RESETCOMPLETE (1 << 0)
#define BIT_SYSCTRL_LP_RESET_N (1 << 0)
#define BIT_UFS_REFCLK_SRC_SEL (1 << 0)
#define MASK_UFS_SYSCRTL_BYPASS (0x3F << 16)
#define MASK_UFS_DEVICE_RESET (0x1 << 16)
#define BIT_UFS_DEVICE_RESET 0x1
#define SRAM_EXT_LD_DONE_BIT (0x1 << 27)
#define PHY_SRAM_BYPASS_BIT (0x1 << 28)
#define BIT_RX_DISABLE_OVR_EN_WR (1 << 4)
#define WDP_BYPASS_EC_BIT (0x1 << 16)
#define ULPH8_BYPASS_EC_BIT (0x1 << 0)
#define OVERALL_BYPASS_EC_BIT (0x1 << 1)
#define ENCRYPT_BYPASS_EC_BIT (0x1 << 16)
#define DENCRYPT_BYPASS_EC_BIT (0x1 << 0)
#define UFS_MEM_CTRL_MASK 0xFFFF
#define UFS_MEM_CTRL_VAL 0x850
#define UFS_BP_MEM_CTRL_MASK 0xFFFF
#define UFS_BP_MEM_CTRL_VAL 0x4858
#define SOC_SCTRL_SCPERRSTDIS1 0x210
/* UFS_DEVICE_RESET_CTRL different addr on Pisces */
#define UFS_RESET_CTRL 0x08
#define PHY_START 0xC000
/* UFS_DEBUG_CTRL */
#define UFS_DEBUG_CTRL_DEFAULT_MASK                                            \
	(BIT_UFS_PWM_CNT_INT_MASK | BIT_UFS_IDLE_CNT_TIMEOUT_MASK |            \
		BIT_UFS_DEV_TMT_CNT_MASK)
#define BIT_UFS_PWM_CNT_EN (1 << 2)
#define BIT_UFS_PWM_CNT_INT_MASK (1 << 3)
#define BIT_UFS_IDLE_CNT_EN (1 << 4)
#define BIT_UFS_IDLE_CNT_TIMEOUT_MASK (1 << 5)
#define BIT_UFS_DEV_TMT_CNT_EN (1 << 7)
#define BIT_UFS_DEV_TMT_CNT_MASK (1 << 8)

/* UFS_DEBUG_STAT */
#define BIT_UFS_PWM_INTR_RAW (1 << 2)
#define BIT_UFS_PWR_INTR (1 << 3)
#define BIT_UFS_IDLE_TIMEOUT (1 << 4)
#define BIT_UFS_IDLE_CNT_TIMEOUT_CLR (1 << 0)
#define BIT_UFS_DEV_TMT_INTR_RAW (1 << 9)
#define BIT_UFS_DEV_TMT_INTR (1 << 10)

/* UFS_PHY_FSM_STATE */
#define TX0_FSM_STATE_CFG_OFFSET 0
#define TX0_FSM_STATE_CFG_SHIFT 4

#define TX1_FSM_STATE_CFG_OFFSET 4
#define TX1_FSM_STATE_CFG_SHIFT 4

#define RX0_FSM_STATE_ATTR_OFFSET 8
#define RX0_FSM_STATE_ATTR_SHIFT 4

#define RX1_FSM_STATE_ATTR_OFFSET 12
#define RX1_FSM_STATE_ATTR_SHIFT 4

#define UFS_PHY_STATE_DISABLED 0
#define UFS_PHY_STATE_HIBERN8 1
#define UFS_PHY_STATE_SLEEP 2
#define UFS_PHY_STATE_STALL 3
#define UFS_PHY_STATE_LSBURST 4
#define UFS_PHY_STATE_HSBURST 5
#define UFS_PHY_STATE_LINECFG 6
#define UFS_PHY_STATE_LINERESET 7

enum {
	TX_EQUALIZER_0DB = 0,
	TX_EQUALIZER_35DB = 35,
	TX_EQUALIZER_60DB = 60,
};

/*
 * HUFS HC specific Registers
 */
enum {
	UFS_REG_OCPTHRTL = 0xc0,
	UFS_REG_OOCPR = 0xc4,
	UFS_REG_CDACFG = 0xd0,
	UFS_REG_CDATX1 = 0xd4,
	UFS_REG_CDATX2 = 0xd8,
	UFS_REG_CDARX1 = 0xdc,
	UFS_REG_CDARX2 = 0xe0,
	UFS_REG_CDASTA = 0xe4,
	UFS_REG_LBMCFG = 0xf0,
	UFS_REG_LBMSTA = 0xf4,
	UFS_REG_UFSMODE = 0xf8,
	UFS_REG_HCLKDIV = 0xfc,
};

/*
 * HUFS SYSCTRL Register
 */
enum {
	UFSSYS_MEMORY_CTRL = 0x0000,
	UFSSYS_PSW_POWER_CTRL = 0x0004,
	UFSSYS_PHY_ISO_EN = 0x0008,
	UFSSYS_HC_LP_CTRL = 0x000C,
	UFSSYS_PHY_CLK_CTRL = 0x0010,
	UFSSYS_PSW_CLK_CTRL = 0x0014,
	UFSSYS_CLOCK_GATE_BYPASS = 0x0018,
	UFSSYS_RESET_CTRL_EN = 0x001C,
	UFSSYS_PHY_RESET_STATUS = 0x0028,
	UFSSYS_HC_DEBUG = 0x002C,
	UFSSYS_PHY_MPX_TEST_CTRL = 0x0030,
	UFSSYS_PHY_MPX_TEST_OBSV = 0x0034,
	UFSSYS_PHY_DTB_OUT = 0x0038,
	UFSSYS_DEBUG_MONITOR_HH = 0x003C,
	UFSSYS_DEBUG_MONITOR_H = 0x0040,
	UFSSYS_DEBUG_MONITOR_L = 0x0044,
	UFSSYS_DEBUG_MONITORUP_H = 0x0048,
	UFSSYS_DEBUG_MONITORUP_L = 0x004C,
	UFSSYS_MK2_CTRL = 0x0050,
	UFSSYS_UFS_SYSCTRL = 0x005C,
	UFSSYS_UFS_RESET_CTRL = 0x0060,
	UFSSYS_UFS_UMECTRL = 0x0064,
	UFSSYS_DEBUG_MONITOR_UME_HH = 0x0068,
	UFSSYS_DEBUG_MONITOR_UME_H = 0x006C,
	UFSSYS_DEBUG_MONITOR_UME_L = 0x0070,
	UFSSYS_UFS_MEM_CLK_GATE_BYPASS = 0x0074,
	UFSSYS_CRG_UFS_CFG = 0x007C,
	UFSSYS_CRG_UFS_CFG1 = 0x0080,
	UFSSYS_UFSAXI_W_QOS_LMTR = 0x0084,
	UFSSYS_UFSAXI_R_QOS_LMTR = 0x0088,
	UFSSYS_CRG_UFS_STAT = 0x00A8,
	UFSSYS_UFS_DEBUG_CTRL = 0x00AC,
	UFSSYS_UFS_DEBUG_STATE = 0x00B0,
	UFSSYS_PWM_TIMEOUT_TIMER = 0x00B8,
	UFSSYS_PWM_TIMEOUT_CURR_TIMER = 0x00C4,
	UFSSYS_UFS_PHY_STATE = 0x00D8,
};

/*
 * HUFS Crypto Registers
 */
enum {
	UFS_REG_CCAP = 0x100,
	UFS_REG_CRYPTOCAP_0 = 0x104,
	UFS_REG_CRYPTOCAP_1 = 0x108,
	UFS_REG_CRYPTOCFG_0_0 = 0x500,
	UFS_REG_CRYPTOCFG_0_1 = 0x504,
	UFS_REG_CRYPTOCFG_0_2 = 0x508,
	UFS_REG_CRYPTOCFG_0_3 = 0x50c,
	UFS_REG_CRYPTOCFG_0_4 = 0x510,
	UFS_REG_CRYPTOCFG_0_5 = 0x514,
	UFS_REG_CRYPTOCFG_0_6 = 0x518,
	UFS_REG_CRYPTOCFG_0_7 = 0x51c,
	UFS_REG_CRYPTOCFG_0_8 = 0x520,
	UFS_REG_CRYPTOCFG_0_9 = 0x524,
	UFS_REG_CRYPTOCFG_0_10 = 0x528,
	UFS_REG_CRYPTOCFG_0_11 = 0x52c,
	UFS_REG_CRYPTOCFG_0_12 = 0x530,
	UFS_REG_CRYPTOCFG_0_13 = 0x534,
	UFS_REG_CRYPTOCFG_0_14 = 0x538,
	UFS_REG_CRYPTOCFG_0_15 = 0x53c,
	UFS_REG_CRYPTOCFG_0_16 = 0x540,
};

/* Here external BL31 function declaration for UFS inline encrypt */
/* Now it is a test magic number */
#define RPMB_SVC_UFS_TEST 0xc600FF20

/* use #0~29 key index */
#ifdef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
#define MAX_CRYPTO_KEY_INDEX 30
#endif

/* REG UFS_REG_OCPTHRTL definition */
#define LP_PGE UFS_BIT(16)
#define LP_AH8_PGE UFS_BIT(17)
#define UFS_BUS_DELAY_MASK 0xFF
#define UFS_BUS_DELAY_VALUE 0x22

#define UFS_HCLKDIV_NORMAL_VALUE 0xE4
#define UFS_HCLKDIV_FPGA_VALUE 0x28

#define HUFS__LIMIT_NUM_LANES_RX 2
#define HUFS_LIMIT_NUM_LANES_TX 2
#define HUFS_LIMIT_HSGEAR_RX UFS_HS_G1
#define HUFS_LIMIT_HSGEAR_TX UFS_HS_G1
#define HUFS_LIMIT_PWMGEAR_RX UFS_PWM_G4
#define HUFS_LIMIT_PWMGEAR_TX UFS_PWM_G4
#define HUFS_LIMIT_RX_PWR_PWM SLOW_MODE
#define HUFS_LIMIT_TX_PWR_PWM SLOW_MODE
#define HUFS_LIMIT_RX_PWR_HS FASTAUTO_MODE
#define HUFS_LIMIT_TX_PWR_HS FASTAUTO_MODE
#define HUFS_LIMIT_HS_RATE PA_HS_MODE_A
#define HUFS_LIMIT_DESIRED_MODE FAST
/* maximum number of reset retries before giving up */
#define MAX_HOST_RESET_RETRIES 7

#define HUFS_CAP_RESERVED UFS_BIT(0)
#define USE_RATE_B UFS_BIT(1)
#define BROKEN_FASTAUTO UFS_BIT(2)
#define USE_ONE_LANE UFS_BIT(3)
#define BROKEN_CLK_GATE_BYPASS UFS_BIT(5)
#define USE_HUFS_MPHY_TC UFS_BIT(6)
#define USE_HS_GEAR1 UFS_BIT(7)
#define USE_HS_GEAR2 UFS_BIT(8)
#define USE_HS_GEAR3 UFS_BIT(9)
#define USE_HS_GEAR4 UFS_BIT(10)
#define RX_CANNOT_DISABLE UFS_BIT(11)
#define USE_SMSV6_TX_RT_ADAPT UFS_BIT(12)
#define RX_VCO_VREF UFS_BIT(13)
#define USE_MPHY_LDO_VOL_WORKROUND UFS_BIT(14)
#define USE_MPHY_CDR_RESET_WORKROUND UFS_BIT(15)
#define USE_MPHY_AUTO_K_WORKROUND UFS_BIT(16)
#define USE_HUFS_MPHY_ASIC UFS_BIT(17)
#define USE_MPHY_CDR_BW_WORKROUND UFS_BIT(18)
#define USE_MAIN_RESUME UFS_BIT(20)
#define USE_TWO_LANE UFS_BIT(21)
#define USE_HUFS_MPHY_5 UFS_BIT(22)

#define MPHY_BOARDID_V200 0x18903205
#define UFS_BIT_DEVICE_RSTDIS BIT(6)
#define UFS_BIT_OEN_RST BIT(2)
#define UFS_BIT_OEN_REFCLK BIT(10)
#define UFS_BIT_DEVICE_RSTEN BIT(6)
#define TX_LS_PREPARE_LENGTH_OVREN 3
#define PA_STALL_NO_CONFIG_TIME_STNCT 15
#define BIT_ENABLE_MASK 16
#define BIT_1LANE_MASK 0xfffffff7
bool is_v200_mphy(struct ufs_hba *hba);

struct hufs_host {
	struct ufs_hba *hba;

	void __iomem *ufs_sys_ctrl;
	void __iomem *pctrl;
	void __iomem *pericrg;
	void __iomem *pmctrl;
	void __iomem *sysctrl;
	void __iomem *actrl;
	void __iomem *hsdt_crg;
	void __iomem *ao_crg;
	void __iomem *core_ctrl;
	void __iomem *hsdt_ctrl;
	void __iomem *hsdt_sysctrl;
	void __iomem *ufs_mphy_mcu_sctrl;
	void __iomem *ib_ufs_mst;
	void __iomem *ufs_tbu;
	void __iomem *mphy_pcs;
	void __iomem *mphy_pma;

#ifdef CONFIG_SCSI_UFS_INTR_HUB
	void __iomem *hsdt0_intr_hub;
#endif

	struct clk *clk_ufsio_ref;

	uint64_t caps;

	u32 avail_ln_rx;
	u32 avail_ln_tx;

	u32 busthrtl_backup;
	u32 reset_gpio;

	u32 tx_equalizer;

	struct ufs_pa_layer_attr dev_req_params;

	int chipsel_gpio;
	struct i2c_client *i2c_client;

	bool need_memrepair;

	int in_suspend;
	bool hufs_standard_uic;
};

struct st_register_dump {
	uint32_t addr;
	char *name;
};

#define hufs_is_link_off(hba) ufshcd_is_link_off(hba)
#define hufs_is_link_active(hba) ufshcd_is_link_active(hba)
#define hufs_is_link_hibern8(hba) ufshcd_is_link_hibern8(hba)

#define ufs_sys_ctrl_writel(host, val, reg)                                    \
	writel((val), (host)->ufs_sys_ctrl + (reg))
#define ufs_sys_ctrl_readl(host, reg) readl((host)->ufs_sys_ctrl + (reg))
#define ufs_sys_ctrl_set_bits(host, mask, reg)                                 \
	ufs_sys_ctrl_writel(                                                   \
		(host), ((mask) | (ufs_sys_ctrl_readl((host), (reg)))), (reg))
#define ufs_sys_ctrl_clr_bits(host, mask, reg)                                 \
	ufs_sys_ctrl_writel((host),                                            \
		((~(mask)) & (ufs_sys_ctrl_readl((host), (reg)))), (reg))

#define ufs_pctrl_writel(host, val, reg) writel((val), (host)->pctrl + (reg))
#define ufs_pctrl_readl(host, reg) readl((host)->pctrl + (reg))

#define ufs_pericrg_writel(host, val, reg)                                     \
	writel((val), (host)->pericrg + (reg))
#define ufs_pericrg_readl(host, reg) readl((host)->pericrg + (reg))
#define ufs_pericrg_set_bits(host, mask, reg)                                     \
	ufs_pericrg_writel(                                                       \
		(host), ((mask) | (ufs_pericrg_readl((host), (reg)))), (reg))

#define ufs_pmctrl_writel(host, val, reg) writel((val), (host)->pmctrl + (reg))
#define ufs_pmctrl_readl(host, reg) readl((host)->pmctrl + (reg))
#define ufs_pmctrl_ctrl_clr_bits(host, mask, reg)                              \
	ufs_pmctrl_ctrl_writel((host),                                         \
		((~(mask)) & (ufs_pmc_ctrl_readl((host), (reg)))), (reg))

#define ufs_sctrl_writel(host, val, reg) writel((val), (host)->sysctrl + (reg))
#define ufs_sctrl_readl(host, reg) readl((host)->sysctrl + (reg))

#define sys_ctrl_set_bits(host, mask, reg)                                     \
	ufs_sctrl_writel(                                                      \
		(host), ((mask) | (ufs_sctrl_readl((host), (reg)))), (reg))
#define sys_ctrl_clr_bits(host, mask, reg)                                     \
	ufs_sctrl_writel(                                                      \
		(host), ((~(mask)) & (ufs_sctrl_readl((host), (reg)))), (reg))

#define ufs_actrl_writel(host, val, reg) writel((val), (host)->actrl + (reg))
#define ufs_actrl_readl(host, reg) readl((host)->actrl + (reg))

#define hsdt_crg_writel(host, val, reg) writel((val), (host)->hsdt_crg + (reg))
#define hsdt_crg_readl(host, reg) readl((host)->hsdt_crg + (reg))
#define hsdt_crg_set_bits(host, mask, reg)                                     \
	hsdt_crg_writel(                                                       \
		(host), ((mask) | (hsdt_crg_readl((host), (reg)))), (reg))

#define core_ctrl_writel(host, val, reg) writel((val), (host)->core_ctrl + (reg))
#define core_ctrl_readl(host, reg) readl((host)->core_ctrl + (reg))
#define core_ctrl_set_bits(host, mask, reg)                                     \
	core_ctrl_writel(                                                       \
		(host), ((mask) | (core_ctrl_readl((host), (reg)))), (reg))

#define ufs_mphy_mcu_writel(host, val, reg) writel((val), (host)->ufs_mphy_mcu_sctrl + (reg))
#define ufs_mphy_mcu_readl(host, reg) readl((host)->ufs_mphy_mcu_sctrl + (reg))

#define ib_ufs_mst_writel(host, val, reg) writel((val), (host)->ib_ufs_mst + (reg))
#define ib_ufs_mst_readl(host, reg) readl((host)->ib_ufs_mst + (reg))

#define mphy_pcs_writel(host, val, reg) writel((val), (host)->mphy_pcs + (reg))
#define mphy_pcs_readl(host, reg) readl((host)->mphy_pcs + (reg))

#define mphy_pma_writel(host, val, reg) writel((val), (host)->mphy_pma + (reg))
#define mphy_pma_readl(host, reg) readl((host)->mphy_pma + (reg))

#define set_bits(mask, reg) writel(((mask) | (readl(reg))), (reg))
#define clr_bits(mask, reg) writel(((~(mask)) & (readl(reg))), (reg))

/* get limit information from dts */
struct hufs_dev_params {
	u32 pwm_rx_gear; /* pwm rx gear to work in */
	u32 pwm_tx_gear; /* pwm tx gear to work in */
	u32 hs_rx_gear;  /* hs rx gear to work in */
	u32 hs_tx_gear;  /* hs tx gear to work in */
	u32 rx_lanes;    /* number of rx lanes */
	u32 tx_lanes;    /* number of tx lanes */
	u32 rx_pwr_pwm;  /* rx pwm working pwr */
	u32 tx_pwr_pwm;  /* tx pwm working pwr */
	u32 rx_pwr_hs;   /* rx hs working pwr */
	u32 tx_pwr_hs;   /* tx hs working pwr */
	u32 hs_rate;     /* rate A/B to work in HS */
	u32 desired_working_mode;
};
#ifndef CONFIG_DFX_DEBUG_FS
#ifdef CONFIG_SCSI_UFSHCD
void delete_ufs_product_name(char *cmdline);
#endif
#endif
extern const struct ufs_hba_variant_ops ufs_hba_hufs_vops;
void sel_equalizer_by_device(struct ufs_hba *hba, u32 *equalizer);
int ufs_i2c_readl(struct ufs_hba *hba, u32 *value, u32 addr);
int ufs_i2c_writel(struct ufs_hba *hba, u32 val, u32 addr);
int create_i2c_client(struct ufs_hba *hba);
void adapt_pll_to_power_mode(struct ufs_hba *hba);
void deemphasis_config(
	struct hufs_host *host, struct ufs_hba *hba,
	struct ufs_pa_layer_attr *dev_req_params);
void i2c_chipsel_gpio_config(struct hufs_host *host, struct device *dev);

void hufs_advertise_quirks(struct ufs_hba *hba);
void hufs_set_pm_lvl(struct ufs_hba *hba);
void hufs_populate_dt(struct device *dev, struct hufs_host *host);
int hufs_get_resource(struct hufs_host *host);
void hufs_regulator_init(struct ufs_hba *hba);
void hufs_inline_crypto_attr(struct ufs_hba *hba);
#ifdef CONFIG_SCSI_UFS_GEAR_CTRL
void hufs_gear_type_sysfs_init(struct ufs_hba *hba);
#endif
#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO
void hufs_uie_key_prepare(struct ufs_hba *hba, int key_index, void *key);
#endif

int hufs_link_startup_pre_change(struct ufs_hba *hba);
int hufs_link_startup_post_change(struct ufs_hba *hba);
void hufs_pwr_change_pre_change(
	struct ufs_hba *hba, struct ufs_pa_layer_attr *dev_req_params);

void hufs_mphy_updata_temp_sqvref(
	struct ufs_hba *hba, struct hufs_host *host);
void hufs_mphy_updata_fsm_ocs5(
	struct ufs_hba *hba, struct hufs_host *host);
void hufs_mphy_v200_updata_fsm(
	struct ufs_hba *hba, struct hufs_host *host);
void hufs_mphy_v200_updata(struct ufs_hba *hba, struct hufs_host *host);
void hufs_mphy_v200_link_post_config(
	struct ufs_hba *hba, struct hufs_host *host);
void hufs_mphy_v200_pwr_change_pre_config(
	struct ufs_hba *hba, struct hufs_host *host);

int hufs_check_hibern8(struct ufs_hba *hba);
void hufs_mphy_write(struct ufs_hba *hba, uint16_t addr, uint16_t value);
int hufs_polling_mphy_write(
	struct ufs_hba *hba, uint16_t addr, uint16_t value);
uint16_t hufs_mphy_read(struct ufs_hba *hba, uint16_t addr);

void hufs_mphy_busdly_config(struct ufs_hba *hba, struct hufs_host *host);
void ufs_clk_init(struct ufs_hba *hba);
void hufs_pwr_change_post_change(struct ufs_hba *hba);
void ufs_soc_init(struct ufs_hba *hba);
void hufs_full_reset(struct ufs_hba *hba);
void set_device_clk(struct ufs_hba *hba);
void hufs_device_hw_reset(struct ufs_hba *hba);
void hufs_set_buck_voltage(struct ufs_hba *hba);
void hufs_mphy_config_update(struct ufs_hba *hba);
int hufs_suspend_before_set_link_state(
	struct ufs_hba *hba, enum ufs_pm_op pm_op);
int hufs_resume_after_set_link_state(
	struct ufs_hba *hba, enum ufs_pm_op pm_op);
int hufs_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op);
int hufs_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op);
int ufs_update_hc_fw(struct ufs_hba *hba);
#ifdef CONFIG_SCSI_UFS_HS_ERROR_RECOVER
int hufs_get_pwr_by_sysctrl(struct ufs_hba *hba);
#endif
#ifdef CONFIG_SCSI_HUFS_LINERESET_CHECK
int hufs_daemon_thread(void *d);
#endif
void hufs_pre_hce_notify(struct ufs_hba *hba);
#ifdef CONFIG_HUFS_HC
int ufs_get_hufs_hc(struct hufs_host *host);
#endif
bool ufshcd_hufs_use_standard_uic(struct ufs_hba *hba);
bool ufshcd_is_hufs_hc(struct ufs_hba *hba);
int ufshcd_adapt_workaround(struct ufs_hba *hba,
			    struct ufs_pa_layer_attr *pwr_mode);
extern const struct of_device_id ufs_of_match[];
#endif /* UFS_HUFS_PLAT_H_ */
