#include <stdio.h>
#include <string.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "log/log.h"
#include "protocol/host/host_protocol.h"
#include "cJSON.h"
#include "storage/storage.h"
#include "protocol/iot/iot_helper.h"
#include "aiot_at_api.h"
#include "bsp/at/at.h"
#include "app_main.h"
#include "gnss/gnss.h"
#include "utils/util.h"
#include "common/error_type.h"
#include "bsp/eeprom/at24c64.h"
#include "bsp/mcu/mcu.h"
#include "bsp/flash/boot.h"
#include "upgrade/iap.h"
#include "bsp/flash/flash.h"
#include "bsp/mcu/mcu.h"
#include "common/config.h"
#include "protocol/uart/uart_config.h"

#define TAG "MAIN"

//--------------------------------------全局变量---------------------------------------------

volatile uint32_t ulHighFrequencyTimerTicks;
char g_ec800m_apn_cmd[256];
uint8_t g_app_upgrade_flag = 0;

void print_welcome_message(void)
{
    LOG("*************************************************************");
    LOG("*                                                           *");
    LOG("*              THIS IS E-Bike Positioning Device            *");
    LOG("*                SOFTWARE VERSION: %d.%d.%d                   *", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_BUILD);
    LOG("*                                                           *");
#if HOST_ENABLED
    LOG("*                        HOST_ENABLED                       *");
#endif
#if MQTT_ENABLED
    LOG("*                        MQTT_ENABLED                       *");
#endif
#if UART_CONFIG_ENABLED
    LOG("*                 UART_CONFIG_ENABLED               *");
#endif
    LOG("*                  Created By Shang Wentao                  *");
    LOG("*                                                           *");
    LOG("*************************************************************");
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

void app_main(void *p)
{
    int8_t err = 0;
    // 初始化mcu
    mcu_init();
    // 初始化存储模块
    storage_init();
    // 初始化cjosn库
    cjson_init();
    // 打印开机信息
    print_welcome_message();
    // 初始化设备
    ec800m_init();

#if UART_CONFIG_ENABLED
    xTaskCreate(uart_config_task, UART_CONFIG_TASK_NAME, UART_CONFIG_TASK_DEPTH, NULL, 3, NULL);
#endif

#if HOST_ENABLED
    xTaskCreate(host_protocol_task, HOST_PROTOCOL_TASK_NAME, 512, NULL, 3, NULL);
#endif

#if MQTT_ENABLED || GNSS_ENABLED
    /* 硬件AT模组初始化 */
    int32_t res = at_hal_init();
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aliyun protocol at_hal_init failed, restart");
        ec800m_poweroff_and_mcu_restart();
    }
    LOGD(TAG, "at hal init success");
#endif

#if MQTT_ENABLED
    // 初始化4G模块
    // iot_connect();
    xTaskCreate((void (*)(void *))iot_connect, "conn", 2048, NULL, 3, NULL);
#endif

#if GNSS_ENABLED
    // gnss_init();
    xTaskCreate((void (*)(void *))gnss_init, "gnss", 1024, NULL, 3, NULL);
#endif

#if defined(DEBUG)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3000));
        LOGD(TAG, "Free heap size: %d", xPortGetFreeHeapSize());
    }
#else
    vTaskDelete(NULL);
#endif
}

void vApplicationStackOverflowHook(TaskHandle_t xTask,
                                   char *pcTaskName)
{
    LOGE(TAG, "Stack Overflow: %s", pcTaskName);
    Error_Handler();
}
