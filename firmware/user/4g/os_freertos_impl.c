#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "os_net_interface.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/**
 * @brief 申请内存
 */
static void *__malloc(uint32_t size)
{
    return pvPortMalloc(size);
}
/**
 * @brief 释放内存
 */
void __free(void *ptr)
{
    vPortFree(ptr);
}
/**
 * @brief 获取当前的时间戳，SDK用于差值计算
 */
uint64_t __time(void)
{
    return (uint64_t)(xTaskGetTickCount() * portTICK_RATE_MS);
}
/**
 * @brief 睡眠指定的毫秒数
 */
void __sleep(uint64_t time_ms)
{
    vTaskDelay(pdMS_TO_TICKS(time_ms));
}
/**
 * @brief 随机数生成方法
 */
void __rand(uint8_t *output, uint32_t output_len)
{
    uint32_t idx = 0, bytes = 0, rand_num = 0;

    srand(__time());
    for (idx = 0; idx < output_len;)
    {
        if (output_len - idx < 4)
        {
            bytes = output_len - idx;
        }
        else
        {
            bytes = 4;
        }
        rand_num = rand();
        while (bytes-- > 0)
        {
            output[idx++] = (uint8_t)(rand_num >> bytes * 8);
        }
    }
}
/**
 * @brief 创建互斥锁
 */
void *__mutex_init(void)
{
    return (void *)xSemaphoreCreateMutex();
}
/**
 * @brief 申请互斥锁
 */
void __mutex_lock(void *mutex)
{
    xSemaphoreTake((SemaphoreHandle_t)mutex, portMAX_DELAY);
}
/**
 * @brief 释放互斥锁
 */
void __mutex_unlock(void *mutex)
{
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}
/**
 * @brief 销毁互斥锁
 */
void __mutex_deinit(void **mutex)
{
    if (mutex == NULL || *mutex == NULL)
    {
        return;
    }
    vSemaphoreDelete((SemaphoreHandle_t)*mutex);
    *mutex = NULL;
}

aiot_os_al_t g_aiot_freertos_api = {
    .malloc = __malloc,
    .free = __free,
    .time = __time,
    .sleep = __sleep,
    .rand = __rand,
    .mutex_init = __mutex_init,
    .mutex_lock = __mutex_lock,
    .mutex_unlock = __mutex_unlock,
    .mutex_deinit = __mutex_deinit,
};
