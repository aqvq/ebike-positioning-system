
#ifndef __FLASH_H
#define __FLASH_H

#include <stdint.h>
#include "error_type.h"

error_t flash_erase_sector(uint8_t *sector);
error_t flash_program_word(uint32_t StartAddress, uint32_t data);
error_t flash_program(uint32_t addr, uint8_t *buffer, uint32_t length);
uint8_t flash_read_byte(uint32_t address);
uint16_t flash_read_half_word(uint32_t address);
uint32_t flash_read_word(uint32_t address);
error_t flash_read(uint32_t addr, uint8_t *buffer, uint32_t length);

#endif // !__FLASH_H