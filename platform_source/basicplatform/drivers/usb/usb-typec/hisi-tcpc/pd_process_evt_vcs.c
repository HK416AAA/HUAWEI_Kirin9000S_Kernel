/*
 * pd_process_evt_vcs.c
 *
 * Power Delivery Process Event For VCS
 *
 * Copyright (c) 2019 Huawei Technologies Co., Ltd.
 *
 * Copyright (C) 2016 Richtek Technology Corp.
 * Author: TH <tsunghan_tsai@richtek.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */


#define LOG_TAG "[EVT_VCS]"

#include "include/pd_core.h"
#include "include/pd_tcpm.h"
#include "include/tcpci_event.h"
#include "include/pd_process_evt.h"

/* PD Control MSG reactions */

DECL_PE_STATE_TRANSITION(PD_CTRL_MSG_REJECT_WAIT) = {
	{ PE_VCS_SEND_SWAP, PE_VIRT_READY },
};
DECL_PE_STATE_REACTION(PD_CTRL_MSG_REJECT_WAIT);

DECL_PE_STATE_TRANSITION(PD_CTRL_MSG_PS_RDY) = {
	{ PE_VCS_WAIT_FOR_VCONN, PE_VCS_TURN_OFF_VCONN },
};
DECL_PE_STATE_REACTION(PD_CTRL_MSG_PS_RDY);

/* DPM Event reactions */

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_ACK) = {
	{ PE_VCS_EVALUATE_SWAP, PE_VCS_ACCEPT_SWAP },
	{ PE_VCS_TURN_ON_VCONN, PE_VCS_SEND_PS_RDY },
	{ PE_VCS_TURN_OFF_VCONN, PE_VIRT_READY },
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_ACK);

DECL_PE_STATE_TRANSITION(PD_DPM_MSG_NAK) = {
	{ PE_VCS_EVALUATE_SWAP, PE_VCS_REJECT_VCONN_SWAP },
};
DECL_PE_STATE_REACTION(PD_DPM_MSG_NAK);

/* Timer Event reactions */

DECL_PE_STATE_TRANSITION(PD_TIMER_SENDER_RESPONSE) = {
	{ PE_VCS_SEND_SWAP, PE_VIRT_READY },
};
DECL_PE_STATE_REACTION(PD_TIMER_SENDER_RESPONSE);


DECL_PE_STATE_TRANSITION(PD_TIMER_VCONN_ON) = {
	{ PE_VCS_WAIT_FOR_VCONN, PE_VIRT_HARD_RESET},
};
DECL_PE_STATE_REACTION(PD_TIMER_VCONN_ON);

/*
 * [BLOCK] Porcess PD Ctrl MSG
 */

static bool pd_process_ctrl_msg_good_crc(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	D("+\n");
	switch (pd_port->pe_state_curr) {
	case PE_VCS_REJECT_VCONN_SWAP:
	case PE_VCS_SEND_PS_RDY:
		PE_TRANSIT_READY_STATE(pd_port);
		return true;

	case PE_VCS_ACCEPT_SWAP:
		PE_TRANSIT_VCS_SWAP_STATE(pd_port);
		return true;

	case PE_VCS_SEND_SWAP:
		pd_enable_timer(pd_port, PD_TIMER_SENDER_RESPONSE);
		return false;

	default:
		return false;
	}
}

static bool pd_process_ctrl_msg_accept(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	D("+\n");
	if (pd_port->pe_state_curr == PE_VCS_SEND_SWAP) {
		PE_TRANSIT_VCS_SWAP_STATE(pd_port);
		return true;
	}

	return false;
}

static bool pd_process_ctrl_msg(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool ret = false;

	D("+\n");
	switch (pd_event->msg) {
	case PD_CTRL_GOOD_CRC:
		return pd_process_ctrl_msg_good_crc(pd_port, pd_event);

	case PD_CTRL_ACCEPT:
		return pd_process_ctrl_msg_accept(pd_port, pd_event);

	case PD_CTRL_WAIT:
	case PD_CTRL_REJECT:
#ifdef CONFIG_PD_DFP_RESET_CABLE
		if (pd_port->detect_emark) {
			if (hisi_usb_typec_force_vconn())
				return pd_process_ctrl_msg_accept(pd_port, pd_event);
			else
				hisi_pd_cable_flag_clear(pd_port);
			pd_port->vswap_ret = PD_CTRL_REJECT;
		}
#endif
		ret = PE_MAKE_STATE_TRANSIT_VIRT(PD_CTRL_MSG_REJECT_WAIT);
		break;

	case PD_CTRL_PS_RDY:
		ret = PE_MAKE_STATE_TRANSIT(PD_CTRL_MSG_PS_RDY);
		break;

	default:
		break;
	}

	return ret;
}

/*
 * [BLOCK] Porcess DPM MSG
 */

static bool pd_process_dpm_msg(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool ret = false;

	D("+\n");
	switch (pd_event->msg) {
	case PD_DPM_ACK:
		ret = PE_MAKE_STATE_TRANSIT_VIRT(PD_DPM_MSG_ACK);
		break;

	case PD_DPM_NAK:
		ret = PE_MAKE_STATE_TRANSIT(PD_DPM_MSG_NAK);
		break;

	default:
		break;
	}

	D("-\n");
	return ret;
}

/*
 * [BLOCK] Porcess Timer MSG
 */

static bool pd_process_timer_msg(
		pd_port_t *pd_port, pd_event_t *pd_event)
{
	bool ret = false;

	D("+\n");
	switch (pd_event->msg) {
	case PD_TIMER_SENDER_RESPONSE:
		ret = PE_MAKE_STATE_TRANSIT_VIRT(PD_TIMER_SENDER_RESPONSE);
		break;

	case PD_TIMER_VCONN_ON:
		ret = PE_MAKE_STATE_TRANSIT_VIRT(PD_TIMER_VCONN_ON);
		break;

	default:
		break;
	}

	D("-\n");
	return ret;
}

/*
 * [BLOCK] Process Policy Engine's VCS Message
 */

bool hisi_pd_process_event_vcs(pd_port_t *pd_port, pd_event_t *pd_event)
{
	D("\n");
	switch (pd_event->event_type) {
	case PD_EVT_CTRL_MSG:
		D("PD_EVT_CTRL_MSG\n");
		return pd_process_ctrl_msg(pd_port, pd_event);

	case PD_EVT_DPM_MSG:
		D("PD_EVT_DPM_MSG\n");
		return pd_process_dpm_msg(pd_port, pd_event);

	case PD_EVT_TIMER_MSG:
		D("PD_EVT_TIMER_MSG\n");
		return pd_process_timer_msg(pd_port, pd_event);

	default:
		return false;
	}
}
