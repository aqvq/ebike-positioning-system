
#ifndef _ALIYUN_PROTOCOL_H_
#define _ALIYUN_PROTOCOL_H_

#include "protocol/iot/iot_interface.h"

/// @brief 连接阿里云物联网平台
/// @param func
/// @return
int32_t aliyun_iot_connect(iot_receive_callback func);

/// @brief 断开连接阿里云物联网平台
/// @param
/// @return
int32_t aliyun_iot_disconnect(void);

/// @brief 发送数据至阿里云物联网平台
/// @param msg
/// @return
int32_t aliyun_iot_send(general_message_t *msg);

/// @brief 向Iot平台发送日志
/// @param level 日志级别
/// @param module_name 模块名称
/// @param code 状态码
/// @param content 日志内容
/// @return 0正常 其他异常
int32_t aliyun_iot_post_log(iot_log_level_t level, char *module_name, int32_t code, char *content);

#endif //