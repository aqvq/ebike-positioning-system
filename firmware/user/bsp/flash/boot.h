/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:39:31
 * @FilePath: \firmware\user\bsp\flash\boot.h
 * @Description: 针对stm32 option byte读写的驱动代码
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#ifndef __BOOT_OB_H
#define __BOOT_OB_H

#include "common/error_type.h"

/// @brief 设置从bootloader启动，立即生效
/// @return error_t
error_t boot_configure_boot_from_bootloader(void);

/// @brief 设置从flash启动，立即生效
/// @return error_t
error_t boot_configure_boot_from_flash(void);

/// @brief 使能dual bank特性
/// @return error_t
error_t boot_enable_dual_bank(void);

/// @brief 交换bank（将另一个bank映射到0x0800 0000，具体原理参考rm0454），立即生效
/// @return error_t
error_t boot_swap_bank(void);

/// @brief 获取当前bank号
/// @return uint8_t 0或1
uint8_t boot_get_current_bank(void);

/// @brief 跳转到系统bootloader执行
void boot_jump_to_bootloader(void);

#endif
