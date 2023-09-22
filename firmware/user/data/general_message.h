/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:26:05
 * @FilePath: \firmware\user\data\general_message.h
 * @Description: 通用消息实现头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef _GENERAL_MESSAGE_H_
#define _GENERAL_MESSAGE_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

/// @brief 通用消息类型
typedef enum {

    /* GPS数据-原始数据直接发送给阿里云平台 */
    GNSS_DATA,

    /* GNSS数据 */
    GNSS_NMEA_DATA,
} general_message_type_t;

/// @brief 通用消息
typedef struct
{
    general_message_type_t type; // 消息类型
    uint16_t data_len;           // 消息长度
    uint8_t data[0];             // 可变长消息，根据消息类型解析
} general_message_t;

/// @brief 创建general_message_t对象
/// @param type 消息类型
/// @param data 可变长消息
/// @param data_len 消息长度
void *create_general_message(general_message_type_t type, void *data, uint16_t data_len);

/// @brief 释放general_message_t对象
/// @param msg
int8_t free_general_message(general_message_t *msg);

/// @brief 主动释放队列中general_message_t结构体元素第1个元素；
/// @brief 实现类似标准队列先进先出的功能，让队列中存在的值都是最新的。
/// @param xQueue
/// @return
int8_t remove_queue_first_item(QueueHandle_t xQueue);

#endif
