#include "IIC.h"
#include "hg24c64.h"

#define assert(x) \
    if (!(x)) printf("Assertion failed: %s\n", #x)
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))

uint8_t Storage_Ready(void)
{
    iic_start();
    iic_send_byte(STORAGE_DEVICE);
    uint8_t ack = iic_receive_ack();
    iic_stop();
    return !ack;
}

void Storage_Write(uint16_t address, uint8_t data)
{
    while (!Storage_Ready())
        ;

    uint8_t res = 1;
    iic_start();
    iic_send_byte(STORAGE_DEVICE);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte((address >> 8) & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte(address & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    iic_send_byte(data);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_stop();
}
uint8_t Storage_Read(uint16_t address)
{
    while (!Storage_Ready())
        ;

    uint8_t res = 1;
    iic_start();
    iic_send_byte(STORAGE_DEVICE);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    iic_send_byte((address >> 8) & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte(address & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    iic_start();
    iic_send_byte(STORAGE_DEVICE | 0x01);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    uint8_t ret = iic_receive_byte();
    iic_send_ack(1);
    iic_stop();
    return ret;
}

void Storage_Write_Page(uint16_t addr, uint8_t *data, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (!Storage_Ready())
        ;
    uint8_t res = 1;
    iic_start();
    iic_send_byte(STORAGE_DEVICE);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte((addr >> 8) & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte(addr & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    for (uint8_t i = 0; i < len; ++i) {
        iic_send_byte(data[i]);
        res = iic_receive_ack();
        if (res) printf("Error in Storage_Write\n");
    }
    iic_stop();
}

void Storage_Read_Page(uint16_t addr, uint8_t *buffer, uint8_t len)
{
    if (len == 0 || len > 32) return;
    while (!Storage_Ready())
        ;

    uint8_t res = 1;
    iic_start();
    iic_send_byte(STORAGE_DEVICE);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Read");
    iic_send_byte((addr >> 8) & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");
    iic_send_byte(addr & 0xFF);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Write\n");

    iic_start();
    iic_send_byte(STORAGE_DEVICE | 0x01);
    res = iic_receive_ack();
    if (res) printf("Error in Storage_Read");

    for (uint8_t i = 0; i < len - 1; ++i) {
        buffer[i] = iic_receive_byte();
        iic_send_ack(0);
    }
    buffer[len - 1] = iic_receive_byte();
    iic_send_ack(1);
    iic_stop();
}

void Storage_Write_Buffer(uint16_t addr, uint8_t *buffer, uint16_t len)
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
}

void Storage_Read_Buffer(uint16_t addr, uint8_t *buffer, uint16_t len)
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
}
