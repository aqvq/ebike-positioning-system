
#include "storage_iic.h"
#include "hg24c64.h"
#include "error_type.h"
#include "utils/macros.h"
#define TAG "HG24C64"

uint8_t Storage_Ready(void)
{
    storage_iic_start();
    storage_iic_send_byte(STORAGE_DEVICE);
    uint8_t ack = storage_iic_receive_ack();
    storage_iic_stop();
    return ack;
}

void Storage_Write(uint16_t address, uint8_t data)
{
    while (Storage_Ready())
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
uint8_t Storage_Read(uint16_t address)
{
    while (Storage_Ready())
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

void Storage_Write_Page(uint16_t addr, uint8_t *data, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (Storage_Ready())
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

void Storage_Read_Page(uint16_t addr, uint8_t *buffer, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (Storage_Ready())
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

#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))

error_t Storage_Write_Buffer(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t end_addr  = addr + len;
    uint16_t next_addr = roundup(addr, 32);
    assert(next_addr % 32 == 0);
    if (next_addr < end_addr) {
        Storage_Write_Page(addr, buffer, next_addr - addr);
        buffer += (next_addr - addr);
        addr = next_addr;
        while (addr < end_addr) {
            Storage_Write_Page(addr, buffer, ((end_addr - addr > 32) ? 32 : (end_addr - addr)));
            addr += 32;
            buffer += 32;
        }
    } else {
        assert(len <= 32);
        Storage_Write_Page(addr, buffer, len);
    }
    return OK;
}

error_t Storage_Read_Buffer(uint16_t addr, uint8_t *buffer, uint16_t len)
{
    uint16_t end_addr  = addr + len;
    uint16_t next_addr = roundup(addr, 32);
    assert(next_addr % 32 == 0);
    if (next_addr < end_addr) {
        Storage_Read_Page(addr, buffer, next_addr - addr);
        buffer += (next_addr - addr);
        addr = next_addr;
        while (addr < end_addr) {
            Storage_Read_Page(addr, buffer, ((end_addr - addr > 32) ? 32 : (end_addr - addr)));
            addr += 32;
            buffer += 32;
        }
    } else {
        assert(len <= 32);
        Storage_Read_Page(addr, buffer, len);
    }
    return OK;
}
