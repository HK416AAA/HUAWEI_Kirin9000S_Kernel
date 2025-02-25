/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Universal Flash Storage Host controller driver
 * Copyright (C) 2011-2013 Samsung India Software Operations
 *
 * Authors:
 *	Santosh Yaraganavi <santosh.sy@samsung.com>
 *	Vinayak Holikatti <h.vinayak@samsung.com>
 */

#ifndef _UFS_H
#define _UFS_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/android_kabi.h>
#include <uapi/scsi/scsi_bsg_ufs.h>
#include "ufs_func.h"

#define MAX_CDB_SIZE UFS_CDB_SIZE

#define GENERAL_UPIU_REQUEST_SIZE (sizeof(struct utp_upiu_req))
#define QUERY_DESC_MAX_SIZE       255
#define QUERY_DESC_MIN_SIZE       2
#define QUERY_DESC_HDR_SIZE       2
#define QUERY_OSF_SIZE            (GENERAL_UPIU_REQUEST_SIZE - \
					(sizeof(struct utp_upiu_header)))
#define UFS_SENSE_SIZE	18

#define UPIU_HEADER_DWORD(byte3, byte2, byte1, byte0)\
			cpu_to_be32((byte3 << 24) | (byte2 << 16) |\
			 (byte1 << 8) | (byte0))
/*
 * UFS device may have standard LUs and LUN id could be from 0x00 to
 * 0x7F. Standard LUs use "Peripheral Device Addressing Format".
 * UFS device may also have the Well Known LUs (also referred as W-LU)
 * which again could be from 0x00 to 0x7F. For W-LUs, device only use
 * the "Extended Addressing Format" which means the W-LUNs would be
 * from 0xc100 (SCSI_W_LUN_BASE) onwards.
 * This means max. LUN number reported from UFS device could be 0xC17F.
 */
#define UFS_UPIU_MAX_UNIT_NUM_ID	0x7F
#define UFS_MAX_LUNS		(SCSI_W_LUN_BASE + UFS_UPIU_MAX_UNIT_NUM_ID)
#define UFS_UPIU_WLUN_ID	(1 << 7)
#define UFS_RPMB_UNIT		0xC4
#define UFS_UPIU_MAX_GENERAL_LUN	8

/* WriteBooster buffer is available only for the logical unit from 0 to 7 */
#define UFS_UPIU_MAX_WB_LUN_ID	8

/* IOCTL opcode for command - ufs set device read only */
#define UFS_IOCTL_BLKROSET      BLKROSET

/* JEDEC UFS Spec version */
enum {
	UFS_DEVICE_SPEC_1_1 = 0x0110,
	UFS_DEVICE_SPEC_2_0 = 0x0200,
	UFS_DEVICE_SPEC_2_1 = 0x0210,
	UFS_DEVICE_SPEC_2_2 = 0x0220,
	UFS_DEVICE_SPEC_3_0 = 0x0300,
	UFS_DEVICE_SPEC_3_1 = 0x0310,
};

/* Well known logical unit id in LUN field of UPIU */
enum {
	UFS_UPIU_REPORT_LUNS_WLUN	= 0x81,
	UFS_UPIU_UFS_DEVICE_WLUN	= 0xD0,
	UFS_UPIU_BOOT_WLUN		= 0xB0,
	UFS_UPIU_RPMB_WLUN		= 0xC4,
};

/* Interconnect descriptor parameters offsets in bytes*/
enum interconnect_desc_param {
	INTERCONNECT_DESC_PARAM_LEN		= 0x0,
	INTERCONNECT_DESC_PARAM_TYPE		= 0x1,
	INTERCONNECT_DESC_PARAM_UNIPRO_VER	= 0x2,
	INTERCONNECT_DESC_PARAM_MPHY_VER	= 0x4,
};

/* Geometry descriptor parameters offsets in bytes*/
enum geometry_desc_param {
	GEOMETRY_DESC_PARAM_LEN			= 0x0,
	GEOMETRY_DESC_PARAM_TYPE		= 0x1,
	GEOMETRY_DESC_PARAM_DEV_CAP		= 0x4,
	GEOMETRY_DESC_PARAM_MAX_NUM_LUN		= 0xC,
	GEOMETRY_DESC_PARAM_SEG_SIZE		= 0xD,
	GEOMETRY_DESC_PARAM_ALLOC_UNIT_SIZE	= 0x11,
	GEOMETRY_DESC_PARAM_MIN_BLK_SIZE	= 0x12,
	GEOMETRY_DESC_PARAM_OPT_RD_BLK_SIZE	= 0x13,
	GEOMETRY_DESC_PARAM_OPT_WR_BLK_SIZE	= 0x14,
	GEOMETRY_DESC_PARAM_MAX_IN_BUF_SIZE	= 0x15,
	GEOMETRY_DESC_PARAM_MAX_OUT_BUF_SIZE	= 0x16,
	GEOMETRY_DESC_PARAM_RPMB_RW_SIZE	= 0x17,
	GEOMETRY_DESC_PARAM_DYN_CAP_RSRC_PLC	= 0x18,
	GEOMETRY_DESC_PARAM_DATA_ORDER		= 0x19,
	GEOMETRY_DESC_PARAM_MAX_NUM_CTX		= 0x1A,
	GEOMETRY_DESC_PARAM_TAG_UNIT_SIZE	= 0x1B,
	GEOMETRY_DESC_PARAM_TAG_RSRC_SIZE	= 0x1C,
	GEOMETRY_DESC_PARAM_SEC_RM_TYPES	= 0x1D,
	GEOMETRY_DESC_PARAM_MEM_TYPES		= 0x1E,
	GEOMETRY_DESC_PARAM_SCM_MAX_NUM_UNITS	= 0x20,
	GEOMETRY_DESC_PARAM_SCM_CAP_ADJ_FCTR	= 0x24,
	GEOMETRY_DESC_PARAM_NPM_MAX_NUM_UNITS	= 0x26,
	GEOMETRY_DESC_PARAM_NPM_CAP_ADJ_FCTR	= 0x2A,
	GEOMETRY_DESC_PARAM_ENM1_MAX_NUM_UNITS	= 0x2C,
	GEOMETRY_DESC_PARAM_ENM1_CAP_ADJ_FCTR	= 0x30,
	GEOMETRY_DESC_PARAM_ENM2_MAX_NUM_UNITS	= 0x32,
	GEOMETRY_DESC_PARAM_ENM2_CAP_ADJ_FCTR	= 0x36,
	GEOMETRY_DESC_PARAM_ENM3_MAX_NUM_UNITS	= 0x38,
	GEOMETRY_DESC_PARAM_ENM3_CAP_ADJ_FCTR	= 0x3C,
	GEOMETRY_DESC_PARAM_ENM4_MAX_NUM_UNITS	= 0x3E,
	GEOMETRY_DESC_PARAM_ENM4_CAP_ADJ_FCTR	= 0x42,
	GEOMETRY_DESC_PARAM_OPT_LOG_BLK_SIZE	= 0x44,
	GEOMETRY_DESC_PARAM_HPB_REGION_SIZE	= 0x48,
	GEOMETRY_DESC_PARAM_HPB_NUMBER_LU	= 0x49,
	GEOMETRY_DESC_PARAM_HPB_SUBREGION_SIZE	= 0x4A,
	GEOMETRY_DESC_PARAM_HPB_MAX_ACTIVE_REGS	= 0x4B,
	GEOMETRY_DESC_PARAM_WB_MAX_ALLOC_UNITS	= 0x4F,
	GEOMETRY_DESC_PARAM_WB_MAX_WB_LUNS	= 0x53,
	GEOMETRY_DESC_PARAM_WB_BUFF_CAP_ADJ	= 0x54,
	GEOMETRY_DESC_PARAM_WB_SUP_RED_TYPE	= 0x55,
	GEOMETRY_DESC_PARAM_WB_SUP_WB_TYPE	= 0x56,
};

/* Health descriptor parameters offsets in bytes*/
enum health_desc_param {
	HEALTH_DESC_PARAM_LEN			= 0x0,
	HEALTH_DESC_PARAM_TYPE			= 0x1,
	HEALTH_DESC_PARAM_EOL_INFO		= 0x2,
	HEALTH_DESC_PARAM_LIFE_TIME_EST_A	= 0x3,
	HEALTH_DESC_PARAM_LIFE_TIME_EST_B	= 0x4,
};

/* WriteBooster buffer mode */
enum {
	WB_BUF_MODE_LU_DEDICATED	= 0x0,
	WB_BUF_MODE_SHARED		= 0x1,
};

/*
 * Logical Unit Write Protect
 * 00h: LU not write protected
 * 01h: LU write protected when fPowerOnWPEn =1
 * 02h: LU permanently write protected when fPermanentWPEn =1
 */
enum ufs_lu_wp_type {
	UFS_LU_NO_WP		= 0x00,
	UFS_LU_POWER_ON_WP	= 0x01,
	UFS_LU_PERM_WP		= 0x02,
};

/* bActiveICCLevel parameter current units */
enum {
	UFSHCD_NANO_AMP		= 0,
	UFSHCD_MICRO_AMP	= 1,
	UFSHCD_MILI_AMP		= 2,
	UFSHCD_AMP		= 3,
};

/* Possible values for dExtendedUFSFeaturesSupport */
enum {
	UFS_DEV_TOO_HIGH_TEMP_SUPPORT	= BIT(4),
	UFS_DEV_HPB_SUPPORT		= BIT(7),
	UFS_DEV_WRITE_BOOSTER_SUP	= BIT(8),
};
#define UFS_DEV_HPB_SUPPORT_VERSION		0x310

#define POWER_DESC_MAX_SIZE			0x62
#define POWER_DESC_MAX_ACTV_ICC_LVLS		16

/* Attribute  bActiveICCLevel parameter bit masks definitions */
#define ATTR_ICC_LVL_UNIT_OFFSET	14
#define ATTR_ICC_LVL_UNIT_MASK		(0x3 << ATTR_ICC_LVL_UNIT_OFFSET)
#define ATTR_ICC_LVL_VALUE_MASK		0x3FF

/* Power descriptor parameters offsets in bytes */
enum power_desc_param_offset {
	PWR_DESC_LEN			= 0x0,
	PWR_DESC_TYPE			= 0x1,
	PWR_DESC_ACTIVE_LVLS_VCC_0	= 0x2,
	PWR_DESC_ACTIVE_LVLS_VCCQ_0	= 0x22,
	PWR_DESC_ACTIVE_LVLS_VCCQ2_0	= 0x42,
};

/* Exception event mask values */
enum {
	MASK_EE_STATUS		= 0xFFFF,
	MASK_EE_URGENT_BKOPS	= (1 << 2),
	MASK_EE_TOO_HIGH_TEMP = (1 << 3),
	MASK_EE_WRITEBOOSTER_EVENT = (1 << 5),
	MASK_EE_BAD_BLOCK_OCCUR	= (1 << 14),
};

/* Background operation status */
enum bkops_status {
	BKOPS_STATUS_NO_OP               = 0x0,
	BKOPS_STATUS_NON_CRITICAL        = 0x1,
	BKOPS_STATUS_PERF_IMPACT         = 0x2,
	BKOPS_STATUS_CRITICAL            = 0x3,
	BKOPS_STATUS_MAX		 = BKOPS_STATUS_CRITICAL,
};

/* UTP QUERY Transaction Specific Fields OpCode */
enum query_opcode {
	UPIU_QUERY_OPCODE_NOP		= 0x0,
	UPIU_QUERY_OPCODE_READ_DESC	= 0x1,
	UPIU_QUERY_OPCODE_WRITE_DESC	= 0x2,
	UPIU_QUERY_OPCODE_READ_ATTR	= 0x3,
	UPIU_QUERY_OPCODE_WRITE_ATTR	= 0x4,
	UPIU_QUERY_OPCODE_READ_FLAG	= 0x5,
	UPIU_QUERY_OPCODE_SET_FLAG	= 0x6,
	UPIU_QUERY_OPCODE_CLEAR_FLAG	= 0x7,
	UPIU_QUERY_OPCODE_TOGGLE_FLAG	= 0x8,
#ifdef CONFIG_HUFS_MANUAL_BKOPS
	/* Hi1861 specific manual GC OPCODE */
	UPIU_QUERY_OPCODE_M_GC_START = 0xF0,
	UPIU_QUERY_OPCODE_M_GC_STOP = 0xF1,
	UPIU_QUERY_OPCODE_M_GC_CHECK = 0xF2,
#endif /* CONFIG_HUFS_MANUAL_BKOPS */
	UPIU_QUERY_OPCODE_READ_HI1861_FSR = 0xF3, /* for hi1861 */
	UPIU_QUERY_OPCODE_VENDOR_READ = 0xF8,
	UPIU_QUERY_OPCODE_VENDOR_WRITE = 0xF9,
	UPIU_QUERY_OPCODE_TZ_CTRL = 0xFA,
	UPIU_QUERY_OPCODE_READ_TZ_DESC = 0xFB,
	UPIU_QUERY_OPCODE_MAX,
};

/* bRefClkFreq attribute values */
enum ufs_ref_clk_freq {
	REF_CLK_FREQ_19_2_MHZ	= 0,
	REF_CLK_FREQ_26_MHZ	= 1,
	REF_CLK_FREQ_38_4_MHZ	= 2,
	REF_CLK_FREQ_52_MHZ	= 3,
	REF_CLK_FREQ_INVAL	= -1,
};

struct ufs_ref_clk {
	unsigned long freq_hz;
	enum ufs_ref_clk_freq val;
};

/* Query response result code */
enum {
	QUERY_RESULT_SUCCESS                    = 0x00,
	QUERY_RESULT_NOT_READABLE               = 0xF6,
	QUERY_RESULT_NOT_WRITEABLE              = 0xF7,
	QUERY_RESULT_ALREADY_WRITTEN            = 0xF8,
	QUERY_RESULT_INVALID_LENGTH             = 0xF9,
	QUERY_RESULT_INVALID_VALUE              = 0xFA,
	QUERY_RESULT_INVALID_SELECTOR           = 0xFB,
	QUERY_RESULT_INVALID_INDEX              = 0xFC,
	QUERY_RESULT_INVALID_IDN                = 0xFD,
	QUERY_RESULT_INVALID_OPCODE             = 0xFE,
	QUERY_RESULT_GENERAL_FAILURE            = 0xFF,
};

/* UTP Transfer Request Command Type (CT) */
enum {
	UPIU_COMMAND_SET_TYPE_SCSI	= 0x0,
	UPIU_COMMAND_SET_TYPE_UFS	= 0x1,
	UPIU_COMMAND_SET_TYPE_QUERY	= 0x2,
};

/* UTP Transfer Request Command Offset */
#define UPIU_COMMAND_TYPE_OFFSET	28

/* Offset of the response code in the UPIU header */
#define UPIU_RSP_CODE_OFFSET		8

enum {
	MASK_SCSI_STATUS		= 0xFF,
	MASK_TASK_RESPONSE              = 0xFF00,
	MASK_RSP_UPIU_RESULT            = 0xFFFF,
	MASK_QUERY_DATA_SEG_LEN         = 0xFFFF,
	MASK_RSP_UPIU_DATA_SEG_LEN	= 0xFFFF,
	MASK_RSP_EXCEPTION_EVENT        = 0x10000,
	MASK_TM_SERVICE_RESP		= 0xFF,
	MASK_TM_FUNC			= 0xFF,
};

/* Task management service response */
enum {
	UPIU_TASK_MANAGEMENT_FUNC_COMPL		= 0x00,
	UPIU_TASK_MANAGEMENT_FUNC_NOT_SUPPORTED = 0x04,
	UPIU_TASK_MANAGEMENT_FUNC_SUCCEEDED	= 0x08,
	UPIU_TASK_MANAGEMENT_FUNC_FAILED	= 0x05,
	UPIU_INCORRECT_LOGICAL_UNIT_NO		= 0x09,
};

/* UFS device power modes */
enum ufs_dev_pwr_mode {
	UFS_ACTIVE_PWR_MODE	= 1,
	UFS_SLEEP_PWR_MODE	= 2,
	UFS_POWERDOWN_PWR_MODE	= 3,
};

#define UFS_WB_BUF_REMAIN_PERCENT(val) ((val) / 10)

/**
 * struct utp_cmd_rsp - Response UPIU structure
 * @residual_transfer_count: Residual transfer count DW-3
 * @reserved: Reserved double words DW-4 to DW-7
 * @sense_data_len: Sense data length DW-8 U16
 * @sense_data: Sense data field DW-8 to DW-12
 */
struct utp_cmd_rsp {
	__be32 residual_transfer_count;
	__be32 reserved[4];
	__be16 sense_data_len;
	u8 sense_data[UFS_SENSE_SIZE];
};

struct ufshpb_active_field {
	__be16 active_rgn;
	__be16 active_srgn;
};
#define HPB_ACT_FIELD_SIZE 4

/**
 * struct utp_hpb_rsp - Response UPIU structure
 * @residual_transfer_count: Residual transfer count DW-3
 * @reserved1: Reserved double words DW-4 to DW-7
 * @sense_data_len: Sense data length DW-8 U16
 * @desc_type: Descriptor type of sense data
 * @additional_len: Additional length of sense data
 * @hpb_op: HPB operation type
 * @lun: LUN of response UPIU
 * @active_rgn_cnt: Active region count
 * @inactive_rgn_cnt: Inactive region count
 * @hpb_active_field: Recommended to read HPB region and subregion
 * @hpb_inactive_field: To be inactivated HPB region and subregion
 */
struct utp_hpb_rsp {
	__be32 residual_transfer_count;
	__be32 reserved1[4];
	__be16 sense_data_len;
	u8 desc_type;
	u8 additional_len;
	u8 hpb_op;
	u8 lun;
	u8 active_rgn_cnt;
	u8 inactive_rgn_cnt;
	struct ufshpb_active_field hpb_active_field[2];
	__be16 hpb_inactive_field[2];
};
#define UTP_HPB_RSP_SIZE 40

/**
 * struct utp_upiu_rsp - general upiu response structure
 * @header: UPIU header structure DW-0 to DW-2
 * @sr: fields structure for scsi command DW-3 to DW-12
 * @qr: fields structure for query request DW-3 to DW-7
 */
struct utp_upiu_rsp {
	struct utp_upiu_header header;
	union {
		struct utp_cmd_rsp sr;
		struct utp_hpb_rsp hr;
		struct utp_upiu_query qr;
	};
};

/**
 * struct utp_upiu_task_req - Task request UPIU structure
 * @header - UPIU header structure DW0 to DW-2
 * @input_param1: Input parameter 1 DW-3
 * @input_param2: Input parameter 2 DW-4
 * @input_param3: Input parameter 3 DW-5
 * @reserved: Reserved double words DW-6 to DW-7
 */
struct utp_upiu_task_req {
	struct utp_upiu_header header;
	__be32 input_param1;
	__be32 input_param2;
	__be32 input_param3;
	/* reserve 2 32bits field */
	__be32 reserved[2];
};

/**
 * struct utp_upiu_task_rsp - Task Management Response UPIU structure
 * @header: UPIU header structure DW0-DW-2
 * @output_param1: Output parameter 1 DW3
 * @output_param2: Output parameter 2 DW4
 * @reserved: Reserved double words DW-5 to DW-7
 */
struct utp_upiu_task_rsp {
	struct utp_upiu_header header;
	__be32 output_param1;
	__be32 output_param2;
	/* reserve 3 32bits field */
	__be32 reserved[3];
};

/**
 * struct ufs_query_req - parameters for building a query request
 * @query_func: UPIU header query function
 * @upiu_req: the query request data
 */
struct ufs_query_req {
	u8 query_func;
	u8 lun;
	bool has_data;
	struct utp_upiu_query upiu_req;
};

/**
 * struct ufs_query_resp - UPIU QUERY
 * @response: device response code
 * @upiu_res: query response data
 */
struct ufs_query_res {
	u8 response;
	struct utp_upiu_query upiu_res;
};

#define UFS_VREG_VCC_MIN_UV	   2700000 /* uV */
#define UFS_VREG_VCC_MAX_UV	   3600000 /* uV */
#define UFS_VREG_VCC_1P8_MIN_UV    1700000 /* uV */
#define UFS_VREG_VCC_1P8_MAX_UV    1950000 /* uV */
#define UFS_VREG_VCCQ_MIN_UV	   1140000 /* uV */
#define UFS_VREG_VCCQ_MAX_UV	   1260000 /* uV */
#define UFS_VREG_VCCQ2_MIN_UV	   1700000 /* uV */
#define UFS_VREG_VCCQ2_MAX_UV	   1950000 /* uV */

/*
 * VCCQ & VCCQ2 current requirement when UFS device is in sleep state
 * and link is in Hibern8 state.
 */
#define UFS_VREG_LPM_LOAD_UA	1000 /* uA */

struct ufs_vreg {
	struct regulator *reg;
	const char *name;
	bool always_on;
	bool enabled;
	int min_uV;
	int max_uV;
	int max_uA;
};

struct ufs_vreg_info {
	struct ufs_vreg *vcc;
	struct ufs_vreg *vccq;
	struct ufs_vreg *vccq2;
	struct ufs_vreg *vdd_hba;
	struct ufs_vreg *buck;
};

struct ufs_dev_info {
	bool f_power_on_wp_en;
	/* Keeps information if any of the LU is power on write protected */
	bool is_lu_power_on_wp;
	/* Maximum number of general LU supported by the UFS device */
	u8 max_lu_supported;
	u8 wb_dedicated_lu;
	u16 wmanufacturerid;
	u16 wmanufacturer_date;
	u16 spec_version;
	u8  serial_num_index;
	uint32_t vendor_feature;
	int fw_version;
	/*UFS device Product Name */
	u8 *model;
	u16 wspecversion;
	u32 clk_gating_wait_us;
	u32 d_ext_ufs_feature_sup;
	u8 b_wb_buffer_type;
	u32 d_wb_alloc_units;
	bool b_rpm_dev_flush_capable;
	u8 b_presrv_uspc_en;
	/* UFS HPB related flag */
	bool	hpb_enabled;
	ANDROID_KABI_RESERVE(1);
};

/**
 * ufs_is_valid_unit_desc_lun - checks if the given LUN has a unit descriptor
 * @dev_info: pointer of instance of struct ufs_dev_info
 * @lun: LU number to check
 * @return: true if the lun has a matching unit descriptor, false otherwise
 */
static inline bool ufs_is_valid_unit_desc_lun(struct ufs_dev_info *dev_info,
		u8 lun, u8 param_offset)
{
	if (!dev_info || !dev_info->max_lu_supported) {
		pr_err("Max General LU supported by UFS isn't initialized\n");
		return false;
	}
	/* WB is available only for the logical unit from 0 to 7 */
	if (param_offset == UNIT_DESC_PARAM_WB_BUF_ALLOC_UNITS)
		return lun < UFS_UPIU_MAX_WB_LUN_ID;
	return lun == UFS_UPIU_RPMB_WLUN || (lun < dev_info->max_lu_supported);
}
#define MAX_MODEL_LEN 16
#define HPB_READ16_CONTROL_VALUE 0x1
#define HPB_READ16_CMDLEN  0x10
#define HPB_READ16_TRANSLEN7  0x07
#define HPB_READ16_TRANSLEN8  0x08

#define HPB_SAMSUNG_BUFID 0x11
enum hpb_read16_compose {
	HPB_READ16_OP,
	HPB_RESERVED0,
	HPB_READ16_LBA1,
	HPB_READ16_LBA2,
	HPB_READ16_LBA3,
	HPB_READ16_LBA4,
	HPB_READ16_PPN1,
	HPB_READ16_PPN2,
	HPB_READ16_PPN3,
	HPB_READ16_PPN4,
	HPB_READ16_PPN5,
	HPB_READ16_PPN6,
	HPB_READ16_PPN7,
	HPB_READ16_PPN8,
	HPB_READ16_TRANS_LEN,
	HPB_READ16_CONTROL
};

#endif /* End of Header */
