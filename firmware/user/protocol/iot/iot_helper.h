/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:57:24
 * @FilePath: \firmware\user\protocol\iot\iot_helper.h
 * @Description: iot协议实现头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef _IOT_HELPER_H_
#define _IOT_HELPER_H_

#include "iot_interface.h"

/// @brief 连接IoT平台
int32_t iot_connect(void);

/// @brief 断开连接IoT平台
int32_t iot_disconnect(void);

/// @brief 向Iot平台发送日志
/// @param level 日志级别
/// @param module_name 模块名称
/// @param code 状态码
/// @param content 日志内容
/// @return 0正常 其他异常
int32_t iot_post_log(iot_log_level_t level, char *module_name, int32_t code, char *content);

#endif
