/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:44:38
 * @FilePath: \firmware\user\partition\iap.c
 */
/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:08:27
 * @FilePath: \firmware\user\partition\iap.c
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "error_type.h"
#include "bsp/flash/flash.h"
#include "log/log.h"
#include "storage/storage.h"
#include "iap.h"
#include "partition_info.h"

#define TAG                "UPDATE"
#define UPDATE_BUFFER_SIZE 1024
#define UPDATE_TIMEOUT     3000

uint8_t g_update_flag           = 0;
static uint32_t write_address   = 0;
static uint8_t sector_map[2][2] = {
    // 起始扇区，扇区数量
    {2, 6},
    {8, 4},
};

error_t iap_update_partition(app_info_t *new)
{
    error_t err    = OK;
    app_info_t cur = {0};
    err |= read_app_current(&cur);
    err |= write_app_previous(&cur);
    err |= write_app_current(new);
    return err;
}

error_t iap_init(uint8_t id)
{
    if (id == 1) {
        write_address = APP1_FLASH_ADDRESS;
    } else if (id == 2) {
        write_address = APP2_FLASH_ADDRESS;
    } else {
        return ERROR_INVALID_PARAMETER;
    }
    return iap_erase(id);
}

error_t iap_deinit()
{
    write_address = 0;
    return OK;
}

error_t iap_write(uint8_t *buffer, uint32_t length)
{
    error_t err = OK;
    if (write_address == 0) {
        LOGE(TAG, "Unexpected error");
        return ERROR;
    }
    err = flash_program(write_address, buffer, length);
    write_address += length;
    return err;
}

error_t iap_erase(uint8_t id)
{
    if (id != 1 && id != 2) {
        return ERROR_INVALID_PARAMETER;
    }
    return flash_erase_sector(sector_map[id - 1]);
}

#if 0
error_t iap_task(void *p)
{
    /* Ymodem更新 */
    Ymodem_Receive(UartRecBuf, AppAddr);

    partition_info_t config={0};
    read_partition_info(&config);
    swap_app_info(&config);
    config.app_current.version_major;
    config.app_current.version_minor;
    config.app_current.version_patch;
    config.enabled = APP_INFO_ENABLED_FLAG;
    config.

    // 重启

}

void update_task(void *p)
{
    if (g_update_flag != 0) {
        LOGE(TAG, "unexpected error");
        vTaskDelete(NULL);
    }
    g_update_flag = 1; // 收到更新信号
    LOGI(TAG, "update task ready to receive");
    uint8_t *buffer = pvPortMalloc(sizeof(uint8_t) * UPDATE_BUFFER_SIZE);
    extern StreamBufferHandle_t uart_stream_buffer;
    uint32_t size = 0;
    if (!uart_stream_buffer) {
        LOGE(TAG, "uart_stream_buffer is null");
        g_update_flag = 0;
        vTaskDelete(NULL);
    }

    app_info_t cur = {0};
    storage_read(app_current, &cur);

    uint32_t write_addr = (cur.addr == 0x08008000 ? 0x08080000 : 0x08008000);
    extern uint8_t sector_map[2][2];

    flash_erase_sector(cur.addr == 0x08008000 ? sector_map[1] : sector_map[0]);
    xStreamBufferReset(uart_stream_buffer);
    while (1) {
        uint32_t ret = xStreamBufferReceive(uart_stream_buffer, buffer, UPDATE_BUFFER_SIZE, pdMS_TO_TICKS(UPDATE_TIMEOUT));
        if (ret == 0) {
            if (g_update_flag == 1) {
                LOGW(TAG, "please send file");
                continue;
            } else if (g_update_flag == 2) {
                LOGI(TAG, "receive file complete");
                break;
            }
        } else {
            if (g_update_flag == 1) {
                g_update_flag = 2; // 开始传输
            }
        }
        size += ret;
        flash_write(write_addr, buffer, UPDATE_BUFFER_SIZE);
        memset(buffer, 0, UPDATE_BUFFER_SIZE);
        LOGI(TAG, "write addr: %x", write_addr);
        write_addr += UPDATE_BUFFER_SIZE;
    }

    storage_write(app_previous, &cur);
    cur.id            = cur.id == 1 ? 2 : 1;
    cur.addr          = (cur.addr == 0x08008000 ? 0x08080000 : 0x08008000);
    cur.version_major = 1;
    cur.version_minor = 1;
    cur.version_patch = 1;
    cur.size          = size;
    strcpy(cur.note, "update app");
    cur.timestamp = HAL_GetTick();
    storage_write(app_current, &cur);
    g_update_flag = 0;
    LOGI(TAG, "update complete");
    LOGI(TAG, "restart");
    stm32_restart();

    vTaskDelete(NULL);
}

error_t update_init(void)
{
    if (pdTRUE != xTaskCreate(update_task, "update_task", 1024, NULL, 3, NULL)) {
        LOGE(TAG, "create task update error");
        return ERROR_CREATE_TASK;
    }
    return OK;
}

#endif