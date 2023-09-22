/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:56:16
 * @FilePath: \firmware\user\bsp\flash\flash.c
 * @Description: stm32 flash驱动代码
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#include "flash.h"
#include "log/log.h"
#include "common/error_type.h"
#include "boot.h"
#include <string.h>

#define TAG "FLASH"

uint32_t flash_read(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    memcpy(buffer, (void *)addr, length);
    return (addr + length);
}

// flash获取空闲bank
static uint8_t flash_get_alternate_bank(void)
{
    return (boot_get_current_bank() == 0 ? 1 : 0);
}

error_t flash_erase_alternate_bank(void)
{
    FLASH_EraseInitTypeDef flash_erase_init;
    uint32_t PageError         = 0;
    flash_erase_init.Banks     = (flash_get_alternate_bank() == 0 ? FLASH_BANK_1 : FLASH_BANK_2);
    flash_erase_init.TypeErase = FLASH_TYPEERASE_MASS;
    flash_erase_init.Page      = 0;
    flash_erase_init.NbPages   = FLASH_PAGE_NB;
    // 擦除数据前要先解锁
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef ret = HAL_FLASHEx_Erase(&flash_erase_init, &PageError);
    HAL_FLASH_Lock();
    if (ret != HAL_OK) {
        return FLASH_ERASE_ERROR;
    }
    return OK;
}

uint32_t flash_get_alternate_bank_address(void)
{
    if (flash_get_alternate_bank() == 0) {
        return 0x08000000;
    } else if (flash_get_alternate_bank() == 1) {
        return 0x08040000;
    } else {
        LOGE(TAG, "Unexpected Error");
        Error_Handler();
    }
}

// buffer points to a double-words data
// which means the length of buffer 8
static error_t flash_write_dword(uint32_t addr, uint8_t *buffer)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, *(uint64_t *)buffer)) {
        return FLASH_WRITE_ERROR;
    }
    return OK;
}

// buffer points to a 64-words data
// which means the length of buffer 256
static error_t flash_write_fast(uint32_t addr, uint8_t *buffer)
{
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_FAST, addr, (uint64_t)buffer)) {
        return FLASH_WRITE_ERROR;
    }
    return OK;
}

uint32_t flash_write(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    error_t err = OK;
    // 写入数据前要先解锁
    HAL_FLASH_Unlock(); // 解锁

    while (length > 0) {
        if (length >= 256) {
            err = flash_write_fast(addr, buffer);
            buffer += 256;
            addr += 256;
            length -= 256;
        } else if (length >= 8) {
            err = flash_write_dword(addr, buffer);
            buffer += 8;
            addr += 8;
            length -= 8;
        } else {
            LOGW(TAG, "Flash write address is not aligned with 64 bytes.");
            uint8_t data[8] = {0};
            memcpy(data, buffer, length);
            err = flash_write_dword(addr, data);
            buffer += length;
            addr += length;
            length -= length;
        }
        if (err != OK) {
            addr = 0;
            break;
        }
    }

    HAL_FLASH_Lock(); // 加锁
    return addr;
}
