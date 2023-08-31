
#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include "error_type.h"

#define STORAGE_DEVICE (0xA0)

uint8_t Storage_Ready(void);
void Storage_Write(uint16_t address, uint8_t data);
uint8_t Storage_Read(uint16_t address);
void Storage_Write_Page(uint16_t addr, uint8_t* data, uint8_t len);
void Storage_Read_Page(uint16_t addr, uint8_t* buffer, uint8_t len);
error_t Storage_Write_Buffer(uint16_t addr, uint8_t* buffer, uint16_t len);
error_t Storage_Read_Buffer(uint16_t addr, uint8_t* buffer, uint16_t len);

#endif /* INC_STORAGE_H_ */
