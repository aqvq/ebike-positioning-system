/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 16:01:47
 * @FilePath: \firmware\user\bsp\flash\flash.h
 * @Description: flash驱动代码头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef __FLASH_H
#define __FLASH_H

#include <stdint.h>
#include "common/error_type.h"

/// @brief 获取空闲bank的起始地址
/// @return flash地址
uint32_t flash_get_alternate_bank_address(void);

/// @brief 擦除空闲bank内容
/// @return 状态码 OK / FLASH_ERASE_ERROR
error_t flash_erase_alternate_bank(void);

/// @brief flash读缓冲区
/// @param addr flash地址
/// @param buffer 读取数据存放地址
/// @param length 读取数据大小
/// @return flash读取的最后一个字节数据的下一个地址
uint32_t flash_read(uint32_t addr, uint8_t *buffer, uint32_t length);

/// @brief flash写缓冲区
/// @param addr flash地址
/// @param buffer 写入数据缓冲区地址
/// @param length 写入数据大小
/// @return flash写入的最后一个字节数据的下一个地址
uint32_t flash_write(uint32_t addr, uint8_t *buffer, uint32_t length);

#endif // !__FLASH_H
