/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:30:12
 * @FilePath: \firmware\user\aliyun\aliyun_dynreg.h
 * @Description: 阿里云动态注册头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _ALIYUN_DYNREG_H_
#define _ALIYUN_DYNREG_H_

#include "data/device_info.h"

/**
 * @brief 判断设备是否已经注册
 *
 * @return uint8_t
 */
uint8_t is_registered();

/**
 * @brief 动态注册
 *
 * @return int8_t
 */
int8_t dynamic_register();

#endif // _ALIYUN_DYNREG_H_
