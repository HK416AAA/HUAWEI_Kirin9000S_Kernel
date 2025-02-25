/*
 * zodiac mmc sdhci controller interface.
 * Copyright (c) Zodiac Technologies Co., Ltd. 2017-2019. All rights reserved.
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
 */

#ifndef __SDHCI_ZODIAC_ANTLIA_H
#define __SDHCI_ZODIAC_ANTLIA_H

/* hsdt1 sd reset */
#define PERRSTEN0	0x60
#define PERRSTDIS0	0x64
#define IP_RST_SD	(0x1 << 1)
#define IP_HRST_SD	(0x1 << 0)

#define MAX_TUNING_LOOP 40

void sdhci_zodiac_hardware_reset(struct sdhci_host *host);
void sdhci_zodiac_hardware_disreset(struct sdhci_host *host);
int sdhci_zodiac_get_resource(struct platform_device *pdev, struct sdhci_host *host);
#endif
