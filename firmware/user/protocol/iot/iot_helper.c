#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "log/log.h"
#include "aliyun/aliyun_protocol.h"
#include "iot_helper.h"
#include "iot_interface.h"
#include "storage/storage.h"
#include "gnss/gnss.h"

//----------------------局部变量--------------------------------------------
static iot_interface_t iot_interface =
    {
        .iot_connect    = aliyun_iot_connect,
        .iot_disconnect = aliyun_iot_disconnect,
        .iot_send       = aliyun_iot_send,
        .iot_post_log   = aliyun_iot_post_log};

static uint8_t is_connect_iot = 0;
static const char *TAG        = "IOT_HELPER";

//----------------------全局变量--------------------------------------------

/* OTA升级标志，定义在main.c中 */
extern uint8_t g_app_upgrade_flag;

static int32_t received_from_iot(void *data)
{
    return 0;
}

void iot_send_task(void *pvParameters)
{
    QueueHandle_t message_queue;
    message_queue = xQueueCreate(100, sizeof(general_message_t *));
    if (message_queue == NULL) {
        LOGE(TAG, "create iot message queue failed");
        vTaskDelete(NULL);
    }
    uint8_t max_num = 0;

    // 订阅GNSS消息
    subscribe_gnss_nmea_data(message_queue);

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

int32_t iot_connect(void)
{
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

int32_t iot_disconnect(void)
{
    is_connect_iot = 0;
    return iot_interface.iot_disconnect();
}

int32_t iot_post_log(iot_log_level_t level, char *module_name, int32_t code, char *content)
{
    if (is_connect_iot) {
        return iot_interface.iot_post_log(level, module_name, code, content);
    }

    return -1;
}
