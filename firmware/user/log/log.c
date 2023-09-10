
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "main.h"
#include "semphr.h"
#include "log/log.h"

#define TAG             "PRINT"

#define LOG_BUF_SIZE    128
#define LOG_STREAM_SIZE 1024

StreamBufferHandle_t g_print_buffer;
SemaphoreHandle_t g_print_mutex;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart3_tx;
void task_print(void *p)
{
    g_print_buffer                   = xStreamBufferCreate(LOG_STREAM_SIZE, 1);
    g_print_mutex                    = xSemaphoreCreateMutex();
    char (*buffer)[LOG_BUF_SIZE / 2] = pvPortMalloc(LOG_BUF_SIZE);
    uint8_t index                    = 0;
    if (g_print_buffer == NULL || buffer == NULL || g_print_mutex == NULL) {
        LOGE(TAG, "task print error");
        Error_Handler();
    }

    while (1) {
        while (huart2.gState != HAL_UART_STATE_READY) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        xSemaphoreTake(g_print_mutex, portMAX_DELAY);
        if (xStreamBufferReceive(g_print_buffer, (void *)buffer, LOG_BUF_SIZE, 0) == pdTRUE) {
            HAL_UART_Transmit_DMA(&huart2, (uint8_t *)buffer[index], strlen(buffer));
            index = !index;
        }
        xSemaphoreGive(g_print_mutex);
    }
    vPortFree(buffer);
    vTaskDelete(NULL);
}

// 实现格式化打印功能
void print(const char *format, ...)
{
    // va_list args;
    // va_start(args, format);
    // vsnprintf(log_buf, LOG_BUF_SIZE, format, args);
    // va_end(args);
    // xStreamBufferSend(g_print_buffer, (void *)log_buf, strlen(log_buf), 0);

}
