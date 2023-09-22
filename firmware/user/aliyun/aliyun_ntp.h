/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:45:49
 * @FilePath: \firmware\user\aliyun\aliyun_ntp.h
 * @Description: 阿里云NTP头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _ALIYUN_NTP_H_
#define _ALIYUN_NTP_H_

#include <stdlib.h>
#include <stdint.h>

/// @brief 阿里云NTP功能初始化
/// @param mqtt_handle
/// @return
int32_t aliyun_ntp_init(void **handle);

/// @brief 阿里云NTP结束会话
/// @return
int32_t aliyun_ntp_deinit(void);

#endif