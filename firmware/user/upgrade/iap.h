/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:08:35
 * @FilePath: \firmware\user\upgrade\iap.h
 * @Description: IAP功能实现头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef __UPDATE_H
#define __UPDATE_H

#include "common/error_type.h"
#include "utils/macros.h"

// STM32F407xx FLASH分区表信息定义
#ifdef STM32F407xx
#define FLASH_ADDRESS          (0x08000000)
#define FLASH_END_ADDRESS      (0x08100000) // 1024KB
#define APP_BOOTLOADER_ADDRESS (0x08000000) // 16KB
#define APP1_FLASH_ADDRESS     (0x08004000) // 496KB
#define APP2_FLASH_ADDRESS     (0x08080000) // 512KB
#define APP1_FLASH_END_ADDRESS (APP2_FLASH_ADDRESS)
#define APP2_FLASH_END_ADDRESS (FLASH_END_ADDRESS)
#endif

// STM32G0B0xx FLASH分区表信息定义
#ifdef STM32G0B0xx
#define FLASH_ADDRESS          (0x08000000)
#define FLASH_END_ADDRESS      (0x08080000) // 512KB
#define APP1_FLASH_ADDRESS     (0x08000000) // 256KB
#define APP2_FLASH_ADDRESS     (0x08040000) // 256KB
#define APP1_FLASH_END_ADDRESS (APP2_FLASH_ADDRESS)
#define APP2_FLASH_END_ADDRESS (FLASH_END_ADDRESS)
#endif

/// @brief IAP初始化，将内部记忆的写入位置置位空闲BANK起始地址
error_t iap_init();

/// @brief IAP析构，将内部记忆的写入位置重置
error_t iap_deinit();

/// @brief IAP写入，内部记忆上次写入位置，无需传入写入flash地址
/// @param buffer 待写入的数据缓冲区
/// @param length 待写入的数据大小
error_t iap_write(uint8_t *buffer, uint32_t length);

/// @brief IAP擦除，擦除空闲BANK
error_t iap_erase();

#endif // !__UPDATE_H
