/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:44:46
 * @FilePath: \firmware\user\protocol\host\host_message_handle.h
 * @Description: 处理host接收到的消息头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _HOST_MESSAGE_HANDLE_H_
#define _HOST_MESSAGE_HANDLE_H_

#include "data/general_message.h"

/// @brief 处理收到的通用数据
/// @param general_message
int8_t handle_host_message(general_message_t *general_message);

#endif //_HOST_MESSAGE_HANDLE_H_
