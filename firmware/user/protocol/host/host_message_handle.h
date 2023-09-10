
#ifndef _HOST_MESSAGE_HANDLE_H_
#define _HOST_MESSAGE_HANDLE_H_

#include "data/general_message.h"

/// @brief 处理收到的通用数据
/// @param general_message
int8_t handle_host_message(general_message_t *general_message);

#endif //_HOST_MESSAGE_HANDLE_H_
