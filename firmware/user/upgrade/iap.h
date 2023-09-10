
#ifndef __UPDATE_H
#define __UPDATE_H

#include "error_type.h"
#include "data/partition_info.h"
#include "utils/macros.h"

#ifdef STM32F407xx
#define FLASH_ADDRESS          (0x08000000)
#define FLASH_END_ADDRESS      (0x08100000) // 1024KB
#define APP_BOOTLOADER_ADDRESS (0x08000000) // 16KB
#define APP1_FLASH_ADDRESS     (0x08004000) // 496KB
#define APP2_FLASH_ADDRESS     (0x08080000) // 512KB
#define APP1_FLASH_END_ADDRESS (APP2_FLASH_ADDRESS)
#define APP2_FLASH_END_ADDRESS (FLASH_END_ADDRESS)
#endif

#ifdef STM32G0B0xx
#define FLASH_ADDRESS          (0x08000000)
#define FLASH_END_ADDRESS      (0x08080000) // 512KB
#define APP_BOOTLOADER_ADDRESS (0x08000000) // 12KB
#define APP1_FLASH_ADDRESS     (0x08004000) // 496KB
#define APP2_FLASH_ADDRESS     (0x08080000) // 512KB
#define APP1_FLASH_END_ADDRESS (APP2_FLASH_ADDRESS)
#define APP2_FLASH_END_ADDRESS (FLASH_END_ADDRESS)
#endif

error_t iap_update_partition(app_info_t *new);
error_t iap_init(uint8_t id);
error_t iap_deinit();
error_t iap_write(uint8_t *buffer, uint32_t length);
error_t iap_erase(uint8_t id);

#endif // !__UPDATE_H
