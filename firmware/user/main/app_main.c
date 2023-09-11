/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:07:57
 * @FilePath: \firmware\user\main\app_main.c
 */

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "log/log.h"
#include "data/gateway_config.h"
#include "app_config.h"
#include "protocol/host/host_protocol.h"
#include "cJSON.h"
#include "storage/storage.h"
#include "protocol/iot/iot_helper.h"
#include "aiot_at_api.h"
#include "bsp/at/at.h"
#include "app_main.h"
#include "protocol/uart/uart_gateway_config.h"
#include "gnss/gnss.h"
#include "utils/util.h"
#include "error_type.h"
#include "bsp/eeprom/at24c64.h"
#include "bsp/mcu/mcu.h"
#include "data/partition_info.h"
#include "bsp/flash/boot.h"
#include "upgrade/iap.h"
#include "bsp/flash/flash.h"

static const char *TAG = "MAIN";
volatile uint32_t ulHighFrequencyTimerTicks;
char g_ec800m_apn_cmd[256];

//--------------------------------------全局变量---------------------------------------------

// 网关参数，开机时从FLASH中读取；若读取失败，则使用如下默认值
gateway_config_t g_gateway_config = {.apn          = "cmiot",
                                     .apn_username = "",
                                     .apn_password = ""};

// EC800m模块APN设置AT指令
char g_ec800m_apn_cmd[256];

/* OTA升级标志，定义在main.c中 */
uint8_t g_app_upgrade_flag = 0;

/* 传感器BLE包数计数(MQTT通讯有效),定义在main.c中 */
uint16_t g_ble_num_mqtt = 0;

int g_major_version = 0; /* 当前程序major版本 */
int g_minor_version = 0; /* 当前程序minor版本 */
int g_patch_version = 0; /* 当前程序patch版本 */
extern void app_init();

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
    LOGI(TAG, "--- gateway config ---");

    int8_t res = read_gateway_config(&g_gateway_config);
    if (res != OK) {
        LOGI(TAG, "read gateway config failed (%d)", res);
    }

    // EC800m模块APN设置AT指令
    sprintf(g_ec800m_apn_cmd, "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",1\r\n", g_gateway_config.apn, g_gateway_config.apn_username, g_gateway_config.apn_password);

    LOGI(TAG, "apn=%s", g_gateway_config.apn);
    LOGI(TAG, "apn_username=%s", g_gateway_config.apn_username);
    LOGI(TAG, "apn_password=%s", g_gateway_config.apn_password);
    LOGI(TAG, "apn_cmd=%s", g_ec800m_apn_cmd);
    LOGI(TAG, "----------------------");
}

void cjson_init()
{
    cJSON_Hooks cJSONhooks_freeRTOS = {0};
    cJSONhooks_freeRTOS.malloc_fn   = pvPortMalloc;
    cJSONhooks_freeRTOS.free_fn     = vPortFree;
    cJSON_InitHooks(&cJSONhooks_freeRTOS);
}

void ec800m_poweroff_and_mcu_restart(void)
{
#if MQTT_ENABLED || GNSS_ENABLED
    iot_disconnect();     // 断开连接
    ec800m_at_poweroff(); // 关闭整个4G模块
#endif
    mcu_restart();
}

void get_running_version(void)
{
    app_info_t app_info;
    read_app_current(&app_info);
    g_major_version = app_info.version_major;
    g_minor_version = app_info.version_minor;
    g_patch_version = app_info.version_patch;
}

error_t storage_init()
{
    error_t err = OK;
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
    storage_interface.storage_read_data_func  = (storage_read_data_func_t)eeprom_read;
    storage_interface.storage_write_data_func = (storage_write_data_func_t)eeprom_write;
    storage_interface.storage_init_func       = (storage_init_func_t)eeprom_check;
    err                                       = storage_install_interface(&storage_interface);
    if (err != OK) {
        LOGD(TAG, "storage init failed: %s", error_string(err));
    }
    return err;
}

void app_main(void)
{
    int8_t err = 0;
    // 初始化mcu
    mcu_init();
    // 初始化存储模块
    storage_init();
    // 初始化cjosn库
    cjson_init();
    // 初始化网关参数
    init_gateway_config();
    // 获取网关固件版本
    get_running_version();
    // 打印app info
    app_init();
    // 打印开机信息
    print_welcome_message();
    // 初始化设备
    ec800m_init();
    // 初始化网络
    at_hal_init();

#if UART_GATEWAY_CONFIG_ENABLED
    xTaskCreate(uart_gateway_config_task, UART_GATEWAY_CONFIG_TASK_NAME, UART_GATEWAY_CONFIG_TASK_DEPTH, NULL, 3, NULL);
#endif

#if MQTT_ENABLED
    // 初始化4G模块
    iot_connect();
    // xTaskCreate(iot_connect, "mqtt", 5120, NULL, 3, NULL);
#endif

#if GNSS_ENABLED
    gnss_init();
    // xTaskCreate(gnss_init, "gnss", 1024, NULL, 3, NULL);
#endif

#if HOST_ENABLED
    xTaskCreate(host_protocol_task, HOST_PROTOCOL_TASK_NAME, 512, NULL, 3, NULL);
#endif
    vTaskDelete(NULL);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
}
