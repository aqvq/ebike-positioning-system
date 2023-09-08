
#ifndef _UART_GATEWAY_CONFIG_H_
#define _UART_GATEWAY_CONFIG_H_

#define UART_GATEWAY_CONFIG_TASK_NAME  "UART_CONFIG"
#define UART_GATEWAY_CONFIG_TASK_DEPTH (1024)

#include <stdint.h>
#include "error_type.h"

/// @brief 发送至电脑端（上位机）的数据格式
typedef struct
{
    char command_type[20];
    char response_type[20];
    char data_type[20];
    char data[1200];
} to_pc_data_t;

typedef struct
{
    char *command_type;
    char *data_type;
    error_t (*parse_function)(const char *input, char *output);
} uart_cmd_t;

#define UART_BUFFER_SIZE (2048)
#define ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))

/**
 * @brief 串口配置网关参数
 *
 * @param pvParameters
 */
void uart_gateway_config_task(void *pvParameters);
error_t uart_data_parse(const char *message);

#endif // _DAJIN_PROTOCOL_H_