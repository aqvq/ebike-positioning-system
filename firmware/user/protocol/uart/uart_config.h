/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 19:00:59
 * @FilePath: \firmware\user\protocol\uart\uart_config.h
 * @Description: 串口配置头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef _UART_CONFIG_H_
#define _UART_CONFIG_H_

#include <stdint.h>
#include "common/error_type.h"

// 串口配置任务名
#define UART_CONFIG_TASK_NAME "UART_CONFIG"
// 串口配置任务栈大小
#define UART_CONFIG_TASK_DEPTH (1024)
// 串口接收缓冲区大小
#define UART_BUFFER_SIZE (2048)
// （回应数据的）json字符串缓冲区大小
#define BUF_SIZE (1500)


/// @brief 发送至电脑端（上位机）的数据格式
typedef struct
{
    char command[20];
    char res[20];
    char type[20];
    char data[1200];
} to_pc_data_t;

/// @brief 串口命令格式
typedef struct
{
    char *command;
    char *type;
    error_t (*parser)(const char *input, char *output);
} uart_cmd_t;

/**
 * @brief 串口配置网关参数
 *
 * @param pvParameters 可忽略
 */
void uart_config_task(void *pvParameters);

/**
 * @brief 解析接收到的串口数据
 *
 * @param message 接收到的串口消息
 * @return error_t
 */
error_t uart_data_parse(const char *message);

#endif
