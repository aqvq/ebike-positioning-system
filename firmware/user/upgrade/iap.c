/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:16:26
 * @FilePath: \firmware\user\upgrade\iap.c
 * @Description: IAP功能实现
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "common/error_type.h"
#include "bsp/flash/flash.h"
#include "log/log.h"
#include "storage/storage.h"
#include "iap.h"

#define TAG "UPGRADE"
static uint32_t write_address = 0;

error_t iap_init()
{
    // write_address = ((boot_get_current_bank() == 0) ? APP2_FLASH_ADDRESS : APP1_FLASH_ADDRESS);

    // STM32G0B0RET6支持Dual Bank，将使用的Bank映射到0x0800 0000将空闲Bank映射到0x0804 0000
    // 故每次写入地址固定为0x0804 0000
    write_address = APP2_FLASH_ADDRESS;
    // 初始化需要先擦除芯片
    return iap_erase();
}

error_t iap_deinit()
{
    // 重置写入地址
    write_address = 0;
    return OK;
}

error_t iap_write(uint8_t *buffer, uint32_t length)
{
    error_t err = OK;
    // 判断是否初始化iap
    if (write_address == 0) {
        LOGE(TAG, "IAP not init or flash write failed");
        return ERROR_IAP_NOT_INIT;
    }
    // 写入flash
    write_address = flash_write(write_address, buffer, length);
    return err;
}

error_t iap_erase()
{
    // 擦除空闲bank
    return flash_erase_alternate_bank();
}
