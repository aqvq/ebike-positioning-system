
#include "flash.h"
#include "log/log.h"
#include "error_type.h"

#define TAG "FLASH"

/**
 * @brief 擦除扇区，只能按sector擦除
 *
 * @param sector uint8_t数组，第一个元素代表擦除扇区起始编号，第二个元素代表擦除扇区数
 */
error_t flash_erase_sector(uint8_t *sector)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock(); // 解锁
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGSERR); // 清除一些错误标志
    error_t err = OK;
    uint32_t SectorError;

    EraseInitStruct.Banks        = FLASH_BANK_1;                       // 选择哪个Bank
    EraseInitStruct.Page       = sector[0];                          // 要擦除哪个扇区
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_PAGES;            // 只是擦除操作
    EraseInitStruct.NbPages    = sector[1];                          // 一次只擦除一个扇区
    if (HAL_OK != HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError)) { /* 擦除某一扇区 */
        err = FLASH_ERASE_ERROR;
        LOGE(TAG, "flash erase error\n");
    }
    HAL_FLASH_Lock(); // 上锁
    return err;
}

// 写数据
error_t flash_program_word(uint32_t StartAddress, uint32_t data)
{
    // 可以添加一些 StartAddress地址 是否有效的判断
    error_t err = OK;
    HAL_FLASH_Unlock(); // 解锁
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, StartAddress, data)) {
        err = FLASH_WRITE_ERROR;
        LOGE(TAG, "flash write error\n");
    }
    HAL_FLASH_Lock(); // 上锁
    return err;
}

error_t flash_program(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    FLASH_EraseInitTypeDef FlashEraseInit = {0};
    HAL_StatusTypeDef FlashStatus         = HAL_OK;
    uint32_t SectorError                  = 0;
    uint32_t end_addr                     = addr + length;
    error_t err                           = OK;

    HAL_FLASH_Unlock(); // 解锁
    if (FlashStatus == HAL_OK) {
        while (addr < end_addr) // 写数据
        {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, ((uint32_t *)buffer)[0]) != HAL_OK) // 写入数据
            {
                printf("flash write error\n");
                err = FLASH_WRITE_ERROR;
                break; // 写入异常
            }
            addr += 4;
            buffer += 4; // 写入下一个数据
        }
    }
    HAL_FLASH_Lock(); // 上锁
    return err;
}

uint8_t flash_read_byte(uint32_t address)
{
    return *(uint8_t *)address;
}

uint16_t flash_read_half_word(uint32_t address)
{
    return *(uint16_t *)address;
}

uint32_t flash_read_word(uint32_t address)
{
    return *(uint32_t *)address;
}

error_t flash_read(uint32_t addr, uint8_t *buffer, uint32_t length)
{
    uint32_t end_addr = addr + length;
    while (addr < end_addr) {
        ((uint32_t *)buffer)[0] = flash_read_word(addr); // 读取1个字节.
        addr += 4;
        buffer += 4;
    }
    return OK;
}
