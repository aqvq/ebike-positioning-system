/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:56:17
 * @FilePath: \firmware\user\protocol\iot\iot_helper.c
 * @Description: iot协议实现
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "log/log.h"
#include "aliyun/aliyun_protocol.h"
#include "iot_helper.h"
#include "iot_interface.h"
#include "storage/storage.h"
#include "gnss/gnss.h"

#define TAG "IOT_HELPER"

//----------------------局部变量--------------------------------------------

// 定义iot接口实例
static iot_interface_t iot_interface =
    {
        .iot_connect    = aliyun_iot_connect,
        .iot_disconnect = aliyun_iot_disconnect,
        .iot_send       = aliyun_iot_send,
        .iot_post_log   = aliyun_iot_post_log};
// iot连接标志变量
static uint8_t is_connect_iot = 0;

//----------------------全局变量--------------------------------------------

/* OTA升级标志，定义在main.c中 */
extern uint8_t g_app_upgrade_flag;

/// @brief 处理从iot收到的数据
/// @param data 数据
/// @return int32_t
static int32_t received_from_iot(void *data)
{
    return 0;
}

/// @brief 向iot发送数据任务
/// @param pvParameters 可忽略
void iot_send_task(void *pvParameters)
{
    // 创建接收数据的消息队列
    QueueHandle_t message_queue;
    message_queue = xQueueCreate(100, sizeof(general_message_t *));
    if (message_queue == NULL) {
        LOGE(TAG, "create iot message queue failed");
        vTaskDelete(NULL);
    }
    uint8_t max_num = 0;

    // 订阅GNSS消息
    subscribe_gnss_data(message_queue);

    general_message_t *general_message;
    while (is_connect_iot) {
        if (g_app_upgrade_flag) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }
        uint8_t now_num = uxQueueMessagesWaiting(message_queue);
        if (now_num > max_num) {
            max_num = now_num;
            LOGI(TAG, "max_num=%d", max_num);
        }
        if (xQueueReceive(message_queue, (void *)&general_message, portMAX_DELAY) == pdPASS) {
            // 发送数据
            if (iot_interface.iot_send(general_message) < 0) {
                LOGE(TAG, "iot send failed, type=%d.", general_message->type);
            }

            // 释放内存
            free_general_message(general_message);
        }
    }

    vTaskDelete(NULL);
}

/// @brief 连接iot
int32_t iot_connect(void)
{
    // while (at_handle.is_init != 1)
    //     vTaskDelay(pdMS_TO_TICKS(100));
    int32_t res = iot_interface.iot_connect(received_from_iot);
    if (res >= 0) {
        is_connect_iot = 1;

        // 创建发送数据任务
        int8_t err = xTaskCreate(iot_send_task, "iot_send_task", 1024, NULL, 3, NULL);
        if (err != pdPASS) {
            LOGE(TAG, "creare iot_send_task failed");
            LOGE(TAG, "Free Heap Size: %d", xPortGetMinimumEverFreeHeapSize());
            LOGE(TAG, "error code: %d", err);
            res = -0x100;
        }
    }
    // return res;
    vTaskDelete(NULL);
}

/// @brief 与iot断开连接
int32_t iot_disconnect(void)
{
    is_connect_iot = 0;
    return iot_interface.iot_disconnect();
}

/// @brief 向Iot平台发送日志
/// @param level 日志级别
/// @param module_name 模块名称
/// @param code 状态码
/// @param content 日志内容
/// @return 0正常 其他异常
int32_t iot_post_log(iot_log_level_t level, char *module_name, int32_t code, char *content)
{
    if (is_connect_iot) {
        return iot_interface.iot_post_log(level, module_name, code, content);
    }

    return -1;
}
