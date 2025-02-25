/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * foss@huawei.com
 *
 * If distributed as part of the Linux kernel, the following license terms
 * apply:
 *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License version 2 and
 * * only version 2 as published by the Free Software Foundation.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
 *
 * Otherwise, the following license terms apply:
 *
 * * Redistribution and use in source and binary forms, with or without
 * * modification, are permitted provided that the following conditions
 * * are met:
 * * 1) Redistributions of source code must retain the above copyright
 * *    notice, this list of conditions and the following disclaimer.
 * * 2) Redistributions in binary form must reproduce the above copyright
 * *    notice, this list of conditions and the following disclaimer in the
 * *    documentation and/or other materials provided with the distribution.
 * * 3) Neither the name of Huawei nor the names of its contributors may
 * *    be used to endorse or promote products derived from this software
 * *    without specific prior written permission.
 *
 * * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RNIC_IOCTL_DEF_H
#define RNIC_IOCTL_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

#define RNIC_IOCTL_START		0x89F0 /* same as SIOCDEVPRIVATE */
#define RNIC_IOCTL_END			0x89FF
#define RNIC_IOCTL_PRIVATE		RNIC_IOCTL_END

#define RNIC_IOCTL_PRIV_ADD_VLAN	0x0001
#define RNIC_IOCTL_PRIV_DEL_VLAN	0x0002
#define RNIC_IOCTL_PRIV_GET_VLAN	0x0003
#define RNIC_IOCTL_PRIV_SET_DROP_TIME	0x0004
#define RNIC_IOCTL_PRIV_GET_DROP_TIME	0x0005
#define RNIC_IOCTL_PRIV_SET_GRO_WT	0x0006
#define RNIC_IOCTL_PRIV_DIS_GRO	0x0007
#define RNIC_IOCTL_PRIV_BOOST_CPU	0x0008

#define RNIC_IOCTL_MAX_VLAN_NUM		16

struct rnic_ioctl_priv_args_s {
	int cmd;
	union {
		unsigned int data;

		struct {
			unsigned int   vlan_num; /* number of vlan id */
			unsigned short vlan_ids[RNIC_IOCTL_MAX_VLAN_NUM];
		} vlan_cfg;

		struct {
			unsigned char code;
		} drop_time;

		unsigned int gro_weight;
		unsigned int gro_disable;
		unsigned int boost_en;
	};
};

#ifdef __cplusplus
}
#endif

#endif /* RNIC_IOCTL_DEF_H */
