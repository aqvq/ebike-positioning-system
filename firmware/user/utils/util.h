/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:29:04
 * @FilePath: \firmware\user\utils\util.h
 * @Description: 实用工具函数头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>
#include <stdlib.h>

/// @brief 获取设备名
/// @return char * 设备名
char *get_device_name();

/// @brief 计算平均值
/// @param data
/// @param len
/// @return
float mean(float *data, uint16_t len);

#endif // _UTIL_H_
