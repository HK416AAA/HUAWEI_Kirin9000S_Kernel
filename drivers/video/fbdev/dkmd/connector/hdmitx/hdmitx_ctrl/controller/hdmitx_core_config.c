/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "dkmd_log.h"
#include "hdmitx_ctrl_reg.h"
#include "hdmitx_video_path_reg.h"
#include "hdmitx_reg.h"
#include "hdmitx_aon_reg.h"
#include "hdmitx_ctrl.h"
#include "hdmitx_infoframe.h"
#include "hdmitx_ddc_config.h"
#include "hdmitx_common.h"
#include "hdmitx_emp.h"
#include "hdmitx_core_config.h"

#define INFOFRAME_PKT_SIZE 31
#define REG_TP_MEMCTRL 0x3C
#define REG_SP_MEMCTRL 0x40
#define REG_AUD_ACR_CTRL 0x1040
#define REG_ACR_CTS_INIT_MASK (0xf << 0)
#define RESET_REQ_ALL 0x8
#define GT_CLK_HDMI_HDCP_MASK BIT(14)

struct infoframe_offset {
	u32 head;
	u32 pkt_l;
	u32 pkt_h;
	u32 cfg;
};

static void core_get_infoframe(const u8 *buffer, struct infoframe_offset *offset)
{
	switch (*buffer) {
	case HDMITX_INFOFRAME_TYPE_AVI:
		offset->head = REG_AVI_PKT_HEADER;
		offset->pkt_l = REG_AVI_SUB_PKT0_L;
		offset->pkt_h = REG_AVI_SUB_PKT0_H;
		offset->cfg = REG_CEA_AVI_CFG;
		break;

	case HDMITX_INFOFRAME_TYPE_AUDIO:
		offset->head = REG_AIF_PKT_HEADER;
		offset->pkt_l = REG_AIF_SUB_PKT0_L;
		offset->pkt_h = REG_AIF_SUB_PKT0_H;
		offset->cfg = REG_CEA_AUD_CFG;
		break;

	case HDMITX_INFOFRAME_TYPE_VENDOR:
		offset->head = REG_VSIF_PKT_HEADER;
		offset->pkt_l = REG_VSIF_SUB_PKT0_L;
		offset->pkt_h = REG_VSIF_SUB_PKT0_H;
		offset->cfg = REG_CEA_VSIF_CFG;
		break;

	case HDMITX_PACKET_TYPE_GAMUT_METADATA:
		offset->head = REG_GAMUT_PKT_HEADER;
		offset->pkt_l = REG_GAMUT_SUB_PKT0_L;
		offset->pkt_h = REG_GAMUT_SUB_PKT0_H;
		offset->cfg = REG_CEA_GAMUT_CFG;
		dpu_pr_info("gamut type");
		break;
	case HDMITX_INFOFRAME_TYPE_DRM:
		offset->head = REG_GEN_PKT_HEADER;
		offset->pkt_l = REG_GEN_SUB_PKT0_L;
		offset->pkt_h = REG_GEN_SUB_PKT0_H;
		offset->cfg = REG_CEA_GEN_CFG;
		dpu_pr_info("DRM type");
		break;
	default:
		dpu_pr_err("do not support infoframe type");
	}
}

static void core_hw_write_info_frame(void *hdmi_reg_base, const u8 *buffer,
	u8 len, const struct infoframe_offset *offset)
{
	const u8 *ptr = buffer;
	u32 i;
	u32 data;

	/* Write PKT header */
	data = reg_sub_pkt_hb0(*ptr++);
	data |= reg_sub_pkt_hb1(*ptr++);
	data |= reg_sub_pkt_hb2(*ptr++);
	hdmi_writel(hdmi_reg_base, offset->head, data);

	/* Write PKT body */
	for (i = 0; i < MAX_SUB_PKT_NUM; i++) {
		data = reg_sub_pktx_pb0(*ptr++);
		data |= reg_sub_pktx_pb1(*ptr++);
		data |= reg_sub_pktx_pb2(*ptr++);
		data |= reg_sub_pktx_pb3(*ptr++);
		/* the address offset of two subpackages is 8bytes */
		hdmi_writel(hdmi_reg_base, offset->pkt_l + i * 8, data);
		data = reg_sub_pktx_pb4(*ptr++);
		data |= reg_sub_pktx_pb5(*ptr++);
		data |= reg_sub_pktx_pb6(*ptr++);
		/* the address offset of two subpackages is 8bytes */
		hdmi_writel(hdmi_reg_base, offset->pkt_h + i * 8, data);
	}
}

void core_hw_send_info_frame(void *hdmi_reg_base, const u8 *buffer, u8 len)
{
	const u8 *ptr = buffer;
	struct infoframe_offset offset = {0};

	if (buffer == NULL || hdmi_reg_base == NULL) {
		dpu_pr_err("ptr null err");
		return;
	}
	if (len < INFOFRAME_PKT_SIZE) {
		dpu_pr_err("buffer size less than send buffer size");
		return;
	}

	core_get_infoframe(ptr, &offset);
	core_hw_write_info_frame(hdmi_reg_base, buffer, len, &offset);

	/* Enable PKT send */
	hdmi_clrset(hdmi_reg_base, offset.cfg,
		REG_EN_M | REG_RPT_EN_M, reg_en(1) | reg_rpt_en(1));
}

static void core_set_hw_deepcolor(void *hdmi_reg_base, u8 mode, bool enable)
{
	if (hdmi_reg_base == NULL) {
		dpu_pr_err("ptr null err");
		return;
	}

	/* set tmds pack mode */
	hdmi_clrset(hdmi_reg_base, REG_TX_PACK_FIFO_CTRL,
				REG_TMDS_PACK_MODE_M, reg_pack_mode(mode));

	/* Enable/disable PKT send, and enable/disable avmute in phase */
	hdmi_clrset(hdmi_reg_base, REG_AVMIXER_CONFIG,
				REG_DC_PKT_EN_M | REG_AVMUTE_IN_PHASE_M,
				reg_dc_pkt_en((u8)enable) | reg_avmute_in_phase((u8)enable));
}

void core_disable_deepcolor_for_dsc(void *hdmi_reg_base)
{
	const u8 pack_mode = 0;

	if (hdmi_reg_base == NULL) {
		dpu_pr_err("hdmi_reg_base is null");
		return;
	}

	core_set_hw_deepcolor(hdmi_reg_base, pack_mode, false);
}

void core_set_color_depth(void *hdmi_reg_base, int in_color_depth)
{
	bool deep_color_en = false;
	u8 pack_mode = 0;

	if (hdmi_reg_base == NULL) {
		dpu_pr_err("reg_base null ptr");
		return;
	}

	switch (in_color_depth) {
	case HDMITX_BPC_24:
		break;
	case HDMITX_BPC_30:
		pack_mode = 1;
		deep_color_en = true;
		break;
	case HDMITX_BPC_36:
		pack_mode = 2; /* pack mode need set to 2. */
		deep_color_en = true;
		break;
	default:
		dpu_pr_err("Not support this output color depth: %d", in_color_depth);
	}

	if (g_hdmitx_gcp_enable_debug)
		deep_color_en = true;

	/* set tmds pack mode */
	hdmi_clrset(hdmi_reg_base, REG_TX_PACK_FIFO_CTRL,
		REG_TMDS_PACK_MODE_M, reg_pack_mode(pack_mode));

	/* Enable/disable PKT send, and enable/disable avmute in phase */
	hdmi_clrset(hdmi_reg_base, REG_AVMIXER_CONFIG,
		REG_DC_PKT_EN_M | REG_AVMUTE_IN_PHASE_M,
		reg_dc_pkt_en((u8)deep_color_en) | reg_avmute_in_phase((u8)deep_color_en));
}

void core_set_hdmi_mode(void *hdmi_reg_base, bool is_hdmi_mode)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "reg base is NULL");

	hdmi_clrset(hdmi_reg_base, REG_AVMIXER_CONFIG, REG_HDMI_MODE_M, reg_hdmi_mode((u8)is_hdmi_mode));
}

void core_set_vdp_packet(void *hdmi_reg_base, bool enabe)
{
	/* Reset vdp packet path */
	hdmi_clr(hdmi_reg_base, REG_TX_METADATA_CTRL_ARST_REQ,
		REG_TX_METADATA_ARST_REQ_M);

	/* enable vdp packet path */
	hdmi_clrset(hdmi_reg_base, REG_TX_METADATA_CTRL, REG_TXMETA_VDP_PATH_EN_M,
		reg_txmeta_vdp_path_en((u8)enabe));
}

void core_send_av_mute(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	/* set av mute */
	hdmi_clrset(hdmi_reg_base, REG_CP_PKT_AVMUTE,
		REG_CP_CLR_AVMUTE_M | REG_CP_SET_AVMUTE_M,
		reg_cp_clr_avmute(0) | reg_cp_set_avmute(1));

	/* Enable PKT send. set mute send 17 times, clr mute always send */
	hdmi_clrset(hdmi_reg_base, REG_CEA_CP_CFG,
		REG_EN_M | REG_RPT_EN_M | REG_RPT_CNT_M,
		reg_en(1) | reg_rpt_en(0) | reg_rpt_cnt(17));
}

void core_send_av_unmute(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	/* set av unmute */
	hdmi_clrset(hdmi_reg_base, REG_CP_PKT_AVMUTE,
		REG_CP_CLR_AVMUTE_M | REG_CP_SET_AVMUTE_M,
		reg_cp_clr_avmute(1) | reg_cp_set_avmute(0));

	/* Enable PKT send */
	hdmi_clrset(hdmi_reg_base, REG_CEA_CP_CFG,
		REG_EN_M | REG_RPT_EN_M,
		reg_en(1) | reg_rpt_en(1));
}

void core_enable_hdcp_clk(void *hsdt1_sub_harden_base)
{
	dpu_check_and_no_retval(!hsdt1_sub_harden_base, err, "null ptr\n");

	hdmi_write_bits(hsdt1_sub_harden_base, 0x0, GT_CLK_HDMI_HDCP_MASK, 1);
}

void core_disable_hdcp_clk(void *hsdt1_sub_harden_base)
{
	dpu_check_and_no_retval(!hsdt1_sub_harden_base, err, "null ptr\n");

	hdmi_write_bits(hsdt1_sub_harden_base, 0x0, GT_CLK_HDMI_HDCP_MASK, 0);
}

void core_enable_memory(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	hdmi_writel(hdmi_reg_base, REG_TP_MEMCTRL, 0x850);
	hdmi_writel(hdmi_reg_base, REG_SP_MEMCTRL, 0x15858);
}

void core_disable_memory(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	hdmi_writel(hdmi_reg_base, REG_TP_MEMCTRL, 0x54);
	hdmi_writel(hdmi_reg_base, REG_SP_MEMCTRL, 0x585c);
}

void core_reset_pwd(void *hdmi_reg_base, bool enable)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	hdmi_clrset(hdmi_reg_base, REG_TX_PWD_RST_CTRL, REG_TX_PWD_SRST_REQ_M, reg_tx_pwd_srst_req((u8)enable));
}

static bool core_hw_tmds_clk_is_stable(void *hdmi_reg_base)
{
	u32 data;

	data = hdmi_readl(hdmi_reg_base, REG_TX_PACK_FIFO_ST);

	return (data & REG_PCLK2TCLK_STABLE) ? true : false;
}

void core_hdmitx_controller_soft_reset(const struct hdmitx_ctrl *hdmitx)
{
	u32 i = 0;

	if (hdmitx == NULL) {
		dpu_pr_err("hdmitx is null");
		return;
	}
	core_reset_pwd(hdmitx->base, true);
	udelay(10);
	core_reset_pwd(hdmitx->base, false);

	do {
		if (core_hw_tmds_clk_is_stable(hdmitx->base))
			break;
		msleep(1); /* check whether the clock is stable per 1ms */
		i++;
	} while (i < 20); /* need wait 20 times. */

	dpu_pr_debug("wait tmds clk stable %u ms(max 20)", i);
}

void core_reset_req_all(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	hdmi_writel(hdmi_reg_base, HDMITX_METADATA_CTRL0_REG, 0x20000120);
	hdmi_writel(hdmi_reg_base, RESET_REQ_ALL, 0xFF);
	hdmi_writel(hdmi_reg_base, RESET_REQ_ALL, 0x0);
}

void core_set_audio_acr(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	hdmi_clrset(hdmi_reg_base, REG_AUD_ACR_CTRL, REG_ACR_CTS_INIT_MASK, 0x4);
}

void core_set_video_rgb_solid_color(void *hdmi_reg_base, u32 solid_color_type, bool enable)
{
	u32 solid_patten_r_cr = 0;
	u32 solid_patten_g_y = 0;
	u32 solid_patten_b_cb = 0;

	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");
	switch(solid_color_type) {
	case SOLID_BLACK:
		break;
	case SOLID_RED:
		solid_patten_r_cr = 0x3ff;
		break;
	case SOLID_GREEN:
		solid_patten_g_y = 0x3ff;
		break;
	case SOLID_BLUE:
		solid_patten_b_cb = 0x3ff;
		break;
	case SOLID_WHITE:
		solid_patten_r_cr = 0x3ff;
		solid_patten_g_y = 0x3ff;
		solid_patten_b_cb = 0x3ff;
		break;
	default:
		dpu_pr_err("Not support color num %u", solid_color_type);
		return;
	}

	hdmi_clrset(hdmi_reg_base, REG_SOLID_PATTERN_CONFIG,
		REG_SOLID_PATTERN_CR_M | REG_SOLID_PATTERN_Y_M | REG_SOLID_PATTERN_CB_M,
		reg_solid_pattern_cr(solid_patten_r_cr) |
		reg_solid_pattern_y(solid_patten_g_y) |
		reg_solid_pattern_cb(solid_patten_b_cb));
}

void core_set_video_color_bar(void *hdmi_reg_base, bool enable)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");
	hdmi_clrset(hdmi_reg_base, REG_PATTERN_GEN_CTRL,
		REG_TPG_ENABLE_M | REG_SOLID_PATTERN_EN_M,
		reg_tpg_enable((u8)enable) | reg_colorbar_en((u8)enable));
}

void core_hw_enable_tmds_scramble(void *hdmi_reg_base, bool enable)
{
	if (hdmi_reg_base == NULL) {
		dpu_pr_err("ptr null err");
		return;
	}

	hdmi_clrset(hdmi_reg_base, REG_HDMI_ENC_CTRL,
				REG_ENC_HDMI2_ON_M | REG_ENC_SCR_ON_M,
				reg_enc_hdmi2_on((u8)enable) | reg_enc_scr_on((u8)enable));
}