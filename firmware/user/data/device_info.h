/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:25:46
 * @FilePath: \firmware\user\data\device_info.h
 * @Description: 设备信息数据结构读写操作实现头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef __DEVICE_INFO_H
#define __DEVICE_INFO_H

#include "common/error_type.h"

/* 白名单模式下用于保存deviceSecret的结构体定义 */
typedef struct
{
    // 设备名，目前使用IMEI作为设备名
    char device_name[20];
    // 设备密钥，通过动态注册获取
    char device_secret[64];
} devinfo_wl_t;

// not used
/* 免白名单模式下用于保存mqtt建连信息clientid, username和password的结构体定义 */
typedef struct
{
    char conn_clientid[128];
    char conn_username[128];
    char conn_password[64];
} devinfo_nwl_t;

/// @brief 读取设备信息
/// @param devinfo_wl 信息保存位置
/// @return error_t
error_t read_device_info(devinfo_wl_t *devinfo_wl);

/// @brief 写入设备信息
/// @param devinfo_wl 待写入的设备信息
/// @return error_t
error_t write_device_info(devinfo_wl_t *devinfo_wl);

/// @brief 解析设备信息结构体数据，将json字符串解析为设备信息结构体内容
/// @param input json字符串
/// @param device 待写入的设备信息结构体
/// @return error_t
error_t device_info_parse(char *input, devinfo_wl_t *device);

/// @brief 打印设备信息结构体数据，将设备信息结构体内容打印为json字符串
/// @param config 待打印的设备信息
/// @param output json字符串输出地址
/// @param output_len json字符串长度（可为NULL）
/// @return error_t
error_t device_info_print(devinfo_wl_t *config, char *output, uint16_t *output_len);

/// @brief 从json字符串中读取设备信息
/// @param text json字符串
/// @return error_t
error_t write_device_info_text(const char *text);

/// @brief 以json字符串形式读取设备信息
/// @param text 字符串缓冲区
/// @return error_t
error_t read_device_info_text(char *text);

#endif // !__DEVICE_INFO_H
