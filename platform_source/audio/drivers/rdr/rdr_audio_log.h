/*
 * rdr_audio_log.h
 *
 * rdr audio log
 *
 * Copyright (c) 2023-2023 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __RDR_AUDIO_LOG_H__
#define __RDR_AUDIO_LOG_H__

#ifndef CONFIG_RDA_AUDIO_DRIVERS
int rdr_audio_write_file(const char *name, const char *data, unsigned int size);
#else
static inline int rdr_audio_write_file(const char *name, const char *data, unsigned int size)
{
	return 0;
}
#endif

#endif /* __RDR_AUDIO_LOG_H__ */

