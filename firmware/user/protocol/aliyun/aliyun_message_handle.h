
#ifndef _ALIYUN_MESSAGE_HANDLE_H_
#define _ALIYUN_MESSAGE_HANDLE_H_

#include "msg/general_message.h"

/// @brief 处理收到的通用数据
/// @param handle mqtt会话句柄
/// @param adv_packet
void handle_aliyun_message(void *handle, general_message_t *general_message);

#endif // _ALIYUN_MESSAGE_HANDLE_H_
