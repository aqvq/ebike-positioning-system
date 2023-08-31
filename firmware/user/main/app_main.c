/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:07:57
 * @FilePath: \firmware\user\main\app_main.c
 */

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "log/log.h"
#include "core/gateway_config.h"
#include "app_config.h"
#include "protocol/host/host_protocol.h"
#include "cJSON.h"
#include "storage/storage.h"
#include "protocol/iot/iot_helper.h"
#include "4g/at_api.h"
#include "4g/hal_adapter.h"
#include "app_main.h"
#include "protocol/uart/uart_gateway_config.h"
#include "gnss/gnss.h"
#include "utils/util.h"
#include "error_type.h"
#include "storage/device/hg24c64.h"

static const char *TAG = "MAIN";
volatile uint32_t ulHighFrequencyTimerTicks;
char g_ec200_apn_cmd[256];

//--------------------------------------全局变量---------------------------------------------

// 网关参数，开机时从FLASH中读取；若读取失败，则使用如下默认值
gateway_config_t g_gateway_config = {.apn          = "cmiot",
                                     .apn_username = "",
                                     .apn_password = ""};

// EC200模块APN设置AT指令
char g_ec200_apn_cmd[256];

/* OTA升级标志，定义在main.c中 */
uint8_t g_app_upgrade_flag = 0;

/* 传感器BLE包数计数(MQTT通讯有效),定义在main.c中 */
uint16_t g_ble_num_mqtt = 0;

int g_major_version = 0; /* 当前程序major版本 */
int g_minor_version = 0; /* 当前程序minor版本 */
int g_patch_version = 0; /* 当前程序patch版本 */

void print_welcome_message(void)
{
    LOG("*************************************************************");
    LOG("*                                                           *");
    LOG("*              THIS IS E-Bike Positioning Device            *");
    LOG("*                SOFTWARE VERSION: %01d.%01d.%02d                   *", g_major_version, g_minor_version, g_patch_version);
    LOG("*                                                           *");
#if HOST_ENABLED
    LOG("*                        HOST_ENABLED                       *");
#endif
#if MQTT_ENABLED
    LOG("*                        MQTT_ENABLED                       *");
#endif
#if UART_GATEWAY_CONFIG_ENABLED
    LOG("*                 UART_GATEWAY_CONFIG_ENABLED               *");
#endif
    LOG("*                  Created By Shang Wentao                  *");
    LOG("*                                                           *");
    LOG("*************************************************************");
}

static void init_gateway_config(void)
{
#if 0 // TODO: Not implemented
    LOGI(TAG, "--- gateway config ---");

    int8_t res = read_gateway_config(&g_gateway_config);
    if (res != OK) {
        LOGI(TAG, "read gateway config failed (%d)", res);
    }

    // EC200模块APN设置AT指令
    sprintf(g_ec200_apn_cmd, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n", g_gateway_config.apn, g_gateway_config.apn_username, g_gateway_config.apn_password);

    LOGI(TAG, "apn=%s", g_gateway_config.apn);
    LOGI(TAG, "apn_username=%s", g_gateway_config.apn_username);
    LOGI(TAG, "apn_password=%s", g_gateway_config.apn_password);
    // LOGI(TAG, "apn_cmd=%s", g_ec20");

    LOGI(TAG, "----------------------");
#endif
}

void cjson_init()
{
    cJSON_Hooks cJSONhooks_freeRTOS = {0};
    cJSONhooks_freeRTOS.malloc_fn   = pvPortMalloc;
    cJSONhooks_freeRTOS.free_fn     = vPortFree;
    cJSON_InitHooks(&cJSONhooks_freeRTOS);
}

void stm32_restart()
{
    __ASM volatile("cpsid i"); // 关中断
    HAL_NVIC_SystemReset();    // 重启
}

void ec200_poweroff_and_mcu_restart(void)
{
#if MQTT_ENABLED || GNSS_ENABLED
    iot_disconnect();   // 断开连接
    aiot_at_poweroff(); // 关闭整个4G模块
#endif
    stm32_restart();
}

void get_running_version(void)
{
    app_info_t app_info;
    read_app_current(&app_info);
    g_major_version = app_info.version_major;
    g_minor_version = app_info.version_minor;
    g_patch_version = app_info.version_patch;
}

void app_main(void)
{
    int8_t err = 0;

    // 初始化存储
    storage_interface_t storage_interface = {0};
#if 0
    // 使用ram做测试，不使用存储模块
    storage_interface.storage_base_address        = pvPortMalloc(sizeof(storage_data_t));
    storage_interface.storage_read_data_func  = storage_default_read;
    storage_interface.storage_write_data_func = storage_default_write;
    storage_interface.storage_init_func       = NULL;
#endif
    storage_interface.storage_base_address    = 0x00;
    storage_interface.storage_read_data_func  = Storage_Read_Buffer;
    storage_interface.storage_write_data_func = Storage_Write_Buffer;
    storage_interface.storage_init_func       = Storage_Ready;

    err = storage_install_interface(&storage_interface);
    if (err != OK) {
        LOGD(TAG, "storage init failed: %s", error_string(err));
    }

    // 初始化cjosn库
    cjson_init();
    // 初始化网关参数
    init_gateway_config();
    // 获取网关固件版本
    get_running_version();
    // 打印开机信息
    print_welcome_message();
#if UART_GATEWAY_CONFIG_ENABLED
    xTaskCreate(uart_gateway_config_task, UART_GATEWAY_CONFIG_TASK_NAME, UART_GATEWAY_CONFIG_TASK_DEPTH, NULL, 3, NULL);
#endif

#if GNSS_ENABLED || MQTT_ENABLED
    // 初始化4G模块
    xTaskCreate(task_4g, "at_init", 5120, NULL, 3, NULL);
#endif

#if HOST_ENABLED
    xTaskCreate(host_protocol_task, HOST_PROTOCOL_TASK_NAME, 512, NULL, 3, NULL);
#endif

    vTaskDelete(NULL);
}
