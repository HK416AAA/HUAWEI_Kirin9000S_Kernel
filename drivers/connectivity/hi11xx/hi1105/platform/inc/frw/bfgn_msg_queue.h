/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: Declaration Message Queuing Interface
 * Author: @CompanyNameTag
 */

#ifndef BFGN_MSG_QUEUE_H
#define BFGN_MSG_QUEUE_H

/* 其他头文件包含 */
#include "types.h"

/* 宏定义 */
#define BFG_MSG_BUTT 0xFFFF

/* 枚举定义 */
/* 判断消息队列是否为空结果 */
typedef enum {
    QUEUE_EMPTY = 0x00,
    QUEUE_NOT_EMPTY = 0x01,
    QUEUE_BUTT
} queue_is_empty_enum;
typedef UINT8 queue_is_empty_enum_uint8;
/* 设置事件结果 */
typedef enum {
    SYS_MSG_NO_SPACE = 0x00, /* 没空间存储事件 */
    SYS_MSG_SET_SUCC = 0x01, /* 设置事件成功 */
    SYS_MSG_BUTT
} msg_set_result_enum;
typedef UINT8 msg_set_result_enum_uint8;

typedef enum {
    MSG_NOT_AVAILABLE = 0x00,
    MSG_AVAILABLE = 0x01,
    MSG_CHECK_BUTT
} msg_check_available_enum;
typedef UINT8 msg_check_available_enum_uint8;

/* STRUCT定义 */
/* 队列节点结构体定义 */
typedef struct node {
    UINT16 us_msg;
    struct node *pst_nextnode;
} msg_node_stru;
/* 队列结构体定义 */
typedef struct {
    msg_node_stru *pst_header;
    msg_node_stru *pst_tail;
} queue_stru;

/* 待处理事件队列、空事件队列 */
typedef struct {
    queue_stru st_msg_handle_que;
    queue_stru st_msg_pool_que;
} queue_group_stru;

/* OTHERS定义 */
typedef void (*MSG_FUNC)(void);

/* 函数声明 */
void bfgn_msg_queue_init(queue_stru *pst_msg_pool_que,
                         queue_stru *pst_msg_handle_que,
                         msg_node_stru ast_msg_res[],
                         UINT16 us_msg_res_len);
UINT8 bfgn_post_msg_to_queue(queue_group_stru *pst_msg_queue, UINT16 usMSGNum);
UINT8 bfgn_insert_msg_to_queue(queue_group_stru *pst_msg_queue, UINT16 usMSGNum);
UINT16 bfgn_get_msg(queue_group_stru *pst_msg_queue);
INT8 bfgn_queue_isempty(queue_stru *pst_msg_queue);

#endif /* end of message_queue_handle.h */
