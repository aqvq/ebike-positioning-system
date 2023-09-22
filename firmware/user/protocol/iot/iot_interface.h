/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:57:39
 * @FilePath: \firmware\user\protocol\iot\iot_interface.h
 * @Description: iot协议实现接口
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef _IOT_INTERFACE_H_
#define _IOT_INTERFACE_H_

#include <stdint.h>
#include "data/general_message.h"

/**
 * @brief 日志级别枚举类型定义
 */
typedef enum {
    IOT_LOG_LEVEL_FATAL,
    IOT_LOG_LEVEL_ERROR,
    IOT_LOG_LEVEL_WARN,
    IOT_LOG_LEVEL_INFO,
    IOT_LOG_LEVEL_DEBUG,
} iot_log_level_t;

/// @brief 从IoT平台接收数据的回调函数指针
typedef int32_t (*iot_receive_callback)(void *data);

/// @brief IoT平台标准接口
typedef struct
{
    /// @brief 连接IoT平台
    int32_t (*iot_connect)(iot_receive_callback func);

    /// @brief 断开连接IoT平台
    int32_t (*iot_disconnect)(void);

    /// @brief 发送数据至IoT平台
    int32_t (*iot_send)(general_message_t *msg);

    /// @brief 向Iot平台发送日志
    int32_t (*iot_post_log)(iot_log_level_t level, char *module_name, int32_t code, char *content);
} iot_interface_t;

#endif
