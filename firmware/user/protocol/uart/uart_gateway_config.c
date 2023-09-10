/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:11:17
 * @FilePath: \firmware\user\protocol\uart\uart_gateway_config.c
 */
#include "uart_gateway_config.h"
#include "log/log.h"
#include "string.h"
#include "cJSON.h"
#include "data/gateway_config.h"
#include "utils/util.h"
#include "FreeRTOS.h"
#include "storage/storage.h"
#include "stream_buffer.h"
#include "aliyun/aliyun_dynreg.h"
#include "main/app_main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "data/device_info.h"
#include "data/partition_info.h"

static const char *TAG = "UART_GATEWAY_CONFIG";
#define UART_GATEWAY_CONFIG_NUM UART_NUM_0
#define BUF_SIZE                (1500)
// static QueueHandle_t uart0_queue;

//------------------------------------------全局变量-------------------------------------------------------

extern int g_major_version; /* 当前程序major版本 */
extern int g_minor_version; /* 当前程序minor版本 */
extern int g_patch_version; /* 当前程序patch版本 */

uint8_t uart_recv_data;                  /* 接收到的数据 */
StreamBufferHandle_t uart_stream_buffer; /* 串口接收数据缓冲区 */
extern UART_HandleTypeDef huart2;
volatile flag_restart = 0;

//========================================================================================================
error_t parse_get_gateway_config(const char *input, char *output);
error_t parse_get_device_info(const char *input, char *output);
error_t parse_get_gateway_info(const char *input, char *output);
error_t parse_set_gateway_config(const char *input, char *output);
error_t parse_set_device_info(const char *input, char *output);
error_t parse_restart(const char *input, char *output);
error_t parse_switch(const char *input, char *output);
error_t parse_rollback(const char *input, char *output);
error_t parse_get_appinfo(const char *input, char *output);
error_t parse_set_appinfo(const char *input, char *output);

static uart_cmd_t uart_cmd[] = {
    {.command_type = "GET", .data_type = "GATEWAYCONFIG", .parse_function = &parse_get_gateway_config},
    {.command_type = "SET", .data_type = "GATEWAYCONFIG", .parse_function = &parse_set_gateway_config},
    {.command_type = "GET", .data_type = "DEVICEINFO", .parse_function = &parse_get_device_info},
    {.command_type = "SET", .data_type = "DEVICEINFO", .parse_function = &parse_set_device_info},
    {.command_type = "GET", .data_type = "APPINFO", .parse_function = &parse_get_appinfo},
    {.command_type = "SET", .data_type = "APPINFO", .parse_function = &parse_set_appinfo},
    {.command_type = "GET", .data_type = "GATEWAYINFO", .parse_function = &parse_get_gateway_info},
    {.command_type = "RESTART", .data_type = "STRING", .parse_function = &parse_restart},
    {.command_type = "SWITCH", .data_type = "STRING", .parse_function = &parse_switch},
    {.command_type = "ROLLBACK", .data_type = "STRING", .parse_function = &parse_rollback},
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
    HAL_UART_Receive_IT(&huart2, &uart_recv_data, 1);
}

static int8_t to_pc_data_print(const to_pc_data_t *data, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "command_type", data->command_type);
    cJSON_AddStringToObject(root, "response_type", data->response_type);
    cJSON_AddStringToObject(root, "data_type", data->data_type);
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
static int8_t gateway_info_print(char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();

    char version[30] = {0};
    sprintf(version, "%d.%d.%d", g_major_version, g_minor_version, g_patch_version);

    cJSON_AddStringToObject(root, "version", version);
    cJSON_AddStringToObject(root, "gateway_id", get_device_name());

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    *output_len = (uint16_t)json_string_len;
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

void uart_gateway_config_task(void *pvParameters)
{
    uart_init();
    LOGD(TAG, "uart gateway config task start");
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
                LOGW(TAG, "ble stream buffer is empty or error!");
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
    cJSON *obj_command_type;
    cJSON *obj_data_type;
    cJSON *obj_data;
    to_pc_data_t data = {0};

    if (root == NULL) {
        LOGE(TAG, "JSON Parse Error.");
        return -1;
    }

    obj_command_type = cJSON_GetObjectItem(root, "command_type");
    obj_data_type    = cJSON_GetObjectItem(root, "data_type");
    obj_data         = cJSON_GetObjectItem(root, "data");

    if (obj_command_type != NULL && obj_data_type != NULL && obj_data != NULL) {
        strcpy(data.command_type, obj_command_type->valuestring);
        uint16_t len   = ARRAY_LEN(uart_cmd);
        uint16_t index = 0;
        while (uart_cmd[index].command_type != 0 && uart_cmd[index].data_type != 0 && uart_cmd[index].parse_function != 0) {
            if (strcmp(uart_cmd[index].command_type, obj_command_type->valuestring) == 0 && strcmp(uart_cmd[index].data_type, obj_data_type->valuestring) == 0) {
                int8_t err = uart_cmd[index].parse_function(obj_data->valuestring, data.data);
                if (err == 0) {
                    // 发送数据序列化为JSON字符串
                    strcpy(data.response_type, "OK");
                    strcpy(data.data_type, obj_data_type->valuestring);
                } else // 读取失败，可能时FLASH中无该信息
                {
                    strcpy(data.response_type, "ERROR");
                    strcpy(data.data_type, "STRING");
                    LOGE(TAG, "Parse Error(code: %d) %s.", err, error_string(err));
                }
                break;
            }
            index += 1;
        }
        if (uart_cmd[index].command_type == 0) {
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

error_t parse_get_gateway_config(const char *input, char *output)
{
    uint16_t text_len = 128;
    error_t res;

    /* 优先从Flash中读取配置信息 */
    res = read_gateway_config_text(output, &text_len);

    if (res != 0) {
        strcpy(output, "read flash error");
        return READ_GATEWAY_CONFIG_TEXT_ERROR;
    }
    if (text_len >= 128) {
        return GATEWAY_CONFIG_TEXT_OVERFLOW;
    }
    return OK;
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

error_t parse_get_gateway_info(const char *input, char *output)
{
    char text[128] = {0};
    uint16_t text_len;
    gateway_info_print(text, &text_len);
    strcpy(output, text);
    return 0;
}

error_t parse_set_gateway_config(const char *input, char *output)
{
    // 写FLASH
    int8_t res = write_gateway_config_text(input);

    LOGD(TAG, "%s", input);

    if (res == 0) {
        strcpy(output, "");
        return 0;
    } else {
        strcpy(output, "write gateway config fail");
        return -1;
    }
}

error_t parse_switch(const char *input, char *output)
{
    // 该程序仅供测试
    app_partition_t partition = {0};
    read_partition_info(&partition);
    app_info_t temp;
    memcpy(&temp, &partition.app_current, sizeof(app_info_t));
    memcpy(&partition.app_current, &partition.app_previous, sizeof(app_info_t));
    memcpy(&partition.app_previous, &temp, sizeof(app_info_t));
    write_partition_info(&partition);
    strcpy(output, "");
    flag_restart = 1;
    return 0;
}

error_t parse_rollback(const char *input, char *output)
{
    app_info_t app = {0};
    read_app_previous(&app);

    if (app.enabled == APP_INFO_ENABLED_FLAG) // id正确
    {
        write_app_current(&app);
        app.enabled = 0;
        write_app_previous(&app);
        strcpy(output, "");
        flag_restart = 1;
        return 0;
    } else {
        strcpy(output, "rollback failed");
        return -1;
    }
    return 0;
}

error_t parse_get_appinfo(const char *input, char *output)
{
    // 发送数据序列化为JSON字符串
    int8_t res = read_partition_info_text(output); // 从FLASH读取配置
    if (res == 0) {
        return 0;
    } else {
        // 读取失败，可能时FLASH中无该信息
        strcpy(output, "read app info error");
        return -1;
    }
}

error_t parse_set_appinfo(const char *input, char *output)
{
    printf("valuestring=%s\r\n", input);

    // 写FLASH
    int8_t res = write_partition_info_text(input);
    if (res == 0) {
        strcpy(output, "");
        return 0;
    } else {
        strcpy(output, "write app info fail");
        printf("error: %d\n", res);
        return -1;
    }
    return 0;
}
