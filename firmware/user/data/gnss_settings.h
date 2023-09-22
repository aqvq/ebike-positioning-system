/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:32:24
 * @FilePath: \firmware\user\data\gnss_settings.h
 * @Description: GNSS配置信息数据结构实现头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#ifndef __GNSS_SETTINGS_H
#define __GNSS_SETTINGS_H

#include "common/error_type.h"

// gnss配置信息结构体类型
typedef struct
{
    uint8_t gnss_upload_strategy;
    uint8_t gnss_offline_strategy;
    uint8_t gnss_ins_strategy;
    // gnss_mode:
    // bit 0 1 2 3 4 5 6 7
    // bit0: gnss agps enable/disable
    // bit1: gnss apflash enable/disable
    // bit2: gnss autogps enable/disable
    // bit345: gnss mode config @ref ec800m_at_gnss_config
    // bit67: reserved
    uint8_t gnss_mode;
} gnss_settings_t;

// GNSS AGPS 比特信息位置
#define GNSS_AGPS_MSK (1 << 0) ///< 是否开启agps
// GNSS APFLASH 比特信息位置
#define GNSS_APFLASH_MSK (1 << 1) ///< 是否开启apflash
// GNSS AUTOGPS 比特信息位置
#define GNSS_AUTOGPS_MSK (1 << 2) ///< 是否开启autogps
// GNSS 模式配置 比特信息位置
#define GNSS_MODE_CONFIG_MSK (7 << 3) ///< GPS模式配置
// GNSS AGPS 比特信息偏移量
#define GNSS_AGPS_SHIFT (0)
// GNSS APFLASH 比特信息偏移量
#define GNSS_APFLASH_SHIFT (1)
// GNSS AUTOGPS 比特信息偏移量
#define GNSS_AUTOGPS_SHIFT (2)
// GNSS 模式配置 比特信息偏移量
#define GNSS_MODE_CONFIG_SHIFT (3)

/// @brief 读取gnss配置信息
/// @param config gnss配置信息保存位置
/// @return error_t
error_t read_gnss_settings(gnss_settings_t *config);

/// @brief 写入gnss配置信息
/// @param config 待写入的gnss配置信息
/// @return error_t
error_t write_gnss_settings(gnss_settings_t *config);

/// @brief 从json字符串解析gnss配置信息结构体数据
/// @param input 输入的json字符串
/// @param config 输出的gnss配置信息结构体位置
/// @return error_t
error_t gnss_settings_parse(char *input, gnss_settings_t *config);

/// @brief 从gnss配置信息结构体打印json字符串
/// @param config 待打印的gnss配置信息
/// @param output 输出的json字符串缓冲区
/// @param output_len 输出的json字符串长度（可为NULL）
/// @return error_t
error_t gnss_settings_print(gnss_settings_t *config, char *output, uint16_t *output_len);

/// @brief 以json字符串形式写入gnss配置信息
/// @param text json字符串
/// @return error_t
error_t write_gnss_settings_text(const char *text);

/// @brief 以json字符串形式读取gnss配置信息
/// @param text json字符串
/// @return error_t
error_t read_gnss_settings_text(char *text);

#endif
