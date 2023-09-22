/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 19:34:40
 * @FilePath: \firmware\user\protocol\uart\uart_config.c
 * @Description: 串口配置实现
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#include "uart_config.h"
#include "log/log.h"
#include "string.h"
#include "cJSON.h"
#include "utils/util.h"
#include "FreeRTOS.h"
#include "storage/storage.h"
#include "stream_buffer.h"
#include "aliyun/aliyun_dynreg.h"
#include "main/app_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "data/device_info.h"
#include "bsp/flash/boot.h"
#include "data/gnss_settings.h"
#include "utils/macros.h"

#define TAG      "UART_CONFIG"
//------------------------------------------全局变量-------------------------------------------------------

/* 接收到的数据 */
uint8_t uart_recv_data;
/* 串口接收数据缓冲区 */
StreamBufferHandle_t uart_stream_buffer;
/* stm32 hal串口实例 */
extern UART_HandleTypeDef huart2;
/* 重启标志位，当置位时串口配置任务会择机重启单片机 */
volatile uint8_t flag_restart = 0;

//========================================================================================================

// 串口命令解析函数声明

error_t parse_get_device_info(const char *input, char *output);
error_t parse_set_device_info(const char *input, char *output);
error_t parse_get_gnss_settings(const char *input, char *output);
error_t parse_set_gnss_settings(const char *input, char *output);
error_t parse_get_version(const char *input, char *output);
error_t parse_restart(const char *input, char *output);
error_t parse_swapbank(const char *input, char *output);
error_t parse_upgrade(const char *input, char *output);

// 串口命令在此处集中定义，便于扩展
static uart_cmd_t uart_cmd[] = {
    // 对设备信息进行读写
    {.command = "DEVICEINFO", .type = "GET", .parser = &parse_get_device_info},
    {.command = "DEVICEINFO", .type = "SET", .parser = &parse_set_device_info},
    // 对GNSS配置信息读写
    {.command = "GNSS", .type = "GET", .parser = &parse_get_gnss_settings},
    {.command = "GNSS", .type = "SET", .parser = &parse_set_gnss_settings},
    // 获取固件版本信息
    {.command = "VERSION", .type = "", .parser = &parse_get_version},
    // 重启单片机
    {.command = "RESTART", .type = "", .parser = &parse_restart},
    // 【仅调试使用】交换Bank
    {.command = "SWAPBANK", .type = "", .parser = &parse_swapbank},
    // 【仅调试使用】通过调试串口进行IAP升级
    {.command = "UPGRADE", .type = "", .parser = &parse_upgrade},
    // 以0结尾的方式比较灵活，后续可以传入指针
    {0, 0, 0},
};

//========================================================================================================

// 调试串口初始化
void uart_init()
{
    uart_stream_buffer = xStreamBufferCreate(UART_BUFFER_SIZE, 1);
    if (uart_stream_buffer == NULL) {
        LOGE(TAG, "uart stream buffer created error");
        Error_Handler();
    }
    if (HAL_OK != HAL_UART_Receive_IT(&huart2, &uart_recv_data, 1)) {
        LOGE(TAG, "uart receive interrupt error");
        Error_Handler();
    }
}

// 调试串口接收中断ISR
void _usart_config_recv_isr(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        if (uart_stream_buffer) {
            xStreamBufferSendFromISR(uart_stream_buffer, &uart_recv_data, 1, NULL);
        }
        if (HAL_OK != HAL_UART_Receive_IT(&huart2, &uart_recv_data, 1)) {
            LOGE(TAG, "uart receive interrupt error");
            Error_Handler();
        }
    }
}

/// @brief 将to_pc_data_t结构体打印成json字符串
/// @param data 待打印的结构体数据
/// @param output json字符串输出缓冲区
/// @param output_len json字符串长度，可为NULL
/// @return int8_t
static int8_t to_pc_data_print(const to_pc_data_t *data, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", data->command);
    cJSON_AddStringToObject(root, "res", data->res);
    cJSON_AddStringToObject(root, "type", data->type);
    cJSON_AddStringToObject(root, "data", data->data);

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    memcpy((void *)output, (void *)json_string, json_string_len);
    if (output_len) {
        *output_len = (uint16_t)json_string_len;
    }

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

/// @brief 获得网关信息JSON字符串
/// @param output 输出json字符串缓冲区
/// @param output_len json字符串长度
/// @return int8_t
static int8_t version_info_print(char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();

    char version[30] = {0};
    sprintf(version, "%d.%d.%d", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);

    cJSON_AddStringToObject(root, "version", version);
    cJSON_AddStringToObject(root, "device", get_device_name());

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    *output_len = (uint16_t)json_string_len;
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

/// @brief 串口配置任务
/// @param pvParameters 可忽略
void uart_config_task(void *pvParameters)
{
    // 初始化调试串口
    uart_init();
    LOGD(TAG, "uart config task start");
    // 分配接收缓冲区内存
    uint8_t *data = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * UART_BUFFER_SIZE);
    if (data == NULL) {
        LOGE(TAG, "malloc failed");
        Error_Handler();
    }
    uint16_t i = 0;
    for (;;) {
        // 判断流缓冲区是否已经初始化
        if (!uart_stream_buffer) {
            LOGE(TAG, "uart stream buffer not init");
            Error_Handler();
        }

        // app升级标志
        extern uint8_t g_app_upgrade_flag;
        // 如果正在进行程序升级，退避一段时间
        if (g_app_upgrade_flag != 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        i = 0;
        do {
            // 缓冲区溢出处理
            if (i >= UART_BUFFER_SIZE) {
                LOGE(TAG, "buffer overflow");
                LOGE(TAG, "Buffer Info:");
                for (uint16_t k = 0; k < UART_BUFFER_SIZE; ++k) {
                    putchar(data[k]);
                }
                putchar('\n');
                Error_Handler();
            }

            // 数据流动方向：
            // 触发中断 -> uart_recv_data -> uart_stream_buffer -> data
            if (xStreamBufferReceive(uart_stream_buffer, &data[i], 1, portMAX_DELAY) == 0) {
                LOGW(TAG, "uart stream buffer is empty or error!");
            }
        } while (data[i++] != '\n'); // 接收一行数据
        data[i] = '\0';              // 字符串以0结尾

        int len = i;
        // 判断接收到的数据是否合法
        if (len > 2 && data[0] == '{' && data[len - 3] == '}') {
            len -= 2;
            data[len] = '\0';
            LOGI(TAG, "data=%s rxBytes=%d", data, len);
            uart_data_parse(data);
        } else {
            LOGE(TAG, "format error (%s)", data);
        }
        LOGD(TAG, "Free Heap Size: %d", xPortGetFreeHeapSize());
    }
    vPortFree(data);
    vTaskDelete(NULL);
    // return 0;
}

error_t uart_data_parse(const char *message)
{
    cJSON *root = cJSON_Parse(message);
    cJSON *obj_command;
    cJSON *obj_type;
    cJSON *obj_data;
    to_pc_data_t data = {0};

    if (root == NULL) {
        LOGE(TAG, "JSON Parse Error.");
        return -1;
    }

    // 获取命令各个部分
    obj_command = cJSON_GetObjectItem(root, "command");
    obj_type    = cJSON_GetObjectItem(root, "type");
    obj_data    = cJSON_GetObjectItem(root, "data");

    // 检查命令格式是否正确、完整
    if (obj_command != NULL && obj_type != NULL && obj_data != NULL) {
        strcpy(data.command, obj_command->valuestring);
        uint16_t len   = array_size(uart_cmd);
        uint16_t index = 0;
        while (uart_cmd[index].command != 0 && uart_cmd[index].type != 0 && uart_cmd[index].parser != 0) {
            // 与命令表格进行匹配
            if (strcmp(uart_cmd[index].command, obj_command->valuestring) == 0 && strcmp(uart_cmd[index].type, obj_type->valuestring) == 0) {
                // 若匹配成功，执行相应的解析函数
                int8_t err = uart_cmd[index].parser(obj_data->valuestring, data.data);
                // 如果解析函数正确返回，返回一个带有OK的json字符串
                if (err == 0) {
                    // 发送数据序列化为JSON字符串
                    strcpy(data.res, "OK");
                    strcpy(data.type, obj_type->valuestring);
                } else {
                    // 读取失败，返回错误信息
                    strcpy(data.res, "ERROR");
                    strcpy(data.type, "STRING");
                    LOGE(TAG, "Parse Error(code: %d) %s.", err, error_string(err));
                }
                break;
            }
            index += 1;
        }
        // 未匹配到命令
        if (uart_cmd[index].command == 0) {
            LOGE(TAG, "Lack of corresponding analytic function");
            cJSON_Delete(root);
            return -3;
        }
    } else {
        // 命令不完整或者格式错误
        LOGE(TAG, "Data missing related fields");
        cJSON_Delete(root);
        return -2;
    }

    char txt[BUF_SIZE] = {0};
    uint16_t txt_len;
    // 将回应数据转为json字符串
    to_pc_data_print(&data, txt, &txt_len);
    LOGI(TAG, "%s\n", txt);

    // 如果重启标志变量被置位，在此刻重启单片机
    if (flag_restart) {
        ec800m_poweroff_and_mcu_restart();
    }

    cJSON_Delete(root);
    return 0;
}

/// @brief 获取device_info
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_get_device_info(const char *input, char *output)
{
    error_t err = read_device_info_text(output);
    if (err != OK) {
        strcpy(output, "get device info error");
    }
    return err;
}

/// @brief 设置device_info
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_set_device_info(const char *input, char *output)
{
    error_t err = OK;
    err         = write_device_info_text(input);
    if (err != OK) {
        strcpy(output, "write device info error");
    }
    return err;
}

/// @brief 重启设备
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_restart(const char *input, char *output)
{
    strcpy(output, "");
    // TODO: 可改善的重启方式
    flag_restart = 1;
    return 0;
}

/// @brief 获取固件版本
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_get_version(const char *input, char *output)
{
    char text[128] = {0};
    uint16_t text_len;
    version_info_print(text, &text_len);
    strcpy(output, text);
    return 0;
}

/// @brief 获取gnss配置信息
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_get_gnss_settings(const char *input, char *output)
{
    // 发送数据序列化为JSON字符串
    int8_t res = read_gnss_settings_text(output); // 从FLASH读取配置
    if (res == 0) {
        return 0;
    } else {
        // 读取失败，可能时FLASH中无该信息
        strcpy(output, "read gnss settings error");
        return -1;
    }
}

/// @brief 设置gnss配置信息
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_set_gnss_settings(const char *input, char *output)
{
    // printf("valuestring=%s\r\n", input);

    // 写FLASH
    int8_t res = write_gnss_settings_text(input);
    if (res == 0) {
        strcpy(output, "");
        return 0;
    } else {
        strcpy(output, "write gnss settings failed");
        // printf("error: %d\n", res);
        return -1;
    }
    return 0;
}

/// @brief 交换bank
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_swapbank(const char *input, char *output)
{
    // 仅供测试
    // 将空闲bank映射到0x0800 0000
    boot_swap_bank(); // 会立即重启
    return 0;
}

/// @brief 从调试串口升级程序，原理是启动系统bootloader，然后进行iap
/// @param input 输入数据
/// @param output 输出数据
/// @return error_t
error_t parse_upgrade(const char *input, char *output)
{
    // 仅供测试
    LOGI(TAG, "Upgrade program, about to start from bootloader...");
    // 启动系统bootloader
    boot_jump_to_bootloader();
    // Will not be executed here
}
