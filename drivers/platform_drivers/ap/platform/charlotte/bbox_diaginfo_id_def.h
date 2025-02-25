#ifndef __BBOX_DIAGINFO_ID_DEF_H__
#define __BBOX_DIAGINFO_ID_DEF_H__ 
enum bbox_diaginfo_errid {
 SoC_DIAGINFO_START=925200000,
 L3_ECC_CE = SoC_DIAGINFO_START,
 CPU_UP_FAIL,
 CPU_PANIC_INFO,
 NOC_FAULT_INFO,
 L3_ECC_UE = 925201200,
 CPU0_L1_ECC_CE,
 CPU1_L1_ECC_CE,
 CPU2_L1_ECC_CE,
 CPU3_L1_ECC_CE,
 CPU4_L1_ECC_CE,
 CPU5_L1_ECC_CE,
 CPU6_L1_ECC_CE,
 CPU7_L1_ECC_CE,
 CPU0_L2_ECC_CE,
 CPU1_L2_ECC_CE,
 CPU2_L2_ECC_CE,
 CPU3_L2_ECC_CE,
 CPU4_L2_ECC_CE,
 CPU5_L2_ECC_CE,
 CPU6_L2_ECC_CE,
 CPU7_L2_ECC_CE,
 CPU0_L1_ECC_UE,
 CPU1_L1_ECC_UE,
 CPU2_L1_ECC_UE,
 CPU3_L1_ECC_UE,
 CPU4_L1_ECC_UE,
 CPU5_L1_ECC_UE,
 CPU6_L1_ECC_UE,
 CPU7_L1_ECC_UE,
 CPU0_L2_ECC_UE,
 CPU1_L2_ECC_UE,
 CPU2_L2_ECC_UE,
 CPU3_L2_ECC_UE,
 CPU4_L2_ECC_UE,
 CPU5_L2_ECC_UE,
 CPU6_L2_ECC_UE,
 CPU7_L2_ECC_UE,
 ECC_CE,
 ECC_DE,
 ECC_UE,
 ACPU_LIT_AVS_VAL = 925201300,
 ACPU_MID_AVS_VAL = 925201301,
 ACPU_BIG_AVS_VAL = 925201302,
 ACPU_L3_AVS_VAL = 925201303,
 IOMCU_DIAG_INFO_HW_START = 925201600,
 IGS_DMD_FDUL_PW_ON = IOMCU_DIAG_INFO_HW_START,
 IGS_DMD_FDUL_PW_OFF = 925201601,
 IGS_DMD_HWTS_PW_ON = 925201602,
 IGS_DMD_HWTS_PW_OFF = 925201603,
 IGS_DMD_AIC_PW_ON = 925201604,
 IGS_DMD_AIC_PW_OFF = 925201605,
 IGS_DMD_CAM_PW_ON = 925201608,
 IGS_DMD_CAM_PW_OFF = 925201609,
 IGS_DMD_CAM_IR_PW = 925201610,
 IGS_DMD_CAM_TIMEOUT = 925201611,
 IO_DIE_ERROR_INFO = 925201620,
 IOMCU_DIAG_INFO_HW_END = 925201699,
 IOMCU_DIAG_INFO_SW_START = 925201700,
 IGS_DMD_SLEEP_FUSION = 925201714,
 IOMCU_DIAG_INFO_SW_END = 925201899,
 IOMCU_DIAG_INFO_MONITER_START = 925201900,
 IOMCU_DIAG_INFO_MONITER_END = 925201959,
 IOMCU_DIAG_INFO_RESERVED_START = 925201960,
 IOMCU_DIAG_INFO_RESERVED_END = 925201999,
 DDR_DIAGINFO_START=925202000,
 LPM3_DDR_FAIl = DDR_DIAGINFO_START,
 LPM3_DDR_PA_SENSOR = 925202001,
 LPM3_DDR_TRAINING = 925202002,
 MSPC_SECFLASH_ERR_INFO = 925202800,
 MSPC_SECFLASH_WARN_INFO = 925202801,
 MSPC_DIAG_INFO = 925202802,
 FBE_DIAG_FAIL_ID = 925204500,
 BLOCK_DMD_CP_IO = 925205500,
 BLOCK_DMD_NORMAL_IO = 925205501,
 UFS_MPHY_CALIBRATION_INFO = 925205510,
 IGS_DMD_NPU_PLL_RETRY = 925206302,
 CLOCK_PLL_AP = 925206300,
 CLOCK_PLL_LPMCU = 925206301,
 IO_EXPANDER_ABSENCE = 925206303,
 LOWPOWER_GPU_AVS_VAL = 925206304,
 LPM3_PERI_PA_SENSOR = 925206305,
 HIGPU_DIAG_INFO_HW_START = 925212800,
 DMD_HIGPU_BUS_FAULT = HIGPU_DIAG_INFO_HW_START,
 DMD_HIGPU_BIT_STUCK = 925212801,
 DMD_HIGPU_PW_ONOFF_FAIL = 925212802,
 DMD_HIGPU_FCP_LOAD_FAIL = 925212803,
 HIGPU_DIAG_INFO_HW_END = 925212899,
 HIGPU_DIAG_INFO_SW_START = 925212900,
 DMD_HIGPU_JOB_FAIL = HIGPU_DIAG_INFO_SW_START,
 DMD_HIGPU_JOB_HANG = 925212901,
 DMD_HIGPU_ZAP_FAULT = 925212902,
 DMD_HIGPU_HARD_RESET = 925212903,
 DMD_HIGPU_PAGE_FAULT = 925212904,
 DMD_HIGPU_SH_INFO = 925212905,
 HIGPU_DIAG_INFO_SW_END = 925212999,
 HISP_DIAG_INFO_HW_START = 925213100,
 DMD_HISP_CPHY_HISI_MIPI_ERR = HISP_DIAG_INFO_HW_START,
 HISP_DIAG_INFO_HW_END = 925213199,
 BBOX_DIAGINFO_END = 925299999
};
enum bbox_diaginfo_module {
 SoC_AP = 1,
 DSS,
 DDR,
 FDUL,
 HIGPU,
};
enum bbox_diaginfo_level {
 CRITICAL = 1,
 WARNING,
 INFO,
};
enum bbox_diaginfo_type {
 HW = 1,
 SW,
};
#endif
