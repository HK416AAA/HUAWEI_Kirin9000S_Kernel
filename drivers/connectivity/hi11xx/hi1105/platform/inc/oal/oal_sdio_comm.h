/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: Header file for storing common host/device information.
 *              The host/device interface mechanisms differ greatly.
 *              The interfaces do not need to be normalized. However,
 *              common information such as structures and macro enumerations
 *              needs to be stored in this file
 * Author: @CompanyNameTag
 */

#ifndef OAL_SDIO_COMM_H
#define OAL_SDIO_COMM_H

#undef CONFIG_SDIO_DEBUG
#define CONFIG_SDIO_FUNC_EXTEND

#define CONFIG_SDIO_MSG_FLOWCTRL
#undef CONFIG_CREDIT_MSG_FLOW_CTRL_DEBUG

#undef CONFIG_SDIO_REINIT_SUPPORT

/* SDIO传输时，将所有的SG LIST 合并成一块完整的内存发送! */
#define CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
#define CONFIG_HISDIO_D2H_SCATT_LIST_ASSEMBLE

/*
 * 定义此宏表示打开arm to host 的消息回读ACK，
 * 消息是从ARM发送至host,
 * 必须读高字节才能产生ACK
 */
#define CONFIG_SDIO_D2H_MSG_ACK

/*
 * 定义此宏表示打开host to arm 的消息回读ACK，
 * 消息是从Host发送至ARM,
 * 必须读高字节才能产生ACK
 */
#undef CONFIG_SDIO_H2D_MSG_ACK

typedef enum _hisdio_d2h_msg_type_ {
    D2H_MSG_WLAN_READY = 0,
    D2H_MSG_FLOWCTRL_UPDATE = 1, /* For the credit flow ctrl */
    D2H_MSG_FLOWCTRL_OFF = 2,    /* can't send data */
    D2H_MSG_FLOWCTRL_ON = 3,     /* can send data */
    D2H_MSG_WAKEUP_SUCC = 4,     /* Wakeup done */
    D2H_MSG_ALLOW_SLEEP = 5,     /* ALLOW Sleep */
    D2H_MSG_DISALLOW_SLEEP = 6,  /* DISALLOW Sleep */
    D2H_MSG_DEVICE_PANIC = 7,    /* arm abort */
    D2H_MSG_POWEROFF_ACK = 8,    /* Poweroff cmd ack */
    D2H_MSG_OPEN_BCPU_ACK = 9,   /* OPEN BCPU cmd ack */
    D2H_MSG_CLOSE_BCPU_ACK = 10, /* CLOSE BCPU cmd ack */
    D2H_MSG_CREDIT_UPDATE = 11,  /* update high priority buffer credit value */
    D2H_MSG_HIGH_PKT_LOSS = 12,  /* high pri pkts loss count */
    D2H_MSG_HALT_BCPU = 13,      /* halt bcpu ack */

    D2H_MSG_SHUTDOWN_IP_PRE_RESPONSE = 14, /* prepare shutdown done */
    D2H_MSG_RESERV_15 = 15,

    D2H_MSG_COUNT = 16 /* max support msg count */
} hisdio_d2h_msg_type;

/* Host to device sdio message type */
typedef enum _hisdio_h2d_msg_type_ {
    H2D_MSG_FLOWCTRL_ON = 0, /* can send data, force to open */
    H2D_MSG_DEVICE_INFO_DUMP = 1,
    H2D_MSG_DEVICE_MEM_DUMP = 2,
    H2D_MSG_TEST = 3,
    H2D_MSG_PM_WLAN_OFF = 4,
    H2D_MSG_SLEEP_REQ = 5,
    H2D_MSG_PM_DEBUG = 6,

    H2D_MSG_RESET_BCPU = 7,
    H2D_MSG_QUERY_RF_TEMP = 8,

    H2D_MSG_DEVICE_MEM_INFO = 10,
    H2D_MSG_STOP_SDIO_TEST = 11,
    H2D_MSG_PM_BCPU_OFF = 12,
    H2D_MSG_HALT_BCPU = 13,

    H2D_MSG_SHUTDOWN_IP_PRE = 14, /* prepare to shutdown ip */
    H2D_MSG_RESERV_15 = 15,

    H2D_MSG_COUNT = 16 /* max support msg count */
} hisdio_h2d_msg_type;

/* sdio flow control info, free cnt */
#define hisdio_short_pkt_set(reg, num)               \
    do {                                             \
        (reg) = (((reg)&0xFFFFFF00) | (((num)&0xFF))); \
    } while (0)
#define hisdio_large_pkt_set(reg, num)                    \
    do {                                                  \
        (reg) = (((reg)&0xFFFF00FF) | (((num)&0xFF) << 8)); \
    } while (0)
#define hisdio_reserve_pkt_set(reg, num)                   \
    do {                                                   \
        (reg) = (((reg)&0xFF00FFFF) | (((num)&0xFF) << 16)); \
    } while (0)
#define hisdio_comm_reg_seq_set(reg, num)                  \
    do {                                                   \
        (reg) = (((reg)&0x00FFFFFF) | (((num)&0xFF) << 24)); \
    } while (0)

#define hisdio_short_pkt_get(reg)    ((reg)&0xFF)
#define hisdio_large_pkt_get(reg)    (((reg) >> 8) & 0xFF)
#define hisdio_reserve_pkt_get(reg)  (((reg) >> 16) & 0xFF)
#define hisdio_comm_reg_seq_get(reg) (((reg) >> 24) & 0xFF)

#define HISDIO_BLOCK_SIZE 512 /* one size of data transfer block size, 64, 128, 256, 512, 1024 */
#define HISDIO_SEND_SIZE 4096
/* The max scatter buffers when host to device */
#define HISDIO_HOST2DEV_SCATT_MAX  64
#define HISDIO_HOST2DEV_SCATT_SIZE 64

/* The max scatter buffers when device to host */
#define HISDIO_DEV2HOST_SCATT_MAX 64

/* The max scatt num of rx and tx */
#define HISDIO_SCATT_MAX_NUM (HISDIO_DEV2HOST_SCATT_MAX)

/* 64B used to store the scatt info,1B means 1 pkt. */
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS 3
/* 1 << 5 */
/* Host to device's descr align length depends on the CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE */
#ifdef CONFIG_HISDIO_H2D_SCATT_LIST_ASSEMBLE
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN 8
#else
#define HISDIO_H2D_SCATT_BUFFLEN_ALIGN 32
#endif

/* Device To Host,descr just request 4 bytes aligned, but 10 bits round [0~1023], so we also aligned to 32 bytes */
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN_BITS 5
/* 1 << 5 */
#ifdef CONFIG_HISDIO_D2H_SCATT_LIST_ASSEMBLE
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN 32
#else
#define HISDIO_D2H_SCATT_BUFFLEN_ALIGN 512
#endif

#define HSDIO_HOST2DEV_PKTS_MAX_LEN 1560

#endif
