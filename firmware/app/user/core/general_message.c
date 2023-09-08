/*
 * @Date: 2023-08-31 15:54:24
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:07:10
 * @FilePath: \firmware\user\core\general_message.c
 */
#include "general_message.h"
#include <string.h>
#include <stdlib.h>
#include "main.h"

/// @brief 释放general_message_t对象
/// @param msg
int8_t free_general_message(general_message_t *msg)
{
    if (msg == NULL)
        return -1;

    if (msg->data == NULL)
        return -2;

    vPortFree(msg->data);
    return 0;
}

int8_t remove_queue_first_item(QueueHandle_t xQueue)
{
    if (xQueue == NULL)
        return -1;

    UBaseType_t uxNumberOfFreeSpaces; // 当前订阅的队列剩余空间
    uxNumberOfFreeSpaces = uxQueueSpacesAvailable(xQueue);

    // 若队列中没有剩余空间，先主动读取之前放入队列的数据并释放。
    if (uxNumberOfFreeSpaces < 5) {
        general_message_t general_message;
        if (xQueueReceive(xQueue, (void *)&general_message, portMAX_DELAY) == pdPASS) {
            // 释放内存
            free_general_message(&general_message);
            return 0;
        }

        return -2;
    }

    return 0;
}
