/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: plat_uart.c header file
 * Author: @CompanyNameTag
 */

#ifndef PLAT_UART_H
#define PLAT_UART_H
/* 其他头文件包含 */
#include <linux/serial_core.h>
#include "hw_bfg_ps.h"

/* 宏定义 */
#define OPEN_TTY_RETRY_COUNT 5
#define RELEASE_DELAT_TIMES 50

/* BFGX正常通信使用3M波特率，host唤醒device时使用115200波特率，单红外时使用921600波特率 */
#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#define DEFAULT_BAUD_RATE 2000000
#else
#define DEFAULT_BAUD_RATE 4000000
#endif

#define LOW_FREQ_BAUD_RATE  4000000
#define HIGH_FREQ_BAUD_RATE 6000000

#define UART_BAUD_RATE_DBG 115200
#define UART_BAUD_RATE_1M  1000000
#define UART_BAUD_RATE_2M  2000000
#define UART_BAUD_RATE_3M  3000000
#define UART_BAUD_RATE_4M  4000000
#define UART_BAUD_RATE_5M  5000000
#define UART_BAUD_RATE_6M  6000000
#define UART_BAUD_RATE_7M  7000000
#define UART_BAUD_RATE_8M  8000000
#define UART_BAUD_RATE_10M 10000000

#define WKUP_DEV_BAUD_RATE 115200
#define IR_ONLY_BAUD_RATE  921600

#define FLOW_CTRL_ENABLE  1
#define FLOW_CTRL_DISABLE 0

#define UART_PCLK_FROM_SENSORHUB 0
#define UART_PCLK_NORMAL         1

typedef enum {
    STATE_TTY_TX = 0,
    STATE_TTY_RX = 1,
    STATE_UART_TX = 2,
    STATE_UART_RX = 3,
} uart_state_index;

enum INI_UART_RATE_ID_ENUM {
    INI_UART_RATE_DEFAULT = 0x0,
    INI_UART_RATE_DBG = 0x1,
    INI_UART_RATE_1M = 0x2,
    INI_UART_RATE_2M = 0x3,
    INI_UART_RATE_3M = 0x4,
    INI_UART_RATE_4M = 0x5,
    INI_UART_RATE_5M = 0x6,
    INI_UART_RATE_6M = 0x7,
    INI_UART_RATE_7M = 0x8,
    INI_UART_RATE_8M = 0x9,
    INI_UART_RATE_NUM
};

#if defined(_PRE_HI375X_LDISC) || defined(_PRE_PRODUCT_HI1620S_KUNPENG)
#define N_HW_BFG           29
#define N_HW_GNSS          (N_HW_BFG - 1)
#else
#define N_HW_BFG           (NR_LDISCS - 1)
#define N_HW_GNSS          (N_HW_BFG - 1)
#endif

/* 函数声明 */
int32_t plat_uart_init(int32_t ldisc_num);
void plat_uart_exit(int32_t ldisc_num);
int32_t open_tty_drv(struct ps_core_s *ps_core_d);
int32_t release_tty_drv(struct ps_core_s *ps_core_d);
unsigned long ps_get_default_baud_rate(struct ps_core_s *ps_core_d);
int32_t ps_change_uart_baud_rate(struct ps_core_s *ps_core_d, long baud_rate, uint8_t enable_flowctl);
void ps_tty_tx_cnt_add(struct ps_core_s *ps_core_d, uint32_t cnt);
void ps_uart_state_dump(struct ps_core_s *ps_core_d);
#endif
