
#ifndef _UART_CONFIG_H_
#define _UART_CONFIG_H_

#define UART_CONFIG_TASK_NAME  "UART_CONFIG"
#define UART_CONFIG_TASK_DEPTH (1024)

#include <stdint.h>
#include "common/error_type.h"

/// @brief 发送至电脑端（上位机）的数据格式
typedef struct
{
    char command[20];
    char res[20];
    char type[20];
    char data[1200];
} to_pc_data_t;

typedef struct
{
    char *command;
    char *type;
    error_t (*parser)(const char *input, char *output);
} uart_cmd_t;

#define UART_BUFFER_SIZE (2048)

/**
 * @brief 串口配置网关参数
 *
 * @param pvParameters
 */
void uart_config_task(void *pvParameters);
error_t uart_data_parse(const char *message);

#endif
