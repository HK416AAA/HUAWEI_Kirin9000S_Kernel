/*
 * Copyright (C) 2010 Trusted Logic S.A.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef _PN5XX_H_
#define _PN5XX_H_

#define MAX_BUFFER_SIZE	512
#define PN547_MAGIC 0xE9
#define PN547_SET_PWR	_IOW(PN547_MAGIC, 0x01, unsigned int)
#define PN547_GET_CHIP_TYPE _IOW(PN547_MAGIC, 0x04, unsigned int)

#define DEFAULT_NFC_CLK_RATE	(192 * 100000L)
#define NFC_CLK_PIN     "clk_out0"
#define NFC_TRY_NUM 3
#define UICC_SUPPORT_CARD_EMULATION (1<<0)
#define eSE_SUPPORT_CARD_EMULATION (1<<1)
#define CARD_UNKNOWN	0
#define CARD1_SELECT	 1
#define CARD2_SELECT  2
#define MAX_ATTRIBUTE_BUFFER_SIZE 128

#define DLOAD_EXTENTED_GPIO_MASK (0x01)
#define IRQ_EXTENTED_GPIO_MASK (0x02)
#define VEN_EXTENTED_GPIO_MASK (0x04)

#define ENABLE_START	0
#define ENABLE_END		1
#define MAX_CONFIG_NAME_SIZE		64

#define MAX_NFC_CHIP_TYPE_SIZE		32
#define MAX_NFC_FW_VERSION_SIZE	32
#define MAX_DETECT_SE_SIZE	32
#define MAX_DETECT_GT_SIZE	8

#define NFC_SVDD_SW_ON	1
#define NFC_SVDD_SW_OFF	0

#define SIM1_CARD		1
#define SIM2_CARD		2
#define ESE_CARD		3

#define NFC_CLK_SRC_CPU		  0
#define NFC_CLK_SRC_PMU		  1
#define NFC_CLK_SRC_PMU_HI6555 2
#define NFC_CLK_SRC_PMU_HI6421V600 3
#define NFC_CLK_SRC_PMU_HI6555V200 255
#define NFC_CLK_SRC_XTAL        4       // need to match HAL defination
#define NFC_CLK_SRC_PMU_HI6421V700 5
#define NFC_CLK_SRC_PMU_HI6421V800 6
#define NFC_CLK_SRC_PMU_HI6421V900 7
#define NFC_CLK_SRC_PMU_HI6555V300 254
#define NFC_CLK_SRC_PMU_HI6555V500 253



#define WAKE_LOCK_TIMEOUT_DISABLE		  0
#define WAKE_LOCK_TIMEOUT_ENALBE		  1
#define MAX_WAKE_LOCK_TIMEOUT_SIZE	16

#define NFC_DMD_NUMBER_MIN  923002000
#define NFC_DMD_NUMBER_MAX  923002016
#define NFC_DMD_I2C_READ_ERROR_NO 923002001

#define TEL_HUAWEI_NV_NFCCAL_NUMBER   372
#define TEL_HUAWEI_NV_NFCCAL_NAME     "NFCCAL"
#define TEL_HUAWEI_NV_NFCCAL_LEGTH    104

#define MIN_NCI_CMD_LEN_WITH_BANKCARD_NUM 14
#define BANKCARD_NUM_LEN                16
#define BANKCARD_NUM_HEAD_LEN           2
#define BANKCARD_NUM_VALUE_LEN     (BANKCARD_NUM_LEN - BANKCARD_NUM_HEAD_LEN)
#define BANKCARD_NUM_OVERRIDE_LEN       8
#define BANKCARD_NUM_OVERRIDE_OFFSET    4
#define MIN_NCI_CMD_LEN_WITH_DOOR_NUM   10
#define FIRST_LINE_NCI_LEN   3
#define NFC_A_PASSIVE   3
#define NFCID_LEN   9
#define DOORCARD_MIN_LEN   4
#define BYTE_CHAR_LEN  2
#define DOORCARD_OVERRIDE_LEN  20
#define MIFARE_DATA_UID_OFFSET 2
#define NCI_CMD_HEAD_OFFSET             0
#define FWUPDATE_ON_OFFSET  38  // (NFC_ERASER_OFFSET +1)

#define CLR_BIT     0
#define SET_BIT     1

#define CHAR_0 48
#define CHAR_9 57

#define MAX_SIZE 256

#define NORMAL_MODE 0
#define RECOVERY_MODE 1
extern int nfc_record_dmd_info(long dmd_no, const char *dmd_info);

typedef struct pmu_reg_control {
    int addr;  /* reg address */
    int pos;   /* bit position */
} t_pmu_reg_control;

struct ese_config_data {
	u32 nfc_ese_num_dts;
	u32 spi_bus;
	u32 gpio_spi_cs;
	u32 gpio_ese_irq;
	u32 gpio_ese_reset;
	u32 svdd_pwr_req_need;
	u32 gpio_svdd_pwr_req;
	u32 spi_switch_need;
	u32 gpio_spi_switch;
};

/*
 * NFCC VEN GPIO could be controlled by different gpio type
 * NFC_ON_BY_GPIO: normal gpio in AP
 * NFC_ON_BY_HISI_PMIC: a gpio in PMU(HI6421V500 or before), usually notely PMU0_NFC_ON
 * NFC_ON_BY_HI6421V600_PMIC: a gpio in HI6421V600 PMU Platform, usually notely PMU0_NFC_ON
 * NFC_ON_BY_REGULATOR_BULK: a gpio used when pn544 chip which can
 * keep high when system shutdown
 * */
enum NFC_ON_TYPE {
	NFC_ON_BY_GPIO = 0,
	NFC_ON_BY_HISI_PMIC,
	NFC_ON_BY_HI6421V600_PMIC,
	NFC_ON_BY_REGULATOR_BULK,
	NFC_ON_BY_HI6555V110_PMIC,
	NFC_ON_BY_HI6421V700_PMIC,
	NFC_ON_BY_HI6421V800_PMIC,
	NFC_ON_BY_HI6421V900_PMIC,
	NFC_ON_BY_HI6555V300_PMIC,
	NFC_ON_BY_HI6555V500_PMIC,
};

enum NFC_SWP_SWITCH_PMU_PLATFROM_TYPE {
	NFC_SWP_WITHOUT_SW = 0,
	NFC_SWP_SW_HI6421V500 = 1,
	NFC_SWP_SW_HI6421V600 = 2,
	NFC_SWP_SW_HI6555V110 = 3,
	NFC_SWP_SW_HI6421V700 = 4,
	NFC_SWP_SW_HI6421V800 = 5,
	NFC_SWP_SW_HI6555V300 = 6,
	NFC_SWP_SW_HI6555V500 = 7,
	NFC_SWP_SW_HI6421V900 = 8,
};


/*
 * NFCC can wired different eSEs by SWP/DWP
 * default NFCC don't wired eSE.
 * NFC_WITHOUT_ESE: NFCC don't wired eSE
 * NFC_ESE_P61: use NXP p61 as eSE
 * NFC_ESE_HISEE: use hisi se as eSE
 */
enum NFC_ESE_TYPE {
	NFC_WITHOUT_ESE = 0,
	NFC_ESE_P61,
	NFC_ESE_HISEE,
};

/*
 * When SE is activated by NFCC,
 * we should record technologies SE can be supported.
 * NFC_WITHOUT_SE_ACTIVATED: cann't find activated SE
 * NFC_WITH_SE_TYPE_A: the activated SE support RF technology A
 * NFC_WITH_SE_TYPE_B: the activated SE support RF technology B
 * NFC_WITH_SE_TYPE_AB: the activated SE support RF technology A & B
 * */
enum NFC_ACTIVATED_SE_INFO_E {
	NFC_WITHOUT_SE_ACTIVATED = 0,
	NFC_WITH_SE_TYPE_A,
	NFC_WITH_SE_TYPE_B,
	NFC_WITH_SE_TYPE_AB,
};

#endif
