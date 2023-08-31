/*
 * @Date: 2023-08-12 16:30:18
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 16:40:47
 * @FilePath: \firmware\user\protocol\host\host_protocol.c
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "core/general_message.h"
#include "log/log.h"
#include "protocol/host/host_protocol.h"
#include "protocol/host/host_message_handle.h"

static const char *TAG = "HOST_PROTOCOL";

/* OTA升级标志，定义在main.c中 */
extern uint8_t g_app_upgrade_flag;

void host_protocol_task(void *pvParameters)
{
#if HOST_PROTOCOL_DEBUG_ENABLED
    LOGI(TAG, "host protocol task started");
#endif

    QueueHandle_t host_message_queue;
    // UBaseType_t space;
    host_message_queue = xQueueCreate(HOST_MESSAGE_QUEUE_SIZE, sizeof(general_message_t));
    if (host_message_queue == NULL) {
        LOGE(TAG, "create host message queue failed");
        vTaskDelete(NULL);
    }

    // 订阅GNSS消息
    subscribe_gnss_nmea_data(host_message_queue);

    general_message_t general_message;
    for (;;) {
        if (g_app_upgrade_flag) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (xQueueReceive(host_message_queue, (void *)&general_message, portMAX_DELAY) == pdPASS) {
            //  发送数据
            if (handle_host_message(&general_message) != 0) {
                LOGE(TAG, "print general message failed");
            }
            LOGD(TAG, "Remain message: %d", uxQueueMessagesWaiting(host_message_queue));
            LOGD(TAG, "Free Heap Size: %d", xPortGetFreeHeapSize());

            // LOGW(TAG, "general_message.type=%d", general_message.type);
            // LOGW(TAG, "general_message.data_len=%d", general_message.data_len);

            // 释放内存
            free_general_message(&general_message);
        }

        // space = uxQueueSpacesAvailable(host_message_queue);
        // LOGI(TAG, "host message queue space=%d/%d", space, HOST_MESSAGE_QUEUE_SIZE);
        // vTaskDelay(pdMS_TO_TICKS(1));
    }

    vTaskDelete(NULL);
}
