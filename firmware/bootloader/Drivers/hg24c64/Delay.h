/*
 * @Date: 2023-08-30 09:23:19
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 16:58:59
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
