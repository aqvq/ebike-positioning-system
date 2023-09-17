
#ifndef __FLASH_H
#define __FLASH_H

#include <stdint.h>
#include "common/error_type.h"

uint32_t flash_get_alternate_bank_address(void);
error_t flash_erase_alternate_bank(void);
uint32_t flash_read(uint32_t addr, uint8_t *buffer, uint32_t length);
uint32_t flash_write(uint32_t addr, uint8_t *buffer, uint32_t length);

#endif // !__FLASH_H
