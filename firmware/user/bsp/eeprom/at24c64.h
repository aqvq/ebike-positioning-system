/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:03:42
 * @FilePath: \firmware\user\bsp\eeprom\at24c64.h
 * @Description: at24c64驱动代码头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef INC_STORAGE_H_
#define INC_STORAGE_H_

#include "common/error_type.h"

// IIC设备地址
#define STORAGE_DEVICE  (0xA0)

/// @brief eeprom检查设备是否工作正常
/// @param  
/// @return OK or EEPROM_CHECK_ERROR
error_t eeprom_check(void);

/// @brief 将任意大小缓冲区写入eeprom
/// @param addr 写入eeprom的地址
/// @param buffer 待写入的数据
/// @param len 待写入的数据大小
/// @return 状态码
error_t eeprom_write(uint16_t addr, uint8_t *buffer, uint16_t len);

/// @brief 从eeprom读入任意大小内容到缓冲区
/// @param addr 待读取的eeprom地址
/// @param buffer 读取数据存放位置
/// @param len 读取数据的大小
/// @return 状态码
error_t eeprom_read(uint16_t addr, uint8_t *buffer, uint16_t len);

#endif /* INC_STORAGE_H_ */
