
#ifndef _GATEWAY_CONFIG_H_
#define _GATEWAY_CONFIG_H_

#include <stdint.h>
#include "error_type.h"

/// @brief 网关配置参数
typedef struct
{
    char apn[127];
    char apn_username[127];
    char apn_password[127];
} gateway_config_t;

// 提供两种方式读写配置信息
// 1. 以text形式读写
// 2. 以结构体形式读写

// 读取网关配置信息字符串
error_t read_gateway_config_text(char *text, uint16_t *text_len);
// 写入网关配置信息字符串
error_t write_gateway_config_text(const char *text);
// 读取网关配置信息结构体
error_t read_gateway_config(gateway_config_t *config);
// 写入网关配置信息结构体
error_t write_gateway_config(gateway_config_t *config);
// 配置信息json格式的解析
error_t gateway_config_parse(const char *message, gateway_config_t *config);
// 配置信息json格式的打印
error_t gateway_config_print(const gateway_config_t *config, char *output, uint16_t *output_len);

#endif
