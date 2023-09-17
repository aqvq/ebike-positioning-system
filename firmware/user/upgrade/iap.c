
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "common/error_type.h"
#include "bsp/flash/flash.h"
#include "log/log.h"
#include "storage/storage.h"
#include "iap.h"

#define TAG "UPGRADE"
static uint32_t write_address = 0;

error_t iap_init()
{
    // write_address = ((boot_get_current_bank() == 0) ? APP2_FLASH_ADDRESS : APP1_FLASH_ADDRESS);
    write_address = APP2_FLASH_ADDRESS;
    return iap_erase();
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
        LOGE(TAG, "IAP not init or flash write failed");
        return ERROR_IAP_NOT_INIT;
    }
    write_address = flash_write(write_address, buffer, length);
    return err;
}

error_t iap_erase()
{
    return flash_erase_alternate_bank();
}
