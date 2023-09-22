/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 14:38:47
 * @FilePath: \firmware\user\bsp\at\at.h
 * @Description: AT驱动命令头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef __EC800M_AT_API_H
#define __EC800M_AT_API_H

#include <stdio.h>
#include <string.h>
#include "aiot_at_api.h"
#include "common/error_type.h"
#include "log/log.h"

#include "aiot_state_api.h"
#include "utils/macros.h"

// at驱动依赖的阿里云SDK相关内容声明
extern core_at_handle_t at_handle;
int32_t core_at_commands_send_sync(const core_at_cmd_item_t *cmd_list, uint16_t cmd_num);

// 下面是at驱动层的所有命令函数声明

/************    at device    ************/

/// @brief ec800m初始化
/// @return error_t
error_t ec800m_init();

/// @brief 初始化at功能
/// @param
/// @return
int32_t at_hal_init(void);

/// @brief ec800m关机
/// @return
int32_t ec800m_at_poweroff();

/// @brief 获取imei序列号，此序列号是设备唯一序列号，可用于识别设备ID，例如：861197068734963
/// @param imei 返回imei序列号
/// @return 状态码 <0失败 >=0成功
int32_t ec800m_at_imei(char *imei);

/// @brief 设备是否打开
/// @return 返回值 < 0 关闭或者异常, >=0 打开
int32_t ec800m_state();

/************    at tcp    ************/

/// @brief 关闭TCP连接
/// @param
/// @return 状态码 <0失败 >=0成功
int32_t ec800m_at_tcp_close();

/// @brief 场景是否打开
/// @param id 返回值id存储位置
/// @param state 返回值state存储位置
/// @return 状态码 <0失败 >=0成功
int32_t ec800m_at_context_state(uint8_t *id, uint8_t *state);

/// @brief TCP是否打开
/// @param id 返回值id存储位置
/// @param state 返回值state存储位置
/// @return 状态码 <0失败 >=0成功
int32_t ec800m_at_tcp_state(uint8_t *id, uint8_t *state);

/************    at gnss    ************/

/**
 * @brief 打开 GNSS
 *
 * @return int32_t < 0 操作失败, >=0 操作成功,返回已经消费掉的数据
 */
int32_t ec800m_at_gnss_open();

/**
 * @brief GNSS是否打开
 * @param state < 0 GNSS关闭或者异常, >=0 GNSS打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_state(uint8_t *state);

/**
 * @brief GNSS 定位
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_location();

/**
 * @brief 关闭 GNSS
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_close();

/**
 * @brief GNSS 定位设置
 * @param mode
 * 0 GPS
 * 1 GPS + BeiDou
 * 3 GPS + GLONASS + Galileo
 * 4 GPS + GLONASS
 * 5 GPS + BeiDou + Galileo
 * 6 GPS + Galileo
 * 7 BeiDou
 * @return int32_t
 */
int32_t ec800m_at_gnss_config(uint8_t mode);

/************    at gnss nmea    ************/

/**
 * @brief GNSS 使能通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nmea_enable();

/**
 * @brief GNSS 禁用通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nmea_disable();

/**
 * @brief GNSS 获取 GGA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nmea_query();

/************    at gnss agps    ************/

/**
 * @brief GNSS AGPS功能打开
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_enable_agps();

/**
 * @brief GNSS AGPS功能关闭
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_disable_agps();

/**
 * @brief AGPS是否打开
 * @param state < 0 AGPS关闭或者异常, >=0 AGPS打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_agps_state(uint8_t *state);

/************    at gnss apflash    ************/

/**
 * @brief 启用 AP-Flash 快速热启动功能
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_enable_apflash();

/**
 * @brief 禁用 AP-Flash 快速热启动功能
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_disable_apflash();

/**
 * @brief apflash是否打开
 * @param state =0 apflash关闭或者异常, =1 apflash打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_apflash_state(uint8_t *state);

/************    at gnss autogps    ************/

/**
 * @brief 启用 GNSS 自启动
 *
 * @return int32_t0
 */
int32_t ec800m_at_gnss_enable_autogps();

/**
 * @brief 禁用 GNSS 自启动
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_disable_autogps();

/**
 * @brief autogps是否打开
 * @param state =0 autogps关闭或者异常, =1 autogps打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_autogps_state(uint8_t *state);

#endif
