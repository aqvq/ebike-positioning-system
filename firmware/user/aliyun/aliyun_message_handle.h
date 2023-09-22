/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:42:06
 * @FilePath: \firmware\user\aliyun\aliyun_message_handle.h
 * @Description: 阿里云上传数据
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _ALIYUN_MESSAGE_HANDLE_H_
#define _ALIYUN_MESSAGE_HANDLE_H_

#include "data/general_message.h"

/// @brief 处理收到的通用数据
/// @param handle mqtt会话句柄
/// @param adv_packet
void handle_aliyun_message(void *handle, general_message_t *general_message);

#endif // _ALIYUN_MESSAGE_HANDLE_H_
