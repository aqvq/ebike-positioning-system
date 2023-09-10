
#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include "error_type.h"

#define STORAGE_DEVICE  (0xA0)

error_t eeprom_check(void);
error_t eeprom_write(uint16_t addr, uint8_t *buffer, uint16_t len);
error_t eeprom_read(uint16_t addr, uint8_t *buffer, uint16_t len);

#endif /* INC_STORAGE_H_ */
