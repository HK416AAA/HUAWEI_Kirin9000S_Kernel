/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2022. All rights reserved.
 * Description: dynamic ion uuid declaration.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef DYNAMIC_ION_UUID_H
#define DYNAMIC_ION_UUID_H

#ifdef DEF_ENG
#define TEE_SERVICE_UT \
{ \
	0x03030303, \
	0x0303, \
	0x0303, \
	{ \
		0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03 \
	} \
}

#define TEE_SERVICE_TEST_DYNION \
{ \
	0x7f313b2a, \
	0x68b9, \
	0x4e92, \
	{ \
		0xac, 0xf9, 0x13, 0x3e, 0xbb, 0x54, 0xeb, 0x56 \
	} \
}
#endif

#define TEE_SECIDENTIFICATION1 \
{ \
	0x8780dda1, \
	0xa49e, \
	0x45f4, \
	{ \
		0x96, 0x97, 0xc7, 0xed, 0x9e, 0x38, 0x5e, 0x83 \
	} \
}

#define TEE_SECIDENTIFICATION3 \
{ \
	0x335129cd, \
	0x41fa, \
	0x4b53, \
	{ \
		0x97, 0x97, 0x5c, 0xcb, 0x20, 0x2a, 0x52, 0xd4 \
	} \
}

#define TEE_SERVICE_AI \
{ \
	0xf4a8816d, \
	0xb6fb, \
	0x4d4f, \
	{ \
		0xa2, 0xb9, 0x7d, 0xae, 0x57, 0x33, 0x13, 0xc0 \
	} \
}

#define TEE_SERVICE_AI_TINY \
{ \
	0xc123c643, \
	0x5b5b, \
	0x4c9f, \
	{ \
		0x90, 0x98, 0xbb, 0x09, 0x56, 0x4d, 0x6e, 0xda \
	} \
}

#define TEE_SERVICE_VCODEC \
{ \
	0x528822b7, \
	0xfc78, \
	0x466b, \
	{ \
		0xb5, 0x7e, 0x62, 0x09, 0x3d, 0x60, 0x34, 0xa7 \
	} \
}
#endif
