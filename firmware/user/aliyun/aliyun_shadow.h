/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:57:47
 * @FilePath: \firmware\user\aliyun\aliyun_shadow.h
 * @Description: 阿里云设备影子头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef _ALIYUN_SHADOW_H_
#define _ALIYUN_SHADOW_H_

#include "aiot_shadow_api.h"

// 设备影子处理函数
void demo_shadow_recv_handler(void *handle, const aiot_shadow_recv_t *recv, void *userdata);

// 报告当前的配置信息
int8_t update_shadow(void *shadow_handle);

/* 发送获取设备影子的请求 */
int32_t demo_get_shadow(void *shadow_handle);

#endif