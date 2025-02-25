/*
 * dubai_peri.c
 *
 * dubai peri module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <dubai_peri_kernel.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <securec.h>
#include <loadmonitor_plat.h>
#include <loadmonitor_common_kernel.h>

#define channel(a)		a
#define volt_level(a)		a
#define ONE_SECOND		1

static struct channel_data g_peri_volt[CHANNEL_SIZE + 1];

u32 get_circle_sec_time(void)
{
	u32 circle_time;

	circle_time = g_monitor_delay_value.peri0 / LOADMONITOR_PERI_FREQ_CS;
	if (circle_time == 0) {
		pr_err("%s, ioctrl need set sample time, flag:0x%x, peri0 freq:%u,%u\n", __func__,
		       g_monitor_delay_value.monitor_enable_flags,
		       g_monitor_delay_value.peri0, LOADMONITOR_PERI_FREQ_CS);
		circle_time = ONE_SECOND;
	}
	return circle_time;
}

void set_channel_peri_data(unsigned int peri_volt, struct channel_data *ch,
			   struct channel_data *sum, unsigned int enable)
{
	if (ch == NULL || enable == 0 || sum == NULL)
		return;

	switch (peri_volt) {
	case volt_level(0):
		ch->volt0 += get_circle_sec_time();
		sum->volt0 += get_circle_sec_time();
		break;
	case volt_level(1):
		ch->volt1 += get_circle_sec_time();
		sum->volt1 += get_circle_sec_time();
		break;
	case volt_level(2):
		ch->volt2 += get_circle_sec_time();
		sum->volt2 += get_circle_sec_time();
		break;
	case volt_level(3):
		ch->volt3 += get_circle_sec_time();
		sum->volt3 += get_circle_sec_time();
		break;
#ifdef PERI_VOTE_8_PROFILE
	case volt_level(4):
		ch->volt4 += get_circle_sec_time();
		sum->volt4 += get_circle_sec_time();
		break;
	case volt_level(5):
		ch->volt5 += get_circle_sec_time();
		sum->volt5 += get_circle_sec_time();
		break;
	case volt_level(6):
		ch->volt6 += get_circle_sec_time();
		sum->volt6 += get_circle_sec_time();
		break;
	case volt_level(7):
		ch->volt7 += get_circle_sec_time();
		sum->volt7 += get_circle_sec_time();
		break;
#endif
	default:
		pr_err("%s,out of peri volt:%d\n", __func__, peri_volt);
	}
}

#ifdef PERI_VOTE_8_PROFILE
void set_dubai_channel0_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_0,
			      &g_peri_volt[channel(0)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(0)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_1,
			      &g_peri_volt[channel(1)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(1)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_2,
			      &g_peri_volt[channel(2)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(2)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_3,
			      &g_peri_volt[channel(3)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(3)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_4,
			      &g_peri_volt[channel(4)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(4)));
}

void set_dubai_channel1_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_5,
			      &g_peri_volt[channel(5)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(5)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_6,
			      &g_peri_volt[channel(6)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(6)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_7,
			      &g_peri_volt[channel(7)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(7)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_8,
			      &g_peri_volt[channel(8)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(8)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_9,
			      &g_peri_volt[channel(9)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(9)));
}

void set_dubai_channel2_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote2.reg.peri_vote_vol_rank_10,
			      &g_peri_volt[channel(10)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(10)));
	set_channel_peri_data(peri->vote2.reg.peri_vote_vol_rank_11,
			      &g_peri_volt[channel(11)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(11)));
	set_channel_peri_data(peri->vote2.reg.peri_vote_vol_rank_12,
			      &g_peri_volt[channel(12)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(12)));
	set_channel_peri_data(peri->vote2.reg.peri_vote_vol_rank_13,
			      &g_peri_volt[channel(13)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(13)));
	set_channel_peri_data(peri->vote2.reg.peri_vote_vol_rank_14,
			      &g_peri_volt[channel(14)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(14)));
}

void set_dubai_channel3_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote3.reg.peri_vote_vol_rank_15,
			      &g_peri_volt[channel(15)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(15)));
}
#else
void set_dubai_channel0_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_0,
			      &g_peri_volt[channel(0)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(0)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_1,
			      &g_peri_volt[channel(1)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(1)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_2,
			      &g_peri_volt[channel(2)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(2)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_3,
			      &g_peri_volt[channel(3)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(3)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_4,
			      &g_peri_volt[channel(4)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(4)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_5,
			      &g_peri_volt[channel(5)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(5)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_6,
			      &g_peri_volt[channel(6)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(6)));
	set_channel_peri_data(peri->vote0.reg.peri_vote_vol_rank_7,
			      &g_peri_volt[channel(7)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(7)));
}

void set_dubai_channel1_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_8,
			      &g_peri_volt[channel(8)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(8)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_9,
			      &g_peri_volt[channel(9)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(9)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_10,
			      &g_peri_volt[channel(10)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(10)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_11,
			      &g_peri_volt[channel(11)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(11)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_12,
			      &g_peri_volt[channel(12)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(12)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_13,
			      &g_peri_volt[channel(13)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(13)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_14,
			      &g_peri_volt[channel(14)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(14)));
	set_channel_peri_data(peri->vote1.reg.peri_vote_vol_rank_15,
			      &g_peri_volt[channel(15)],
			      &g_peri_volt[CHANNEL_SUM],
			      peri->enable.value & BIT(channel(15)));
}
#endif
void format_dubai_peri_data(struct dubai_peri *peri)
{
	if (peri == NULL)
		return;

	set_dubai_channel0_peri_data(peri);
	set_dubai_channel1_peri_data(peri);
#ifdef PERI_VOTE_8_PROFILE
	set_dubai_channel2_peri_data(peri);
	set_dubai_channel3_peri_data(peri);
#endif
}

int clear_dubai_peri_data(void)
{
	errno_t ret;

	ret = memset_s(g_peri_volt, sizeof(g_peri_volt), 0, sizeof(g_peri_volt));
	if (ret != EOK)
		pr_err("%s,memset peri data err:%d\n", __func__, ret);
	return ret;
}

int ioctrl_get_dubai_peri_data(void __user *argp)
{
	int ret = 0;

	if (copy_to_user(argp, g_peri_volt, sizeof(g_peri_volt))) {
		pr_err("peri data get failed\n");
		ret = -EFAULT;
	}
	return ret;
}

int ioctrl_get_dubai_peri_size(void __user *argp)
{
	int ret = 0;
	struct peri_data_size peri_size;

	peri_size.vote_col = (unsigned int)(sizeof(struct channel_data) / sizeof(u32));
	peri_size.vote_row = CHANNEL_SIZE + 1;
	if (copy_to_user(argp, &peri_size, sizeof(peri_size))) {
		pr_err("peri size get failed\n");
		ret = -EFAULT;
	}
	return ret;
}

