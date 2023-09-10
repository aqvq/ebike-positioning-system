#ifndef _IOT_INTERFACE_H_
#define _IOT_INTERFACE_H_

#include <stdint.h>
#include "msg/general_message.h"

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
