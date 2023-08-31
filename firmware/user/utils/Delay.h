/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:12:05
 * @FilePath: \firmware\user\utils\Delay.h
 */

#ifndef __DELAY_H
#define __DELAY_H
#include <stdint.h>

// us延时函数
void delay_us(uint32_t us);
// ms延时函数
void delay_ms(uint32_t ms);
// s延时函数
void delay_s(uint32_t s);

#endif
