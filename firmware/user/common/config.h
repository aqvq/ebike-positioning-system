/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 16:23:57
 * @FilePath: \firmware\user\common\config.h
 * @Description: 项目相关配置信息
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

// ========== 一些常用类型的宏定义 ==========

// bool类型定义
typedef uint8_t bool;
// bool true定义
#define true 1
// bool false定义
#define false 0
// NULL定义
#define NULL ((void *)0)

// ========== RAM相关的定义 ==========

// RAM堆大小定义
#define RAM_HEAP_SIZE (120 * 1024)
// CCRAM堆大小定义
// #define CCMRAM_HEAP_SIZE     (64 * 1024)
// RAM起始地址
#define RAM_START_ADDRESS ((uint8_t *)0x20000000)
// RAM总大小
#define RAM_SIZE (128 * 1024)
// CCRAM起始地址
// #define CCMRAM_START_ADDRESS ((uint8_t *)0x10000000)
// CCRAM总大小
// #define CCMRAM_SIZE          (64 * 1024)
// 声明CCRAM区的变量的宏定义
#define CCMRAM __attribute__((section(".ccmram")))
// 声明在RAM区的变量的宏定义
#define RAM __attribute__((section(".ram")))

// ========== 固件版本信息定义 ==========

// 固件版本信息major
#define APP_VERSION_MAJOR (1)
// 固件版本信息minor
#define APP_VERSION_MINOR (1)
// 固件版本信息build
#define APP_VERSION_BUILD (0)

// ========== 功能条件编译开关的定义 ==========

// 串口发送数据给上位机
#define HOST_ENABLED (1)
// 4G通讯功能使能
#define MQTT_ENABLED (1)
// 采集GPS数据使能
#define GNSS_ENABLED (1)
// 用于上位机配置网关参数功能
#define UART_CONFIG_ENABLED (1)

#endif // _COMMON_CONFIG_H_
