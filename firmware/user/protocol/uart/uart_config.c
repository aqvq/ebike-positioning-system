/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:11:17
 * @FilePath: \firmware\user\protocol\uart\uart_config.c
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
#define BUF_SIZE (1500)
//------------------------------------------全局变量-------------------------------------------------------

uint8_t uart_recv_data;                  /* 接收到的数据 */
StreamBufferHandle_t uart_stream_buffer; /* 串口接收数据缓冲区 */
extern UART_HandleTypeDef huart2;
volatile uint8_t flag_restart = 0;

//========================================================================================================
error_t parse_get_device_info(const char *input, char *output);
error_t parse_set_device_info(const char *input, char *output);
error_t parse_get_gnss_settings(const char *input, char *output);
error_t parse_set_gnss_settings(const char *input, char *output);
error_t parse_get_version(const char *input, char *output);
error_t parse_restart(const char *input, char *output);
error_t parse_swapbank(const char *input, char *output);
error_t parse_upgrade(const char *input, char *output);

static uart_cmd_t uart_cmd[] = {
    {.command = "DEVICEINFO", .type = "GET", .parser = &parse_get_device_info},
    {.command = "DEVICEINFO", .type = "SET", .parser = &parse_set_device_info},
    {.command = "GNSS", .type = "GET", .parser = &parse_get_gnss_settings},
    {.command = "GNSS", .type = "SET", .parser = &parse_set_gnss_settings},
    {.command = "VERSION", .type = "", .parser = &parse_get_version},
    {.command = "RESTART", .type = "", .parser = &parse_restart},
    {.command = "SWAPBANK", .type = "", .parser = &parse_swapbank},
    {.command = "UPGRADE", .type = "", .parser = &parse_upgrade},
    {0, 0, 0}, // TODO: 以0结尾的方式比较灵活，后续可以传入指针
};

//========================================================================================================

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

static int8_t to_pc_data_print(const to_pc_data_t *data, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command", data->command);
    cJSON_AddStringToObject(root, "res", data->res);
    cJSON_AddStringToObject(root, "type", data->type);
    cJSON_AddStringToObject(root, "data", data->data);

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    *output_len = (uint16_t)json_string_len;
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

/// @brief 获得网关信息JSON字符串
/// @param output
/// @param output_len
/// @return
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

void uart_config_task(void *pvParameters)
{
    uart_init();
    LOGD(TAG, "uart config task start");
    uint8_t *data = (uint8_t *)pvPortMalloc(sizeof(uint8_t) * UART_BUFFER_SIZE);
    if (data == NULL) {
        LOGE(TAG, "malloc failed");
        Error_Handler();
    }
    uint16_t i = 0;
    for (;;) {
        if (!uart_stream_buffer) {
            LOGE(TAG, "uart stream buffer not init");
            Error_Handler();
        }
        extern uint8_t g_app_upgrade_flag;
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
            if (xStreamBufferReceive(uart_stream_buffer, &data[i], 1, portMAX_DELAY) == 0) {
                LOGW(TAG, "uart stream buffer is empty or error!");
            }
        } while (data[i++] != '\n'); // 接收一行数据
        data[i] = '\0';

        int len = i;
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

    obj_command = cJSON_GetObjectItem(root, "command");
    obj_type    = cJSON_GetObjectItem(root, "type");
    obj_data    = cJSON_GetObjectItem(root, "data");

    if (obj_command != NULL && obj_type != NULL && obj_data != NULL) {
        strcpy(data.command, obj_command->valuestring);
        uint16_t len   = array_size(uart_cmd);
        uint16_t index = 0;
        while (uart_cmd[index].command != 0 && uart_cmd[index].type != 0 && uart_cmd[index].parser != 0) {
            if (strcmp(uart_cmd[index].command, obj_command->valuestring) == 0 && strcmp(uart_cmd[index].type, obj_type->valuestring) == 0) {
                int8_t err = uart_cmd[index].parser(obj_data->valuestring, data.data);
                if (err == 0) {
                    // 发送数据序列化为JSON字符串
                    strcpy(data.res, "OK");
                    strcpy(data.type, obj_type->valuestring);
                } else // 读取失败，可能时FLASH中无该信息
                {
                    strcpy(data.res, "ERROR");
                    strcpy(data.type, "STRING");
                    LOGE(TAG, "Parse Error(code: %d) %s.", err, error_string(err));
                }
                break;
            }
            index += 1;
        }
        if (uart_cmd[index].command == 0) {
            LOGE(TAG, "Lack of corresponding analytic function");
            cJSON_Delete(root);
            return -3;
        }
    } else {
        LOGE(TAG, "Data missing related fields");
        cJSON_Delete(root);
        return -2;
    }

    char txt[BUF_SIZE] = {0};
    uint16_t txt_len;
    to_pc_data_print(&data, txt, &txt_len);
    LOGI(TAG, "%s\n", txt);

    if (flag_restart) {
        ec800m_poweroff_and_mcu_restart();
    }

    cJSON_Delete(root);
    return 0;
}

error_t parse_get_device_info(const char *input, char *output)
{
    error_t err = read_device_info_text(output);
    if (err != OK) {
        strcpy(output, "get device info error");
    }
    return err;
}

error_t parse_set_device_info(const char *input, char *output)
{
    error_t err = OK;
    err         = write_device_info_text(input);
    if (err != OK) {
        strcpy(output, "write device info error");
    }
    return err;
}

error_t parse_restart(const char *input, char *output)
{
    strcpy(output, "");
    // TODO: 可改善的重启方式
    flag_restart = 1;
    return 0;
}

error_t parse_get_version(const char *input, char *output)
{
    char text[128] = {0};
    uint16_t text_len;
    version_info_print(text, &text_len);
    strcpy(output, text);
    return 0;
}

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

error_t parse_set_gnss_settings(const char *input, char *output)
{
    printf("valuestring=%s\r\n", input);

    // 写FLASH
    int8_t res = write_gnss_settings_text(input);
    if (res == 0) {
        strcpy(output, "");
        return 0;
    } else {
        strcpy(output, "write gnss settings failed");
        printf("error: %d\n", res);
        return -1;
    }
    return 0;
}

error_t parse_swapbank(const char *input, char *output)
{
    // 仅供测试
    boot_swap_bank(); // 会立即重启
    return 0;
}

error_t parse_upgrade(const char *input, char *output)
{
    LOGI(TAG, "Upgrade program, about to start from bootloader...");
    boot_jump_to_bootloader();
    // Will not be executed here
}
