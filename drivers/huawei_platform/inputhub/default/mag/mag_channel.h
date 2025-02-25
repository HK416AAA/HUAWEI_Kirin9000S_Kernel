/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: mag channel header file
 * Author: linjianpeng <linjianpeng1@huawei.com>
 * Create: 2020-05-25
 */

#ifndef __MAG_CHANNEL_H__
#define __MAG_CHANNEL_H__

int send_mag_calibrate_data_to_mcu(void);
int send_mag1_calibrate_data_to_mcu(void);
int write_mag1sensor_calibrate_data_to_nv(const char *src);
int write_magsensor_calibrate_data_to_nv(const char *src, int length);
void send_mag_charger_to_mcu(void);
int send_current_to_mcu_mag(int current_value_now);
void mag_charge_notify_close(void);
void mag_charge_notify_open(void);
void reset_mag_calibrate_data(void);

#endif
#ifdef CHA_MAG_DETACH
#define CHARGER_TYPE_MAG_SDP    0
#define CHARGER_TYPE_MAG_CDP    1
#define CHARGER_TYPE_MAG_DCP    2
#define CHARGER_TYPE_MAG_UNKNOWN    3
#define CHARGER_TYPE_MAG_NONE   4
#define PLEASE_PROVIDE_MAG_POWER    5
#define CHARGER_TYPE_MAG_ILLEGAL    6
#endif