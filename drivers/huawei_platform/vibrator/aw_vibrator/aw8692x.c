 /*
  * aw8692x.c
  *
  * code for vibrator
  *
  * Copyright (c) 2020 Huawei Technologies Co., Ltd.
  *
  * This software is licensed under the terms of the GNU General Public
  * License version 2, as published by the Free Software Foundation, and
  * may be copied, distributed, and modified under those terms.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/debugfs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/power_supply.h>
#include <linux/vmalloc.h>
#include <linux/pm_qos.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/soc.h>

#include "haptic_hv.h"
#include "haptic_hv_reg.h"
#include "aw8692x_reg.h"


/* from haptic_hv i2c fuc */
int hv_i2c_r_bytes(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint8_t *buf,
		uint32_t len)
{
	int ret;
	struct i2c_msg msg[] = {
		[0] = {
			.addr = aw_haptic->i2c->addr,
			.flags = 0,
			.len = sizeof(uint8_t),
			.buf = &reg_addr,
			},
		[1] = {
			.addr = aw_haptic->i2c->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
			},
	};

	if (!aw_haptic) {
		aw_err ("aw_haptic is null");
		return -EINVAL;
	}
	ret = i2c_transfer(aw_haptic->i2c->adapter, msg, ARRAY_SIZE(msg));
	if (ret < 0) {
		aw_err("transfer failed");
		return ret;
	} else if (ret != AW_I2C_READ_MSG_NUM) {
		aw_err("transfer failed(size error)");
		return -ENXIO;
	}
	return ret;
}

int hv_i2c_w_bytes(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint8_t *buf,
		uint32_t len)
{
	uint8_t *data = NULL;
	int ret = -1;

	if (!aw_haptic || !buf) {
		aw_err ("aw_haptic or buf is null");
		return -EINVAL;
	}
	data = kmalloc(len + 1, GFP_KERNEL);
	data[0] = reg_addr;
	memcpy(&data[1], buf, len);

	if (!aw_haptic->i2c) {
		aw_err ("i2c is null");
		return -EINVAL;
	}
	ret = i2c_master_send(aw_haptic->i2c, data, len + 1);
	if (ret < 0)
		aw_err("i2c master send 0x%02x err", reg_addr);
	kfree(data);
	return ret;
}

int hv_i2c_w_bits(struct aw_haptic *aw_haptic, uint8_t reg_addr, uint32_t mask,
			uint8_t reg_data)
{
	uint8_t reg_val = 0;
	int ret = -1;

	if (!aw_haptic) {
		aw_err ("aw_haptic is null");
		return -EINVAL;
	}
	ret = hv_i2c_r_bytes(aw_haptic, reg_addr, &reg_val, AW_I2C_BYTE_ONE);
	if (ret < 0) {
		aw_err("i2c read error, ret=%d", ret);
		return ret;
	}
	reg_val &= mask;
	reg_val |= reg_data;
	ret = hv_i2c_w_bytes(aw_haptic, reg_addr, &reg_val, AW_I2C_BYTE_ONE);
	if (ret < 0) {
		aw_err("i2c write error, ret=%d", ret);
		return ret;
	}
	return 0;
}
/* from haptic_hv i2c fuc */


static int aw8692x_check_qualify(struct aw_haptic *aw_haptic)
{
	int ret = -1;
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	ret = hv_i2c_r_bytes(aw_haptic, AW8692X_REG_EFCFG6, &reg_val,
					AW_I2C_BYTE_ONE);
	if (ret < 0)
		return ret;
	if (!(reg_val & 0x80)) {
		aw_err("unqualified chip!");
		return -ERANGE;
	}
	return 0;
}

static void aw8692x_set_pwm(struct aw_haptic *aw_haptic, uint8_t mode)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	switch (mode) {
	case AW_PWM_48K:
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL4,
				AW8692X_BIT_SYSCTRL4_WAVDAT_MODE_MASK,
				AW8692X_BIT_SYSCTRL4_WAVDAT_48K);
		break;
	case AW_PWM_24K:
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL4,
				AW8692X_BIT_SYSCTRL4_WAVDAT_MODE_MASK,
				AW8692X_BIT_SYSCTRL4_WAVDAT_24K);
		break;
	case AW_PWM_12K:
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL4,
				AW8692X_BIT_SYSCTRL4_WAVDAT_MODE_MASK,
				AW8692X_BIT_SYSCTRL4_WAVDAT_12K);
		break;
	default:
		aw_err("error param!");
		break;
	}
}

static void aw8692x_enable_gain(struct aw_haptic *aw_haptic,  bool flag)
{
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL4, (~(1<<0)), flag);
}

static void aw8692x_set_gain(struct aw_haptic *aw_haptic, uint8_t gain)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PLAYCFG2, &gain, AW_I2C_BYTE_ONE);
}

static void aw8692x_set_bst_peak_cur(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	/* Unlock register */
	reg_val = AW8692X_BIT_TMCFG_TM_UNLOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);
	switch (aw_haptic->bst_pc) {
	case AW_BST_PC_L1:
		aw_info("bst pc = L1");
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG13,
				AW8692X_BIT_ANACFG13_BST_PC_MASK,
				AW8692X_BIT_ANACFG13_BST_PEAKCUR_3P45A);
		break;
	case AW_BST_PC_L2:
		aw_info("bst pc = L2");
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG13,
				AW8692X_BIT_ANACFG13_BST_PC_MASK,
				AW8692X_BIT_ANACFG13_BST_PEAKCUR_4A);
		break;
	default:
		aw_info("bst pc = L1");
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG13,
				AW8692X_BIT_ANACFG13_BST_PC_MASK,
				AW8692X_BIT_ANACFG13_BST_PEAKCUR_3P45A);
		break;
	}
	/* lock register */
	reg_val = AW8692X_BIT_TMCFG_TM_LOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);
}

static void aw8692x_set_bst_vol(struct aw_haptic *aw_haptic, uint8_t bst_vol)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (bst_vol & AW8692X_BIT_PLAYCFG1_BIT8)
		bst_vol = AW8692X_BIT_PLAYCFG1_BST_VOUT_MAX;
	if (bst_vol < AW8692X_BIT_PLAYCFG1_BST_VOUT_6V)
		bst_vol = AW8692X_BIT_PLAYCFG1_BST_VOUT_6V;
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG1,
			AW8692X_BIT_PLAYCFG1_BST_VOUT_VREFSET_MASK, bst_vol);
}

static void aw8692x_set_wav_seq(struct aw_haptic *aw_haptic, uint8_t wav,
				uint8_t seq)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_WAVCFG1 + wav, &seq,
				AW_I2C_BYTE_ONE);
}

static void aw8692x_set_wav_loop(struct aw_haptic *aw_haptic, uint8_t wav,
								uint8_t loop)
{
	uint8_t tmp = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (wav % 2) {
		tmp = loop << 0;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_WAVCFG9 + (wav / 2),
				AW8692X_BIT_WAVLOOP_SEQ_EVEN_MASK, tmp);
	} else {
		tmp = loop << 4;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_WAVCFG9 + (wav / 2),
				AW8692X_BIT_WAVLOOP_SEQ_ODD_MASK, tmp);
	}
}

static void aw8692x_set_rtp_data(struct aw_haptic *aw_haptic, uint8_t *data,
								uint32_t len)
{
	if (!aw_haptic || !data) {
		aw_err("aw_haptic or data is null");
		return;
	}
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_RTPDATA, data, len);
}

static void aw8692x_set_rtp_aei(struct aw_haptic *aw_haptic, bool flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (flag) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSINTM,
				AW8692X_BIT_SYSINTM_FF_AEM_MASK,
				AW8692X_BIT_SYSINTM_FF_AEM_ON);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSINTM,
				AW8692X_BIT_SYSINTM_FF_AEM_MASK,
				AW8692X_BIT_SYSINTM_FF_AEM_OFF);
	}
}

static void aw8692x_set_ram_addr(struct aw_haptic *aw_haptic)
{
	uint8_t ram_addr[2] = {0};
	uint32_t base_addr = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	base_addr = aw_haptic->ram.base_addr;
	ram_addr[0] = (uint8_t)AW8692X_RAMADDR_H(base_addr);
	ram_addr[1] = (uint8_t)AW8692X_RAMADDR_L(base_addr);

	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_RAMADDRH, ram_addr, AW_I2C_BYTE_TWO);
}

static void aw8692x_set_base_addr(struct aw_haptic *aw_haptic)
{
	uint8_t rtp_addr[2] = {0};
	uint32_t base_addr = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	base_addr = aw_haptic->ram.base_addr;
	rtp_addr[0] = (uint8_t)AW8692X_BASEADDR_H(base_addr);
	rtp_addr[1] = (uint8_t)AW8692X_BASEADDR_L(base_addr);

	hv_i2c_w_bits(aw_haptic, AW8692X_REG_RTPCFG1,
			AW8692X_BIT_RTPCFG1_BASE_ADDR_H_MASK, rtp_addr[0]);
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_RTPCFG2, &rtp_addr[1],
			AW_I2C_BYTE_ONE);
}

static void aw8692x_auto_break_mode(struct aw_haptic *aw_haptic, bool flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (flag) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_BRK_EN_MASK,
				AW8692X_BIT_PLAYCFG3_BRK_ENABLE);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_BRK_EN_MASK,
				AW8692X_BIT_PLAYCFG3_BRK_DISABLE);
	}
}

static void aw8692x_f0_detect(struct aw_haptic *aw_haptic, bool flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (flag) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG1,
				AW8692X_BIT_CONTCFG1_EN_F0_DET_MASK,
				AW8692X_BIT_CONTCFG1_F0_DET_ENABLE);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG1,
				AW8692X_BIT_CONTCFG1_EN_F0_DET_MASK,
				AW8692X_BIT_CONTCFG1_F0_DET_DISABLE);
	}
}

static uint8_t aw8692x_get_glb_state(struct aw_haptic *aw_haptic)
{
	uint8_t state = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_GLBRD5, &state, AW_I2C_BYTE_ONE);
	return state;
}

static void aw8692x_play_go(struct aw_haptic *aw_haptic, bool flag)
{
	uint8_t go_on = AW8692X_BIT_PLAYCFG4_GO_ON;
	uint8_t stop_on = AW8692X_BIT_PLAYCFG4_STOP_ON;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_info("enter, flag = %d", flag);
	if (flag)
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PLAYCFG4, &go_on,
				AW_I2C_BYTE_ONE);
	else
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PLAYCFG4, &stop_on,
				AW_I2C_BYTE_ONE);
}

static int aw8692x_wait_enter_standby(struct aw_haptic *aw_haptic, uint32_t cnt)
{
	int i = cnt;
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	while (i) {
		reg_val = aw8692x_get_glb_state(aw_haptic);
		if (reg_val == AW8692X_BIT_GLBRD5_STATE_STANDBY) {
			aw_info("entered standby!");
			return 0;
		}
		i--;
		aw_dbg("wait for standby");
		usleep_range(2000, 2500);
	}

	return -ERANGE;
}

static void aw8692x_bst_mode_config(struct aw_haptic *aw_haptic, uint8_t mode)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (aw_haptic->bst_enable) {
		switch (mode) {
		case AW_BST_BOOST_MODE:
			aw_info("haptic bst mode = bst");
			hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG1,
					AW8692X_BIT_PLAYCFG1_BST_MODE_MASK,
					AW8692X_BIT_PLAYCFG1_BST_MODE);
			break;
		case AW_BST_BYPASS_MODE:
			aw_info("haptic bst mode = bypass");
			hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG1,
					AW8692X_BIT_PLAYCFG1_BST_MODE_MASK,
					AW8692X_BIT_PLAYCFG1_BST_MODE_BYPASS);
			break;
		default:
			aw_err("bst = %d error", mode);
			break;
		}
	} else {
		aw_info("haptic bst mode = bypass");
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG1,
				AW8692X_BIT_PLAYCFG1_BST_MODE_MASK,
				AW8692X_BIT_PLAYCFG1_BST_MODE_BYPASS);
	}
}

static void aw8692x_play_mode(struct aw_haptic *aw_haptic, uint8_t play_mode)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	switch (play_mode) {
	case AW_STANDBY_MODE:
		aw_info("enter standby mode");
		aw_haptic->play_mode = AW_STANDBY_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL3,
				AW8692X_BIT_SYSCTRL3_STANDBY_MASK,
				AW8692X_BIT_SYSCTRL3_STANDBY_ON);
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL3,
				AW8692X_BIT_SYSCTRL3_STANDBY_MASK,
				AW8692X_BIT_SYSCTRL3_STANDBY_OFF);
		break;
	case AW_RAM_MODE:
		aw_info("enter ram mode");
		aw_haptic->play_mode = AW_RAM_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_MASK,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_RAM);
		break;
	case AW_RAM_LOOP_MODE:
		aw_info("enter ram loop mode");
		aw_haptic->play_mode = AW_RAM_LOOP_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_MASK,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_RAM);
		/* bst mode */
		aw8692x_bst_mode_config(aw_haptic, AW_BST_BYPASS_MODE);
		break;
	case AW_RTP_MODE:
		aw_info("enter rtp mode");
		aw_haptic->play_mode = AW_RTP_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_MASK,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_RTP);
		/* bst mode */
		aw8692x_bst_mode_config(aw_haptic, AW_BST_BOOST_MODE);
		break;
	case AW_TRIG_MODE:
		aw_info("enter trig mode");
		aw_haptic->play_mode = AW_TRIG_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_MASK,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_RAM);
		break;
	case AW_CONT_MODE:
		aw_info("enter cont mode");
		aw_haptic->play_mode = AW_CONT_MODE;
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_MASK,
				AW8692X_BIT_PLAYCFG3_PLAY_MODE_CONT);
		/* bst mode */
		aw8692x_bst_mode_config(aw_haptic, AW_BST_BYPASS_MODE);
		break;
	default:
		aw_err("play mode %d error", play_mode);
		break;
	}
}

static void aw8692x_stop(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;
	int ret = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_haptic->play_mode = AW_STANDBY_MODE;
	reg_val = AW8692X_BIT_PLAYCFG4_STOP_ON;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PLAYCFG4, &reg_val, AW_I2C_BYTE_ONE);
	ret = aw8692x_wait_enter_standby(aw_haptic, 40);
	if (ret < 0) {
		aw_err("force to enter standby mode!");
		aw8692x_play_mode(aw_haptic, AW_STANDBY_MODE);
	}
}

static void aw8692x_ram_init(struct aw_haptic *aw_haptic, bool flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (flag) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL3,
				AW8692X_BIT_SYSCTRL3_EN_RAMINIT_MASK,
				AW8692X_BIT_SYSCTRL3_EN_RAMINIT_ON);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL3,
				AW8692X_BIT_SYSCTRL3_EN_RAMINIT_MASK,
				AW8692X_BIT_SYSCTRL3_EN_RAMINIT_OFF);
	}
}

static void aw8692x_upload_lra(struct aw_haptic *aw_haptic, uint32_t flag)
{
	uint8_t cali_data = 0;
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	/* Unlock register */
	reg_val = AW8692X_BIT_TMCFG_TM_UNLOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);
	switch (flag) {
	case AW_WRITE_ZERO:
		aw_info("write zero to trim_lra!");
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG20,
				AW8692X_BIT_ANACFG20_TRIM_LRA_MASK, cali_data);
		break;
	case AW_F0_CALI_LRA:
		aw_info("write f0_cali_data to trim_lra = 0x%02X",
			aw_haptic->f0_cali_data);
		cali_data = (char)aw_haptic->f0_cali_data &
					AW8692X_BIT_ANACFG20_TRIM_LRA;
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_ANACFG20, &cali_data,
					AW_I2C_BYTE_ONE);
		break;
	case AW_OSC_CALI_LRA:
		aw_info("write osc_cali_data to trim_lra = 0x%02X",
			aw_haptic->osc_cali_data);
		cali_data = (char)aw_haptic->osc_cali_data &
					AW8692X_BIT_ANACFG20_TRIM_LRA;
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_ANACFG20, &cali_data,
					AW_I2C_BYTE_ONE);
		break;
	default:
		aw_err("error param!");
		break;
	}
	/* lock register */
	reg_val = AW8692X_BIT_TMCFG_TM_LOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);
}

static uint8_t aw8692x_get_trim_lra(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_ANACFG20, &reg_val, AW_I2C_BYTE_ONE);
	reg_val &= 0x3F;
	return reg_val;
}

static void aw8692x_vbat_mode_config(struct aw_haptic *aw_haptic, uint8_t flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (flag == AW_CONT_VBAT_HW_COMP_MODE) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_VBATCTRL,
				AW8692X_BIT_VBATCTRL_VBAT_MODE_MASK,
				AW8692X_BIT_VBATCTRL_VBAT_MODE_HW);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_VBATCTRL,
				AW8692X_BIT_VBATCTRL_VBAT_MODE_MASK,
				AW8692X_BIT_VBATCTRL_VBAT_MODE_SW);
	}
}

static void aw8692x_protect_config(struct aw_haptic *aw_haptic, uint8_t addr,
								uint8_t val)
{
	aw_info("enter");
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (addr == 1) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG2,
				AW8692X_BIT_PWMCFG2_PRCT_MODE_MASK,
				AW8692X_BIT_PWMCFG2_PRCT_MODE_VALID);
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG1,
				AW8692X_BIT_PWMCFG1_PRC_EN_MASK,
				AW8692X_BIT_PWMCFG1_PRC_ENABLE);
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG3,
				AW8692X_BIT_PWMCFG3_PR_EN_MASK,
				AW8692X_BIT_PWMCFG3_PR_ENABLE);
	} else if (addr == 0) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG2,
				AW8692X_BIT_PWMCFG2_PRCT_MODE_MASK,
				AW8692X_BIT_PWMCFG2_PRCT_MODE_INVALID);
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG1,
				AW8692X_BIT_PWMCFG1_PRC_EN_MASK,
				AW8692X_BIT_PWMCFG1_PRC_DISABLE);
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG3,
				AW8692X_BIT_PWMCFG3_PR_EN_MASK,
				AW8692X_BIT_PWMCFG3_PR_DISABLE);
	} else if (addr == AW8692X_REG_PWMCFG1) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG1,
				AW8692X_BIT_PWMCFG1_PRCTIME_MASK, val);
	} else if (addr == AW8692X_REG_PWMCFG3) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PWMCFG3,
				AW8692X_BIT_PWMCFG3_PRLVL_MASK, val);
	} else if (addr == AW8692X_REG_PWMCFG4) {
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PWMCFG4, &val,
				AW_I2C_BYTE_ONE);
	}
}

static void aw8692x_cont_config(struct aw_haptic *aw_haptic)
{
	/* uint8_t drv1_time = 0xFF; */
	uint8_t drv2_time = 0xFF;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	/* work mode */
	aw8692x_play_mode(aw_haptic, AW_CONT_MODE);
	/* f0 driver level */
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG6,
			&aw_haptic->info.cont_drv1_lvl, AW_I2C_BYTE_ONE);
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG7,
			&aw_haptic->info.cont_drv2_lvl, AW_I2C_BYTE_ONE);
	/* DRV1_TIME */
	/* i2c_w_byte(aw_haptic, AW8692X_REG_CONTCFG8, &drv1_time,
		      AW_I2C_BYTE_ONE); */
	/* DRV2_TIME */
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG9, &drv2_time,
			AW_I2C_BYTE_ONE);
	/* cont play go */
	aw8692x_play_go(aw_haptic, true);
}

static void aw8692x_one_wire_init(struct aw_haptic *aw_haptic)
{
	uint8_t trig_prio = 0x6c;
	uint8_t delay_2p5ms = AW8692X_BIT_START_DLY_2P5MS;

	aw_info("enter");
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	/* if enable one-wire, trig1 priority must be less than trig2 and trig3 */
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_GLBCFG4, &trig_prio,
			AW_I2C_BYTE_ONE);
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_GLBCFG2, &delay_2p5ms,
			AW_I2C_BYTE_ONE);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_TRGCFG8,
			AW8692X_BIT_TRGCFG8_TRG_ONEWIRE_MASK,
			AW8692X_BIT_TRGCFG8_TRG_ONEWIRE_ENABLE);
}

static void aw8692x_trig1_param_init(struct aw_haptic *aw_haptic)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_haptic->trig[0].trig_level = aw_haptic->info.trig_cfg[0];
	aw_haptic->trig[0].trig_polar = aw_haptic->info.trig_cfg[1];
	aw_haptic->trig[0].pos_enable = aw_haptic->info.trig_cfg[2];
	aw_haptic->trig[0].pos_sequence = aw_haptic->info.trig_cfg[3];
	aw_haptic->trig[0].neg_enable = aw_haptic->info.trig_cfg[4];
	aw_haptic->trig[0].neg_sequence = aw_haptic->info.trig_cfg[5];
	aw_haptic->trig[0].trig_brk = aw_haptic->info.trig_cfg[6];
	aw_haptic->trig[0].trig_bst = aw_haptic->info.trig_cfg[7];
}

static void aw8692x_trig2_param_init(struct aw_haptic *aw_haptic)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_haptic->trig[1].trig_level = aw_haptic->info.trig_cfg[8];
	aw_haptic->trig[1].trig_polar = aw_haptic->info.trig_cfg[9];
	aw_haptic->trig[1].pos_enable = aw_haptic->info.trig_cfg[10];
	aw_haptic->trig[1].pos_sequence = aw_haptic->info.trig_cfg[11];
	aw_haptic->trig[1].neg_enable = aw_haptic->info.trig_cfg[12];
	aw_haptic->trig[1].neg_sequence = aw_haptic->info.trig_cfg[13];
	aw_haptic->trig[1].trig_brk = aw_haptic->info.trig_cfg[14];
	aw_haptic->trig[1].trig_bst = aw_haptic->info.trig_cfg[15];
}

static void aw8692x_trig3_param_init(struct aw_haptic *aw_haptic)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_haptic->trig[2].trig_level = aw_haptic->info.trig_cfg[16];
	aw_haptic->trig[2].trig_polar = aw_haptic->info.trig_cfg[17];
	aw_haptic->trig[2].pos_enable = aw_haptic->info.trig_cfg[18];
	aw_haptic->trig[2].pos_sequence = aw_haptic->info.trig_cfg[19];
	aw_haptic->trig[2].neg_enable = aw_haptic->info.trig_cfg[20];
	aw_haptic->trig[2].neg_sequence = aw_haptic->info.trig_cfg[21];
	aw_haptic->trig[2].trig_brk = aw_haptic->info.trig_cfg[22];
	aw_haptic->trig[2].trig_bst = aw_haptic->info.trig_cfg[23];
}

static void aw8692x_trig1_param_config(struct aw_haptic *aw_haptic)
{
	uint8_t trig_config = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (aw_haptic->trig[0].trig_level)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_MODE_LEVEL;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_MODE_EDGE;

	if (aw_haptic->trig[0].trig_polar)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_POLAR_NEG;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_POLAR_POS;

	if (aw_haptic->trig[0].trig_brk)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_AUTO_BRK_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_AUTO_BRK_DISABLE;

	if (aw_haptic->trig[0].trig_bst)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_BST_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG1_BST_DISABLE;

	hv_i2c_w_bits(aw_haptic, AW8692X_REG_TRGCFG7,
			(AW8692X_BIT_TRGCFG7_TRG1_MODE_MASK &
			AW8692X_BIT_TRGCFG7_TRG1_POLAR_MASK &
			AW8692X_BIT_TRGCFG7_TRG1_AUTO_BRK_MASK &
			AW8692X_BIT_TRGCFG7_TRG1_BST_MASK), trig_config);

	trig_config = 0;

	/* pos config */
	if (aw_haptic->trig[0].pos_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[0].pos_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG1, &trig_config,
				AW_I2C_BYTE_ONE);

	trig_config = 0;

	/* neq config */
	if (aw_haptic->trig[0].neg_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[0].neg_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG4, &trig_config,
			AW_I2C_BYTE_ONE);
}

static void aw8692x_trig2_param_config(struct aw_haptic *aw_haptic)
{
	uint8_t trig_config = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (aw_haptic->trig[1].trig_level)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_MODE_LEVEL;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_MODE_EDGE;

	if (aw_haptic->trig[1].trig_polar)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_POLAR_NEG;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_POLAR_POS;

	if (aw_haptic->trig[1].trig_brk)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_AUTO_BRK_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_AUTO_BRK_DISABLE;

	if (aw_haptic->trig[1].trig_bst)
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_BST_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG7_TRG2_BST_DISABLE;

	hv_i2c_w_bits(aw_haptic, AW8692X_REG_TRGCFG7,
			(AW8692X_BIT_TRGCFG7_TRG2_MODE_MASK &
			AW8692X_BIT_TRGCFG7_TRG2_POLAR_MASK &
			AW8692X_BIT_TRGCFG7_TRG2_AUTO_BRK_MASK &
			AW8692X_BIT_TRGCFG7_TRG2_BST_MASK), trig_config);

	trig_config = 0;

	/* pos config */
	if (aw_haptic->trig[1].pos_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[1].pos_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG2, &trig_config,
			AW_I2C_BYTE_ONE);

	trig_config = 0;

	/* neq config */
	if (aw_haptic->trig[1].neg_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[1].neg_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG5, &trig_config,
			AW_I2C_BYTE_ONE);
}

static void aw8692x_trig3_param_config(struct aw_haptic *aw_haptic)
{
	uint8_t trig_config = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (aw_haptic->trig[2].trig_level)
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_MODE_LEVEL;
	else
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_MODE_EDGE;

	if (aw_haptic->trig[2].trig_polar)
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_POLAR_NEG;
	else
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_POLAR_POS;

	if (aw_haptic->trig[2].trig_brk)
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_AUTO_BRK_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_AUTO_BRK_DISABLE;

	if (aw_haptic->trig[2].trig_bst)
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_BST_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRGCFG8_TRG3_BST_DISABLE;

	hv_i2c_w_bits(aw_haptic, AW8692X_REG_TRGCFG8,
			(AW8692X_BIT_TRGCFG8_TRG3_MODE_MASK &
			AW8692X_BIT_TRGCFG8_TRG3_POLAR_MASK &
			AW8692X_BIT_TRGCFG8_TRG3_AUTO_BRK_MASK &
			AW8692X_BIT_TRGCFG8_TRG3_BST_MASK), trig_config);

	trig_config = 0;

	/* pos config */
	if (aw_haptic->trig[2].pos_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[2].pos_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG3, &trig_config,
			AW_I2C_BYTE_ONE);

	trig_config = 0;

	/* neq config */
	if (aw_haptic->trig[2].neg_enable)
		trig_config |= AW8692X_BIT_TRG_ENABLE;
	else
		trig_config |= AW8692X_BIT_TRG_DISABLE;

	trig_config |= aw_haptic->trig[2].neg_sequence;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TRGCFG6, &trig_config,
			AW_I2C_BYTE_ONE);
}

static void aw8692x_auto_bst_enable(struct aw_haptic *aw_haptic, uint8_t flag)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw_haptic->auto_boost = flag;
	if (flag) {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_AUTO_BST_MASK,
				AW8692X_BIT_PLAYCFG3_AUTO_BST_ENABLE);
	} else {
		hv_i2c_w_bits(aw_haptic, AW8692X_REG_PLAYCFG3,
				AW8692X_BIT_PLAYCFG3_AUTO_BST_MASK,
				AW8692X_BIT_PLAYCFG3_AUTO_BST_DISABLE);
	}
}

static void aw8692x_interrupt_setup(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSINT, &reg_val, AW_I2C_BYTE_ONE);
	aw_info("reg SYSINT=0x%02X", reg_val);
	/* edge int mode */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSCTRL4,
			(AW8692X_BIT_SYSCTRL4_INT_MODE_MASK &
			AW8692X_BIT_SYSCTRL4_INT_EDGE_MODE_MASK),
			(AW8692X_BIT_SYSCTRL4_INT_MODE_EDGE |
			AW8692X_BIT_SYSCTRL4_INT_EDGE_MODE_POS));
	/* int enable */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_SYSINTM,
			(AW8692X_BIT_SYSINTM_BST_SCPM_MASK &
			AW8692X_BIT_SYSINTM_BST_OVPM_MASK &
			AW8692X_BIT_SYSINTM_UVLM_MASK &
			AW8692X_BIT_SYSINTM_OCDM_MASK &
			AW8692X_BIT_SYSINTM_OTM_MASK),
			(AW8692X_BIT_SYSINTM_BST_SCPM_ON |
			AW8692X_BIT_SYSINTM_BST_OVPM_OFF |
			AW8692X_BIT_SYSINTM_UVLM_ON |
			AW8692X_BIT_SYSINTM_OCDM_ON |
			AW8692X_BIT_SYSINTM_OTM_ON));
}

static int aw8692x_juge_rtp_going(struct aw_haptic *aw_haptic)
{
	uint8_t glb_state = 0;
	uint8_t rtp_state = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	glb_state = aw8692x_get_glb_state(aw_haptic);
	if (aw_haptic->rtp_routine_on ||
		(glb_state == AW8692X_BIT_GLBRD5_STATE_RTP_GO)) {
		rtp_state = 1;  /* is going on */
		aw_info("rtp_routine_on");
	}
	return rtp_state;
}

static ssize_t aw8692x_get_ram_data(struct aw_haptic *aw_haptic, char *buf)
{
	uint8_t ram_data[AW_RAMDATA_RD_BUFFER_SIZE] = {0};
	int i = 0;
	int j = 0;
	int size = 0;
	ssize_t len = 0;

	if (!aw_haptic || !buf) {
		aw_err("aw_haptic or buf is null");
		return -EINVAL;
	}
	while (i < aw_haptic->ram.len) {
		if ((aw_haptic->ram.len - i) < AW_RAMDATA_RD_BUFFER_SIZE)
			size = aw_haptic->ram.len - i;
		else
			size = AW_RAMDATA_RD_BUFFER_SIZE;

		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_RAMDATA, ram_data, size);
		for (j = 0; j < size; j++) {
			len += snprintf(buf + len, PAGE_SIZE - len,
					"0x%02X,", ram_data[j]);
		}
		i += size;
	}
	return len;
}

static void aw8692x_get_first_wave_addr(struct aw_haptic *aw_haptic,
										uint8_t *wave_addr)
{
	uint8_t reg_val[3] = {0};

	if (!aw_haptic || !wave_addr) {
		aw_err("aw_haptic or wave_addr is null");
		return;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_RAMDATA, reg_val, AW_I2C_BYTE_THREE);
	wave_addr[0] = reg_val[1];
	wave_addr[1] = reg_val[2];
}

static void aw8692x_get_wav_seq(struct aw_haptic *aw_haptic, uint32_t len)
{
	uint8_t i = 0;
	uint8_t reg_val[AW_SEQUENCER_SIZE] = {0};

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (len > AW_SEQUENCER_SIZE)
		len = AW_SEQUENCER_SIZE;
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_WAVCFG1, reg_val, len);
	for (i = 0; i < len; i++)
		aw_haptic->seq[i] = reg_val[i];
}

static size_t aw8692x_get_wav_loop(struct aw_haptic *aw_haptic, char *buf)
{
	uint8_t i = 0;
	uint8_t reg_val[AW_SEQUENCER_LOOP_SIZE] = {0};
	size_t count = 0;

	if (!aw_haptic || !buf) {
		aw_err("aw_haptic or buf is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_WAVCFG9, reg_val,
			AW_SEQUENCER_LOOP_SIZE);

	for (i = 0; i < AW_SEQUENCER_LOOP_SIZE; i++) {
		aw_haptic->loop[i * 2 + 0] = (reg_val[i] >> 4) & 0x0F;
		aw_haptic->loop[i * 2 + 1] = (reg_val[i] >> 0) & 0x0F;

		count += snprintf(buf + count, PAGE_SIZE - count,
						"seq%d loop: 0x%02x\n", i * 2 + 1,
						aw_haptic->loop[i * 2 + 0]);
		count += snprintf(buf + count, PAGE_SIZE - count,
						"seq%d loop: 0x%02x\n", i * 2 + 2,
						aw_haptic->loop[i * 2 + 1]);
	}
	return count;
}

static void aw8692x_irq_clear(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSINT, &reg_val, AW_I2C_BYTE_ONE);
	aw_info("reg SYSINT=0x%02X", reg_val);
}

static uint8_t aw8692x_get_prctmode(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_PWMCFG2, &reg_val, AW_I2C_BYTE_ONE);
	reg_val &= 0x08;
	return reg_val;
}

static int aw8692x_get_irq_state(struct aw_haptic *aw_haptic)
{
	int ret = -1;
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSINT, &reg_val, AW_I2C_BYTE_ONE);
	aw_dbg("reg SYSINT=0x%02X", reg_val);

	if (reg_val & AW8692X_BIT_SYSINT_BST_OVPI) {
		aw_haptic->rtp_routine_on = 0;
		aw_err("chip ov int error");
	}

	if (reg_val & AW8692X_BIT_SYSINT_UVLI) {
		aw_haptic->rtp_routine_on = 0;
		aw_err("chip uvlo int error");
	}

	if (reg_val & AW8692X_BIT_SYSINT_OCDI) {
		aw_haptic->rtp_routine_on = 0;
		aw_err("chip over current int error");
	}

	if (reg_val & AW8692X_BIT_SYSINT_OTI) {
		aw_haptic->rtp_routine_on = 0;
		aw_err("chip over temperature int error");
	}

	if (reg_val & AW8692X_BIT_SYSINT_DONEI) {
		aw_haptic->rtp_routine_on = 0;
		aw_info("chip playback done");
	}

	if (reg_val & AW8692X_BIT_SYSINT_FF_AEI)
		ret = 0;

	if (reg_val & AW8692X_BIT_SYSINT_FF_AFI)
		aw_info("aw_haptic rtp mode fifo almost full!");
	return ret;
}

static void aw8692x_read_f0(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val[4] = {0};
	uint32_t f0_reg = 0;
	uint64_t f0_tmp = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	/* lra_f0 */
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_CONTCFG14, reg_val, 4);
	f0_reg = (reg_val[0] << 8) | reg_val[1];
	if (!f0_reg) {
		aw_haptic->f0_val_err = -1;
		aw_err("lra_f0 is error, f0_reg=0");
		return;
	}
	f0_tmp = 384000 * 10 / f0_reg;
	aw_haptic->f0 = (uint32_t)f0_tmp;
	aw_info("lra_f0=%d", aw_haptic->f0);

	/* cont_f0 */
	f0_reg = (reg_val[2] << 8) | reg_val[3];
	if (!f0_reg) {
		aw_haptic->f0_val_err = -1;
		aw_err("cont_f0 is error, f0_reg=0");
		return;
	}
	f0_tmp = 384000 * 10 / f0_reg;
	aw_haptic->cont_f0 = (uint32_t)f0_tmp;
	aw_info("cont_f0=%d", aw_haptic->cont_f0);
}

static int aw8692x_get_f0(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;
	uint8_t brk_en_default = 0;
	uint8_t reg_array[3] = {aw_haptic->info.cont_drv2_lvl,
				aw_haptic->info.cont_drv1_time,
				aw_haptic->info.cont_drv2_time};

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	aw_haptic->f0 = aw_haptic->info.f0_pre;
	/* enter standby mode */
	aw8692x_stop(aw_haptic);
	/* f0 calibrate work mode */
	aw8692x_play_mode(aw_haptic, AW_CONT_MODE);
	/* enable f0 detect */
	aw8692x_f0_detect(aw_haptic, true);
	/* cont config */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG6,
			AW8692X_BIT_CONTCFG6_TRACK_EN_MASK,
			AW8692X_BIT_CONTCFG6_TRACK_ENABLE);
	/* enable auto break */
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_PLAYCFG3, &reg_val, AW_I2C_BYTE_ONE);
	brk_en_default = 0x04 & reg_val;
	aw8692x_auto_break_mode(aw_haptic, true);

	/* f0 driver level */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG6,
			AW8692X_BIT_CONTCFG6_DRV1_LVL_MASK,
			aw_haptic->info.cont_drv1_lvl);
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG7, reg_array,
			AW_I2C_BYTE_THREE);
	/* TRACK_MARGIN */
	if (!aw_haptic->info.cont_track_margin) {
		aw_err("aw_haptic->info.cont_track_margin = 0");
	} else {
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG11,
				&aw_haptic->info.cont_track_margin,
				AW_I2C_BYTE_ONE);
	}
	/* DRV_WIDTH */
	/* i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG3, &aw_haptic->info.cont_drv_width, AW_I2C_BYTE_ONE); */

	/* cont play go */
	aw8692x_play_go(aw_haptic, true);
	/* 300ms */
	aw8692x_wait_enter_standby(aw_haptic, 1000);
	aw8692x_read_f0(aw_haptic);
	/* restore default config */
	aw8692x_f0_detect(aw_haptic, false);
	/* recover auto break config */
	if (brk_en_default)
		aw8692x_auto_break_mode(aw_haptic, true);
	else
		aw8692x_auto_break_mode(aw_haptic, false);
	return 0;
}

static uint8_t aw8692x_rtp_get_fifo_afs(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSST, &reg_val, AW_I2C_BYTE_ONE);
	reg_val &= AW8692X_BIT_SYSST_FF_AFS;
	reg_val = reg_val >> 3;
	return reg_val;
}

static uint8_t aw8692x_rtp_get_fifo_afi(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;

	aw_dbg("%s: enter!\n", __func__);
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSINT, &reg_val, AW_I2C_BYTE_ONE);
	reg_val &= AW8692X_BIT_SYSST_FF_AFS;
	reg_val = reg_val >> 3;
	return reg_val;
}

static uint8_t aw8692x_get_osc_status(struct aw_haptic *aw_haptic)
{
	uint8_t state = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSST2, &state, AW_I2C_BYTE_ONE);
	state &= AW8692X_BIT_SYSST2_FF_EMPTY;
	return state;
}

static int aw8692x_select_d2s_gain(uint8_t reg)
{
	int d2s_gain = 0;

	switch (reg) {
	case AW8692X_BIT_DETCFG2_D2S_GAIN_1:
		d2s_gain = 1;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_2:
		d2s_gain = 2;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_4:
		d2s_gain = 4;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_8:
		d2s_gain = 8;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_10:
		d2s_gain = 10;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_16:
		d2s_gain = 16;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_20:
		d2s_gain = 20;
		break;
	case AW8692X_BIT_DETCFG2_D2S_GAIN_40:
		d2s_gain = 40;
		break;
	default:
		d2s_gain = -1;
		break;
	}
	return d2s_gain;
}

static void aw8692x_get_lra_resistance(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;
	uint8_t reg_array[2] = {0};
	uint8_t adc_fs_default = 0;
	uint8_t d2s_gain = 0;
	uint32_t lra_code = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw8692x_ram_init(aw_haptic, true);
	aw8692x_stop(aw_haptic);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG2,
			AW8692X_BIT_DETCFG2_DET_SEQ0_MASK,
			AW8692X_BIT_DETCFG2_DET_SEQ0_RL);
	hv_i2c_r_bytes(aw_haptic,  AW8692X_REG_DETCFG1, &reg_val, AW_I2C_BYTE_ONE);
	adc_fs_default = reg_val & AW8692X_BIT_DETCFG1_ADC_FS;
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG1,
			(AW8692X_BIT_DETCFG1_ADC_FS_MASK &
			AW8692X_BIT_DETCFG1_DET_GO_MASK),
			(AW8692X_BIT_DETCFG1_ADC_FS_96KHZ |
			AW8692X_BIT_DETCFG1_DET_GO_DET_SEQ0));
	usleep_range(3000, 3500);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG1,
			AW8692X_BIT_DETCFG1_DET_GO_MASK,
			AW8692X_BIT_DETCFG1_DET_GO_NA);
	/* restore default config */
	aw8692x_ram_init(aw_haptic, false);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG1,
			AW8692X_BIT_DETCFG1_ADC_FS_MASK, adc_fs_default);
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_DETCFG2, &reg_val, AW_I2C_BYTE_ONE);
	reg_val &= AW8692X_BIT_DETCFG2_D2S_GAIN;
	d2s_gain = aw8692x_select_d2s_gain(reg_val);
	if (d2s_gain < 0) {
		aw_err("d2s_gain is error");
		return;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_DETRD1, reg_array, AW_I2C_BYTE_TWO);
	lra_code = ((reg_array[0] & AW8692X_BIT_DETRD1_AVG_DATA) << 8) +
														reg_array[1];
	aw_haptic->lra = AW8692X_LRA_FORMULA(lra_code, d2s_gain);
}


static void aw8692x_set_repeat_seq(struct aw_haptic *aw_haptic, uint8_t seq)
{
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw8692x_set_wav_seq(aw_haptic, 0x00, seq);
	aw8692x_set_wav_loop(aw_haptic, 0x00, AW8692X_WAVLOOP_INIFINITELY);
}

static void aw8692x_get_vbat(struct aw_haptic *aw_haptic)
{
	uint8_t reg_array[2] = {0};
	uint32_t vbat_code = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	aw8692x_stop(aw_haptic);
	aw8692x_ram_init(aw_haptic, true);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG2,
			AW8692X_BIT_DETCFG2_DET_SEQ0_MASK,
			AW8692X_BIT_DETCFG2_DET_SEQ0_VBAT);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG1,
			AW8692X_BIT_DETCFG1_DET_GO_MASK,
			AW8692X_BIT_DETCFG1_DET_GO_DET_SEQ0);
	usleep_range(3000, 3500);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG1,
			AW8692X_BIT_DETCFG1_DET_GO_MASK,
			AW8692X_BIT_DETCFG1_DET_GO_NA);
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_DETRD1, reg_array, AW_I2C_BYTE_TWO);
	vbat_code = ((reg_array[0] & AW8692X_BIT_DETRD1_AVG_DATA) << 8) +
														reg_array[1];
	aw_haptic->vbat = AW8692X_VBAT_FORMULA(vbat_code);

	if (aw_haptic->vbat > AW8692X_VBAT_MAX) {
		aw_haptic->vbat = AW8692X_VBAT_MAX;
		aw_info("vbat max limit = %d", aw_haptic->vbat);
	}
	if (aw_haptic->vbat < AW_VBAT_MIN) {
		aw_haptic->vbat = AW_VBAT_MIN;
		aw_info("vbat min limit = %d", aw_haptic->vbat);
	}
	aw_info("awinic->vbat=%dmV, vbat_code=0x%02X",
		aw_haptic->vbat, vbat_code);
	aw8692x_ram_init(aw_haptic, false);
}

static ssize_t aw8692x_get_reg(struct aw_haptic *aw_haptic, ssize_t len,
							char *buf)
{
	uint8_t i = 0;
	uint8_t reg_array[AW8692X_REG_ANACFG22 + 1] = {0};

	if (!aw_haptic || !buf) {
		aw_err("aw_haptic or buf is null");
		return -EINVAL;
	}
	for (i = 0; i < AW8692X_REG_WAVCFG1; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_RSTCFG, reg_array,
									AW8692X_REG_WAVCFG1);
	for (i = AW8692X_REG_WAVCFG1; i <= AW8692X_REG_WAVCFG13; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_WAVCFG1,
					&reg_array[AW8692X_REG_WAVCFG1],
					AW8692X_REG_WAVCFG13 - AW8692X_REG_WAVCFG1);
	for (i = AW8692X_REG_CONTCFG1; i < AW8692X_REG_CONTCFG17; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_CONTCFG1,
					&reg_array[AW8692X_REG_CONTCFG1],
					AW8692X_REG_CONTCFG17 - AW8692X_REG_CONTCFG1);
	for (i = AW8692X_REG_CONTCFG17; i < AW8692X_REG_RTPDATA; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_CONTCFG17,
					&reg_array[AW8692X_REG_CONTCFG17],
					AW8692X_REG_RTPDATA - AW8692X_REG_CONTCFG17);
	for (i = AW8692X_REG_RTPDATA + 1; i < AW8692X_REG_RAMDATA; i++)
		hv_i2c_r_bytes(aw_haptic, (AW8692X_REG_RTPDATA + 1),
					&reg_array[AW8692X_REG_RTPDATA + 1],
					(AW8692X_REG_RAMDATA - AW8692X_REG_RTPDATA - 1));
	for (i = AW8692X_REG_RAMDATA + 1; i < AW8692X_REG_TRIMCFG1; i++)
		hv_i2c_r_bytes(aw_haptic, (AW8692X_REG_RAMDATA + 1),
					&reg_array[AW8692X_REG_RAMDATA + 1],
					(AW8692X_REG_TRIMCFG1 - AW8692X_REG_RAMDATA - 1));
	for (i = AW8692X_REG_TRIMCFG1; i <= AW8692X_REG_EFCFG6; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_TRIMCFG1,
					&reg_array[AW8692X_REG_TRIMCFG1],
					(AW8692X_REG_EFCFG6 - AW8692X_REG_TRIMCFG1));
	for (i = AW8692X_REG_ANACFG12; i <= AW8692X_REG_ANACFG22; i++)
		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_ANACFG12,
					&reg_array[AW8692X_REG_ANACFG12],
					(AW8692X_REG_ANACFG22 - AW8692X_REG_ANACFG12));
	for (i = 0; i <= AW8692X_REG_ANACFG22; i++)
		if ((i != AW8692X_REG_RTPDATA) && (i != AW8692X_REG_RAMDATA))
			len += snprintf(buf + len, PAGE_SIZE - len,
					"reg:0x%02X=0x%02X\n", i, reg_array[i]);
	return len;
}

static void aw8692x_offset_cali(struct aw_haptic *aw_haptic)
{
}

static void aw8692x_trig_init(struct aw_haptic *aw_haptic)
{
	aw_info("enter!");
	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	if (aw_haptic->info.is_enabled_one_wire) {
		aw_info("one wire is enabled!");
		aw8692x_one_wire_init(aw_haptic);
	} else {
		aw8692x_trig1_param_init(aw_haptic);
		aw8692x_trig1_param_config(aw_haptic);
	}
	aw8692x_trig2_param_init(aw_haptic);
	aw8692x_trig3_param_init(aw_haptic);
	aw8692x_trig2_param_config(aw_haptic);
	aw8692x_trig3_param_config(aw_haptic);
}

#ifdef AW_CHECK_RAM_DATA
static int aw8692x_check_ram_data(struct aw_haptic *aw_haptic,
								uint8_t *cont_data,
								uint8_t *ram_data, uint32_t len)
{
	int i = 0;

	if (!cont_data || !ram_data) {
		aw_err("cont or ram is null");
		return -EINVAL;
	}
	for (i = 0; i < len; i++) {
		if (ram_data[i] != cont_data[i]) {
			aw_err("check ramdata error, addr=0x%04x, ram_data=0x%02x, file_data=0x%02x",
				i, ram_data[i], cont_data[i]);
			return -ERANGE;
		}
	}
	return 0;
}
#endif

static int aw8692x_container_update(struct aw_haptic *aw_haptic,
						struct aw_haptic_container *awinic_cont)
{
	uint8_t ae_addr_h = 0;
	uint8_t af_addr_h = 0;
	uint8_t ae_addr_l = 0;
	uint8_t af_addr_l = 0;
	uint8_t reg_array[3] = {0};
	uint32_t base_addr = 0;
	uint32_t shift = 0;
	int i = 0;
	int len = 0;
	int ret = 0;

#ifdef AW_CHECK_RAM_DATA
	uint8_t ram_data[AW_RAMDATA_RD_BUFFER_SIZE] = {0};
#endif
	aw_info("enter");
	if (!aw_haptic || !awinic_cont) {
		aw_err("aw_haptic or cont is null");
		return -EINVAL;
	}
	mutex_lock(&aw_haptic->lock);
	aw_haptic->ram.baseaddr_shift = 2;
	aw_haptic->ram.ram_shift = 4;
	/* RAMINIT Enable */
	aw8692x_ram_init(aw_haptic, true);
	/* Enter standby mode */
	aw8692x_stop(aw_haptic);
	/* base addr */
	shift = aw_haptic->ram.baseaddr_shift;
	aw_haptic->ram.base_addr =
		(uint32_t)((awinic_cont->data[0 + shift] << 8) |
		(awinic_cont->data[1 + shift]));
	base_addr = aw_haptic->ram.base_addr;
	aw_info("base_addr = %d", aw_haptic->ram.base_addr);

	/* set FIFO_AE and FIFO_AF addr */
	ae_addr_h = (uint8_t)AW8692X_FIFO_AE_ADDR_H(base_addr);
	af_addr_h = (uint8_t)AW8692X_FIFO_AF_ADDR_H(base_addr);
	reg_array[0] = ae_addr_h | af_addr_h;
	reg_array[1] = (uint8_t)AW8692X_FIFO_AE_ADDR_L(base_addr);
	reg_array[2] = (uint8_t)AW8692X_FIFO_AF_ADDR_L(base_addr);
	aw_info("write to RTPCFG3");
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_RTPCFG3, reg_array,
				AW_I2C_BYTE_THREE);

	/* get FIFO_AE and FIFO_AF addr */
	aw_info("get FIFO_AE and FIFO_AF addr");
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_RTPCFG3, reg_array,
				AW_I2C_BYTE_THREE);
	ae_addr_h = ((reg_array[0]) & AW8692X_BIT_RTPCFG3_FIFO_AEH) >> 4;
	ae_addr_l = reg_array[1];
	aw_info("almost_empty_threshold = %d",
		(uint16_t)((ae_addr_h << 8) | ae_addr_l));
	af_addr_h = ((reg_array[0]) & AW8692X_BIT_RTPCFG3_FIFO_AFH);
	af_addr_l = reg_array[2];
	aw_info("almost_full_threshold = %d",
		(uint16_t)((af_addr_h << 8) | af_addr_l));

	aw8692x_set_base_addr(aw_haptic);
	aw8692x_set_ram_addr(aw_haptic);
	i = aw_haptic->ram.ram_shift;
	while (i < awinic_cont->len) {
		if ((awinic_cont->len - i) < AW_RAMDATA_WR_BUFFER_SIZE)
			len = awinic_cont->len - i;
		else
			len = AW_RAMDATA_WR_BUFFER_SIZE;

		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_RAMDATA,
				&awinic_cont->data[i], len);
		i += len;
	}

#ifdef AW_CHECK_RAM_DATA
	aw8692x_set_ram_addr(aw_haptic);
	i = aw_haptic->ram.ram_shift;
	while (i < awinic_cont->len) {
		if ((awinic_cont->len - i) < AW_RAMDATA_RD_BUFFER_SIZE)
			len = awinic_cont->len - i;
		else
			len = AW_RAMDATA_RD_BUFFER_SIZE;

		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_RAMDATA, ram_data, len);
		ret = aw8692x_check_ram_data(aw_haptic, &awinic_cont->data[i],
									ram_data, len);
		if (ret < 0)
			break;
		i += len;
	}
	if (ret)
		aw_err("ram data check sum error");
	else
		aw_info("ram data check sum pass");
#endif
	/* RAMINIT Disable */
	aw8692x_ram_init(aw_haptic, false);
	mutex_unlock(&aw_haptic->lock);
	return ret;
}

static uint64_t aw8692x_get_theory_time(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;
	uint32_t fre_val = 0;
	uint64_t theory_time = 0;

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return -EINVAL;
	}
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_SYSCTRL2, &reg_val, AW_I2C_BYTE_ONE);
	fre_val = reg_val & AW8692X_BIT_SYSCTRL2_RCK_FRE;
	if (fre_val == AW8692X_RCK_FRE_24K)
		theory_time = (aw_haptic->rtp_len / 24000) * 1000000;	/* 24K */
	if (fre_val == AW8692X_RCK_FRE_48K)
		theory_time = (aw_haptic->rtp_len / 48000) * 1000000;	/* 48K */
	aw_info("microsecond:%llu  theory_time = %llu",
		aw_haptic->microsecond, theory_time);
	return theory_time;
}

static void aw8692x_parse_dt(struct device *dev, struct aw_haptic *aw_haptic,
							struct device_node *np)
{
	uint8_t duration_time[3];
	uint8_t trig_config_temp[24] = {1, 0, 1, 1, 1, 2, 0, 0,
									1, 0, 0, 1, 0, 2, 0, 0,
									1, 0, 0, 1, 0, 2, 0, 0};
	uint32_t val = 0;

	if (!aw_haptic || !np) {
		aw_err("aw_haptic or np is null");
		return;
	}
	val = of_property_read_u8(np, "aw8692x_max_bst_vol",
							&aw_haptic->info.max_bst_vol);
	if (val != 0)
		aw_info("aw8692x_max_bst_vol not found");
	val = of_property_read_u32(np, "f0_pre", &aw_haptic->info.f0_pre);
	if (val != 0)
		aw_info("vib_f0_pre not found");
	val = of_property_read_u8(np, "f0_cali_percent",
					&aw_haptic->info.f0_cali_percent);
	if (val != 0)
		aw_info("vib_f0_cali_percent not found");
	val = of_property_read_u8(np, "aw8692x_cont_drv1_lvl",
							&aw_haptic->info.cont_drv1_lvl);
	if (val != 0)
		aw_info("vib_cont_drv1_lvl not found");
	val = of_property_read_u8(np, "aw8692x_cont_drv2_lvl",
							&aw_haptic->info.cont_drv2_lvl);
	if (val != 0)
		aw_info("vib_cont_drv2_lvl not found");
	val = of_property_read_u8(np, "aw8692x_cont_drv1_time",
							&aw_haptic->info.cont_drv1_time);
	if (val != 0)
		aw_info("vib_cont_drv1_time not found");
	val = of_property_read_u8(np, "aw8692x_cont_drv2_time",
							&aw_haptic->info.cont_drv2_time);
	if (val != 0)
		aw_info("vib_cont_drv2_time not found");
	val = of_property_read_u8(np, "aw8692x_cont_drv_width",
							&aw_haptic->info.cont_drv_width);
	if (val != 0)
		aw_info("vib_cont_drv_width not found");
	val = of_property_read_u8(np, "aw8692x_cont_wait_num",
							&aw_haptic->info.cont_wait_num);
	if (val != 0)
		aw_info("vib_cont_wait_num not found");
	val = of_property_read_u8(np, "aw8692x_cont_brk_time",
							&aw_haptic->info.cont_brk_time);
	if (val != 0)
		aw_info("vib_cont_brk_time not found");
	val = of_property_read_u8(np, "aw8692x_cont_track_margin",
							&aw_haptic->info.cont_track_margin);
	if (val != 0)
		aw_info("vib_cont_track_margin not found");
	val = of_property_read_u8(np, "aw8692x_brk_bst_md",
							&aw_haptic->info.brk_bst_md);
	if (val != 0)
		aw_info("vib_brk_bst_md not found");
	val = of_property_read_u8(np, "aw8692x_cont_tset",
							&aw_haptic->info.cont_tset);
	if (val != 0)
		aw_info("vib_cont_tset not found");
	val = of_property_read_u8(np, "aw8692x_cont_bemf_set",
							&aw_haptic->info.cont_bemf_set);
	if (val != 0)
		aw_info("vib_cont_bemf_set not found");
	val = of_property_read_u8(np, "aw8692x_cont_bst_brk_gain",
							&aw_haptic->info.cont_bst_brk_gain);
	if (val != 0)
		aw_info("vib_cont_bst_brk_gain not found");
	val = of_property_read_u8(np, "aw8692x_cont_brk_gain",
							&aw_haptic->info.cont_brk_gain);
	if (val != 0)
		aw_info("vib_cont_brk_gain not found");
	val = of_property_read_u8(np, "aw8692x_d2s_gain",
							&aw_haptic->info.d2s_gain);
	if (val != 0)
		aw_info("vib_d2s_gain not found");
	val = of_property_read_u8_array(np, "aw8692x_duration_time",
					duration_time,
					ARRAY_SIZE(duration_time));
	if (val != 0)
		aw_info("aw8692x_duration_time not found");
	else
		memcpy(aw_haptic->info.duration_time, duration_time,
			sizeof(duration_time));
	val = of_property_read_u8_array(np, "aw8692x_trig_config",
					trig_config_temp,
					ARRAY_SIZE(trig_config_temp));
	if (val != 0)
		aw_info("vib_trig_config not found");
	else
		memcpy(aw_haptic->info.trig_cfg, trig_config_temp,
				sizeof(trig_config_temp));
	val = of_property_read_u8(np, "aw8692x_bst_vol_default",
							&aw_haptic->info.bst_vol_default);
	if (val != 0)
		aw_info("vib_bst_vol_default not found");
	val = of_property_read_u8(np, "aw8692x_bst_vol_ram",
							&aw_haptic->info.bst_vol_ram);
	if (val != 0)
		aw_info("vib_bst_vol_ram not found");
	val = of_property_read_u8(np, "aw8692x_bst_vol_rtp",
							&aw_haptic->info.bst_vol_rtp);
	if (val != 0)
		aw_info("vib_bst_vol_rtp not found");
	aw_haptic->info.is_enabled_auto_bst =
		of_property_read_bool(np, "aw8692x_is_enabled_auto_bst");
	aw_info("aw_haptic->info.is_enabled_auto_bst = %d",
		aw_haptic->info.is_enabled_auto_bst);
	aw_haptic->info.is_enabled_one_wire = of_property_read_bool(np,
									"aw8692x_is_enabled_one_wire");
	aw_info("aw_haptic->info.is_enabled_one_wire = %d",
		aw_haptic->info.is_enabled_one_wire);
	aw_haptic->info.is_enabled_lowpower_bst_config = of_property_read_bool(np,
		"aw8692x_vib_lowpower_bst_config");
	aw_info("aw_haptic->info.is_enabled_lowpower_bst_config = %d",
		aw_haptic->info.is_enabled_lowpower_bst_config);
	aw_haptic->info.is_enabled_richtap_core = of_property_read_bool(np,
		"aw8692x_is_enabled_richtap_core");
	aw_info("aw_haptic->info.aw8692x_is_enabled_richtap_core = %u",
		aw_haptic->info.is_enabled_richtap_core);
}

static void aw8692x_misc_para_init(struct aw_haptic *aw_haptic)
{
	uint8_t reg_val = 0;
	uint8_t reg_array[8] = {0};

	if (!aw_haptic) {
		aw_err("aw_haptic is null");
		return;
	}
	reg_val = AW_I2C_ADDR;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_SYSCTRL5, &reg_val, AW_I2C_BYTE_ONE);
	/* Get vamx and gain */
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_PLAYCFG1,
			reg_array, AW_I2C_BYTE_TWO);
	if (aw_haptic->info.bst_vol_default > 0)
		aw_haptic->vmax = aw_haptic->info.bst_vol_default;
	else
		aw_haptic->vmax = reg_array[0] &
			AW8692X_BIT_PLAYCFG1_BST_VOUT_VREFSET;
	aw_haptic->gain = reg_array[1];
	/* Get wave_seq */
	hv_i2c_r_bytes(aw_haptic, AW8692X_REG_WAVCFG1,
				reg_array, AW_I2C_BYTE_EIGHT);
	aw_haptic->index = reg_array[0];
	memcpy(aw_haptic->seq, reg_array, AW_SEQUENCER_SIZE);
	/* Unlock register */
	reg_val = AW8692X_BIT_TMCFG_TM_UNLOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);
	/* Close boost skip */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG12,
			AW8692X_BIT_ANACFG12_BST_SKIP_MASK,
			AW8692X_BIT_ANACFG12_BST_SKIP_SHUTDOWN);
	/* Open adaptive ipeak current limiting */
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG15,
			AW8692X_BIT_ANACFG15_BST_PEAK_MODE_MASK,
			AW8692X_BIT_ANACFG15_BST_PEAK_BACK);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_ANACFG16,
			AW8692X_BIT_ANACFG16_BST_SRC_MASK,
			AW8692X_BIT_ANACFG16_BST_SRC_3NS);
	reg_val = AW8692X_BIT_PWMCFG1_INIT_VAL;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_PWMCFG1, &reg_val, AW_I2C_BYTE_ONE);
	/* lock register */
	reg_val = AW8692X_BIT_TMCFG_TM_LOCK;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_TMCFG, &reg_val, AW_I2C_BYTE_ONE);

	if (!aw_haptic->info.brk_bst_md)
		aw_err("aw_haptic->info.brk_bst_md = 0!");
	if (!aw_haptic->info.cont_brk_time)
		aw_err("aw_haptic->info.cont_brk_time = 0!");
	if (!aw_haptic->info.cont_tset)
		aw_err("aw_haptic->info.cont_tset = 0!");
	if (!aw_haptic->info.cont_bemf_set)
		aw_err("aw_haptic->info.cont_bemf_set = 0!");
	if (!aw_haptic->info.cont_bst_brk_gain)
		aw_err("aw_haptic->info.cont_bst_brk_gain = 0");
	if (!aw_haptic->info.cont_brk_gain)
		aw_err("aw_haptic->info.cont_brk_gain = 0!");
	if (!aw_haptic->info.d2s_gain)
		aw_err("aw_haptic->info.d2s_gain = 0!");

	hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG1,
			AW8692X_BIT_CONTCFG1_BRK_BST_MD_MASK,
			(aw_haptic->info.brk_bst_md << 6));
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG5,
			AW8692X_BIT_CONTCFG5_BST_BRK_GAIN_MASK &
			AW8692X_BIT_CONTCFG5_BRK_GAIN_MASK,
			(aw_haptic->info.cont_bst_brk_gain << 4) |
			aw_haptic->info.cont_brk_gain);
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG10,
			&aw_haptic->info.cont_brk_time, AW_I2C_BYTE_ONE);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_CONTCFG13,
			AW8692X_BIT_CONTCFG13_TSET_MASK &
			AW8692X_BIT_CONTCFG13_BEME_SET_MASK,
			(aw_haptic->info.cont_tset << 4) |
			aw_haptic->info.cont_bemf_set);
	hv_i2c_w_bits(aw_haptic, AW8692X_REG_DETCFG2,
			AW8692X_BIT_DETCFG2_D2S_GAIN_MASK,
			aw_haptic->info.d2s_gain);
}

/******************************************************
 *
 * Extern function : sysfs attr
 *
 ******************************************************/
static ssize_t aw8692x_bst_vol_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"bst_vol_ram=%d, bst_vol_rtp=%d\n",
			aw_haptic->info.bst_vol_ram,
			aw_haptic->info.bst_vol_rtp);
	return len;
}

static ssize_t aw8692x_bst_vol_store(struct device *dev,
							struct device_attribute *attr,
							const char *buf, size_t count)
{
	uint32_t databuf[2] = { 0, 0 };
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	if (sscanf(buf, "%d %d", &databuf[0], &databuf[1]) == 2) {
		aw_haptic->info.bst_vol_ram = databuf[0];
		aw_haptic->info.bst_vol_rtp = databuf[1];
	}
	return count;
}

static ssize_t aw8692x_cont_wait_num_show(struct device *dev,
								struct device_attribute *attr,
								char *buf)
{
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"cont_wait_num = 0x%02X\n",
			aw_haptic->info.cont_wait_num);
	return len;
}

static ssize_t aw8692x_cont_wait_num_store(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t count)
{
	int rc = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;
	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	rc = kstrtou8(buf, 0, &aw_haptic->info.cont_wait_num);
	if (rc < 0)
		return rc;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG4,
				&aw_haptic->info.cont_wait_num, AW_I2C_BYTE_ONE);

	return count;
}

static ssize_t aw8692x_cont_drv_lvl_show(struct device *dev,
								struct device_attribute *attr,
								char *buf)
{
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;
	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"cont_drv1_lvl = 0x%02X, cont_drv2_lvl = 0x%02X\n",
			aw_haptic->info.cont_drv1_lvl,
			aw_haptic->info.cont_drv2_lvl);
	return len;
}

static ssize_t aw8692x_cont_drv_lvl_store(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t count)
{
	uint32_t databuf[2] = {0};
	uint8_t reg_array[2] = {0};
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		aw_haptic->info.cont_drv1_lvl = databuf[0];
		aw_haptic->info.cont_drv2_lvl = databuf[1];

		hv_i2c_r_bytes(aw_haptic, AW8692X_REG_CONTCFG6, reg_array,
					AW_I2C_BYTE_ONE);
		reg_array[0] &= AW8692X_BIT_CONTCFG6_DRV1_LVL_MASK;
		reg_array[0] |= aw_haptic->info.cont_drv1_lvl;
		reg_array[1] = aw_haptic->info.cont_drv2_lvl;
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG6, reg_array,
					AW_I2C_BYTE_TWO);
	}
	return count;
}

static ssize_t aw8692x_cont_drv_time_show(struct device *dev,
								struct device_attribute *attr,
								char *buf)
{
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	len += snprintf(buf + len, PAGE_SIZE - len,
			"cont_drv1_time = 0x%02X, cont_drv2_time = 0x%02X\n",
			aw_haptic->info.cont_drv1_time,
			aw_haptic->info.cont_drv2_time);
	return len;
}

static ssize_t aw8692x_cont_drv_time_store(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t count)
{
	uint8_t reg_array[2] = {0};
	uint32_t databuf[2] = {0};
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	if (sscanf(buf, "%x %x", &databuf[0], &databuf[1]) == 2) {
		aw_haptic->info.cont_drv1_time = databuf[0];
		aw_haptic->info.cont_drv2_time = databuf[1];
		reg_array[0] = (uint8_t)aw_haptic->info.cont_drv1_time;
		reg_array[1] = (uint8_t)aw_haptic->info.cont_drv2_time;
		hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG8, reg_array,
					AW_I2C_BYTE_TWO);
	}
	return count;
}

static ssize_t aw8692x_cont_brk_time_show(struct device *dev,
								struct device_attribute *attr,
								char *buf)
{
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	len += snprintf(buf + len, PAGE_SIZE - len, "cont_brk_time = 0x%02X\n",
			aw_haptic->info.cont_brk_time);
	return len;
}

static ssize_t aw8692x_cont_brk_time_store(struct device *dev,
								struct device_attribute *attr,
								const char *buf, size_t count)
{
	int rc = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);
	rc = kstrtou8(buf, 0, &aw_haptic->info.cont_brk_time);
	if (rc < 0)
		return rc;
	hv_i2c_w_bytes(aw_haptic, AW8692X_REG_CONTCFG10,
		    &aw_haptic->info.cont_brk_time, AW_I2C_BYTE_ONE);
	return count;
}

static ssize_t aw8692x_trig_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	uint8_t i = 0;
	ssize_t len = 0;
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return len;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return len;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);
	for (i = 0; i < AW_TRIG_NUM; i++) {
		len += snprintf(buf + len, PAGE_SIZE - len,
				"trig%d: trig_level=%d, trig_polar=%d, pos_enable=%d, pos_sequence=%d, \
				neg_enable=%d, neg_sequence=%d trig_brk=%d, trig_bst=%d\n",
				i + 1,
				aw_haptic->trig[i].trig_level,
				aw_haptic->trig[i].trig_polar,
				aw_haptic->trig[i].pos_enable,
				aw_haptic->trig[i].pos_sequence,
				aw_haptic->trig[i].neg_enable,
				aw_haptic->trig[i].neg_sequence,
				aw_haptic->trig[i].trig_brk,
				aw_haptic->trig[i].trig_bst);
	}

	return len;
}

static ssize_t aw8692x_trig_store(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	uint32_t databuf[9] = { 0 };
	cdev_t *cdev = NULL;
	struct aw_haptic *aw_haptic = NULL;

	if (!dev || !buf) {
		aw_err ("dev or buf is null");
		return -EINVAL;
	}
	cdev = dev_get_drvdata(dev);
	if (!cdev) {
		aw_err ("cdev is null");
		return -EINVAL;
	}
	aw_haptic = container_of(cdev, struct aw_haptic, vib_dev);

	if (sscanf(buf, "%d %d %d %d %d %d %d %d %d", &databuf[0], &databuf[1],
			&databuf[2], &databuf[3], &databuf[4], &databuf[5],
			&databuf[6], &databuf[7], &databuf[8]) == 9) {
		aw_info("%d, %d, %d, %d, %d, %d, %d, %d, %d",
			databuf[0], databuf[1], databuf[2], databuf[3],
			databuf[4], databuf[5], databuf[6], databuf[7],
			databuf[8]);
		if (databuf[0] < 1 || databuf[0] > 3) {
			aw_info("input trig_num out of range!");
			return count;
		}
		if (databuf[0] == 1 && aw_haptic->info.is_enabled_one_wire) {
			aw_info("trig1 pin used for one wire!");
			return count;
		}
		if (!aw_haptic->ram_init) {
			aw_err("ram init failed, not allow to play!");
			return count;
		}
		if (databuf[4] > aw_haptic->ram.ram_num ||
			databuf[6] > aw_haptic->ram.ram_num) {
			aw_err("input seq value out of range!");
			return count;
		}
		databuf[0] -= 1;

		aw_haptic->trig[databuf[0]].trig_level = databuf[1];
		aw_haptic->trig[databuf[0]].trig_polar = databuf[2];
		aw_haptic->trig[databuf[0]].pos_enable = databuf[3];
		aw_haptic->trig[databuf[0]].pos_sequence = databuf[4];
		aw_haptic->trig[databuf[0]].neg_enable = databuf[5];
		aw_haptic->trig[databuf[0]].neg_sequence = databuf[6];
		aw_haptic->trig[databuf[0]].trig_brk = databuf[7];
		aw_haptic->trig[databuf[0]].trig_bst = databuf[8];
		mutex_lock(&aw_haptic->lock);
		switch (databuf[0]) {
		case AW8692X_TRIG1:
			aw8692x_trig1_param_config(aw_haptic);
			break;
		case AW8692X_TRIG2:
			aw8692x_trig2_param_config(aw_haptic);
			break;
		case AW8692X_TRIG3:
			aw8692x_trig3_param_config(aw_haptic);
			break;
		}
		mutex_unlock(&aw_haptic->lock);
	}
	return count;
}

static DEVICE_ATTR(bst_vol, S_IWUSR | S_IRUGO, aw8692x_bst_vol_show,
				aw8692x_bst_vol_store);
static DEVICE_ATTR(cont_wait_num, S_IWUSR | S_IRUGO, aw8692x_cont_wait_num_show,
				aw8692x_cont_wait_num_store);
static DEVICE_ATTR(cont_drv_lvl, S_IWUSR | S_IRUGO, aw8692x_cont_drv_lvl_show,
				aw8692x_cont_drv_lvl_store);
static DEVICE_ATTR(cont_drv_time, S_IWUSR | S_IRUGO, aw8692x_cont_drv_time_show,
				aw8692x_cont_drv_time_store);
static DEVICE_ATTR(cont_brk_time, S_IWUSR | S_IRUGO, aw8692x_cont_brk_time_show,
				aw8692x_cont_brk_time_store);
static DEVICE_ATTR(trig, S_IWUSR | S_IRUGO, aw8692x_trig_show,
				aw8692x_trig_store);

static struct attribute *aw8692x_vibrator_attributes[] = {
	&dev_attr_bst_vol.attr,
	&dev_attr_cont_wait_num.attr,
	&dev_attr_cont_drv_lvl.attr,
	&dev_attr_cont_drv_time.attr,
	&dev_attr_cont_brk_time.attr,
	&dev_attr_trig.attr,
	NULL
};

static struct attribute_group aw8692x_vibrator_attribute_group = {
	.attrs = aw8692x_vibrator_attributes
};

static int aw8692x_creat_node(struct aw_haptic *aw_haptic)
{
	int ret = 0;

	ret = sysfs_create_group(&aw_haptic->vib_dev.dev->kobj,
						&aw8692x_vibrator_attribute_group);
	if (ret < 0) {
		aw_err("error create aw8692x sysfs attr files");
		return ret;
	}
	return 0;
}

struct aw_haptic_func aw8692x_func_list = {
	.play_stop = aw8692x_stop,
	.ram_init = aw8692x_ram_init,
	.get_vbat = aw8692x_get_vbat,
	.creat_node = aw8692x_creat_node,
	.get_f0 = aw8692x_get_f0,
	.cont_config = aw8692x_cont_config,
	.offset_cali = aw8692x_offset_cali,
	.read_f0 = aw8692x_read_f0,
	.get_irq_state = aw8692x_get_irq_state,
	.check_qualify = aw8692x_check_qualify,
	.juge_rtp_going = aw8692x_juge_rtp_going,
	.set_bst_peak_cur = aw8692x_set_bst_peak_cur,
	.get_theory_time = aw8692x_get_theory_time,
	.get_lra_resistance = aw8692x_get_lra_resistance,
	.set_pwm = aw8692x_set_pwm,
	.play_mode = aw8692x_play_mode,
	.set_bst_vol = aw8692x_set_bst_vol,
	.interrupt_setup = aw8692x_interrupt_setup,
	.set_repeat_seq = aw8692x_set_repeat_seq,
	.auto_bst_enable = aw8692x_auto_bst_enable,
	.vbat_mode_config = aw8692x_vbat_mode_config,
	.set_wav_seq = aw8692x_set_wav_seq,
	.set_wav_loop = aw8692x_set_wav_loop,
	.set_ram_addr = aw8692x_set_ram_addr,
	.set_rtp_data = aw8692x_set_rtp_data,
	.container_update = aw8692x_container_update,
	.protect_config = aw8692x_protect_config,
	.parse_dt = aw8692x_parse_dt,
	.trig_init = aw8692x_trig_init,
	.irq_clear = aw8692x_irq_clear,
	.get_wav_loop = aw8692x_get_wav_loop,
	.play_go = aw8692x_play_go,
	.misc_para_init = aw8692x_misc_para_init,
	.set_rtp_aei = aw8692x_set_rtp_aei,
	.set_gain = aw8692x_set_gain,
	.upload_lra = aw8692x_upload_lra,
	.bst_mode_config = aw8692x_bst_mode_config,
	.get_reg = aw8692x_get_reg,
	.get_prctmode = aw8692x_get_prctmode,
	.get_trim_lra = aw8692x_get_trim_lra,
	.get_ram_data = aw8692x_get_ram_data,
	.get_first_wave_addr = aw8692x_get_first_wave_addr,
	.get_glb_state = aw8692x_get_glb_state,
	.get_osc_status = aw8692x_get_osc_status,
	.rtp_get_fifo_afs = aw8692x_rtp_get_fifo_afs,
	.get_wav_seq = aw8692x_get_wav_seq,
	.enable_gain = aw8692x_enable_gain,
	.rtp_get_fifo_afi = aw8692x_rtp_get_fifo_afi,
};

struct aw_haptic_func *aw8692x_func(void)
{
	return &aw8692x_func_list;
}
EXPORT_SYMBOL(aw8692x_func);

MODULE_LICENSE("GPL v2");