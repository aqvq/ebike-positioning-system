/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:44:38
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
#include "data/partition_info.h"

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
    write_address = flash_get_alternate_bank_address();
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
        LOGE(TAG, "IAP Not Init");
        return ERROR_IAP_NOT_INIT;
    }
    write_address = flash_write(write_address, buffer, length);
    return err;
}

error_t iap_erase(uint8_t id)
{
    return flash_erase_alternate_bank();
}
