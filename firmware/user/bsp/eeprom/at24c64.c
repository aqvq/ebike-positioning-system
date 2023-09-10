#include "at24c64.h"
#include "storage_iic.h"
#include "error_type.h"
#include "utils/macros.h"
#include "main.h"
#define TAG "AT24C64"

error_t eeprom_check(void)
{
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    uint8_t ack = storage_iic_receive_ack();
    storage_iic_stop();
    return ((ack == 0) ? OK : EEPROM_CHECK_ERROR); // normally should be OK
}

static void bsp_eeprom_write(uint16_t address, uint8_t data)
{
    while (eeprom_check() != OK)
        ;

    uint8_t res = 1;
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte((address >> 8) & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte(address & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    storage_iic_send_byte(data);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_stop();
}
static uint8_t bsp_eeprom_read(uint16_t address)
{
    while (eeprom_check() != OK)
        ;

    uint8_t res = 1;
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    storage_iic_send_byte((address >> 8) & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte(address & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE | 0x01);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    uint8_t ret = storage_iic_receive_byte();
    storage_iic_send_ack(1);
    storage_iic_stop();
    return ret;
}

static void bsp_eeprom_write_page(uint16_t addr, uint8_t *data, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (eeprom_check() != OK)
        ;
    uint8_t res = 1;
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte((addr >> 8) & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte(addr & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    for (uint8_t i = 0; i < len; ++i) {
        storage_iic_send_byte(data[i]);
        res = storage_iic_receive_ack();
        if (res) printf("Error in Storage_Write\n");
    }
    storage_iic_stop();
}

static void bsp_eeprom_read_page(uint16_t addr, uint8_t *buffer, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (eeprom_check() != OK)
        ;

    uint8_t res = 1;
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    storage_iic_send_byte((addr >> 8) & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    storage_iic_send_byte(addr & 0xFF);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE | 0x01);
    res = storage_iic_receive_ack();
    if (res) printf("Error in Storage_Read");

    for (uint8_t i = 0; i < len - 1; ++i) {
        buffer[i] = storage_iic_receive_byte();
        storage_iic_send_ack(0);
    }
    buffer[len - 1] = storage_iic_receive_byte();
    storage_iic_send_ack(1);
    storage_iic_stop();
}

error_t eeprom_write(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t end_addr  = addr + len;
    uint16_t next_addr = roundup(addr, 32);
    assert(next_addr % 32 == 0);
    if (next_addr < end_addr) {
        bsp_eeprom_write_page(addr, buffer, next_addr - addr);
        buffer += (next_addr - addr);
        addr = next_addr;
        while (addr < end_addr) {
            bsp_eeprom_write_page(addr, buffer, ((end_addr - addr > 32) ? 32 : (end_addr - addr)));
            addr += 32;
            buffer += 32;
        }
    } else {
        assert(len <= 32);
        bsp_eeprom_write_page(addr, buffer, len);
    }
    return OK;
}

error_t eeprom_read(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t end_addr  = addr + len;
    uint16_t next_addr = roundup(addr, 32);
    assert(next_addr % 32 == 0);
    if (next_addr < end_addr) {
        bsp_eeprom_read_page(addr, buffer, next_addr - addr);
        buffer += (next_addr - addr);
        addr = next_addr;
        while (addr < end_addr) {
            bsp_eeprom_read_page(addr, buffer, ((end_addr - addr > 32) ? 32 : (end_addr - addr)));
            addr += 32;
            buffer += 32;
        }
    } else {
        assert(len <= 32);
        bsp_eeprom_read_page(addr, buffer, len);
    }
    return OK;
}
