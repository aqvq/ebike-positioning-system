/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 16:13:02
 * @FilePath: \firmware\user\bsp\mcu\mcu.h
 * @Description: mcu驱动代码头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef __BSP_MCU_H
#define __BSP_MCU_H
#include <stdint.h>

/****************************************** MCU GPIO引脚定义 ******************************************/

// ec800m power电源控制引脚
#define GPIO_PIN_EC800M_POWER GPIOD, GPIO_PIN_0
// ec800m reset控制引脚
#define GPIO_PIN_EC800M_RESET GPIOD, GPIO_PIN_1
// ec800m vbat电源控制引脚
#define GPIO_PIN_EC800M_VBAT_EN GPIOD, GPIO_PIN_2
// ec800m gnss天线电源控制引脚
#define GPIO_PIN_EC800M_GNSS_ANT_EN GPIOD, GPIO_PIN_3
// ec800m usb vbus电源控制引脚
#define GPIO_PIN_EC800M_VBUS_CTRL GPIOD, GPIO_PIN_4
// ec800m dtr引脚
#define GPIO_PIN_EC800M_DTR GPIOB, GPIO_PIN_2
// ec800m ri引脚
#define GPIO_PIN_EC800M_RI GPIOB, GPIO_PIN_1
// ec800m dcd引脚
#define GPIO_PIN_EC800M_DCD GPIOB, GPIO_PIN_0
// ec800m rxd引脚
#define GPIO_PIN_EC800M_RXD GPIOC, GPIO_PIN_5
// ec800m txd引脚
#define GPIO_PIN_EC800M_TXD GPIOC, GPIO_PIN_4
// ec800m rts引脚
#define GPIO_PIN_EC800M_RTS GPIOB, GPIO_PIN_3
// ec800m cts引脚
#define GPIO_PIN_EC800M_CTS GPIOB, GPIO_PIN_4
// ec800m usb dp引脚
#define GPIO_PIN_EC800M_DP GPIOA, GPIO_PIN_12
// ec800m usb dm引脚
#define GPIO_PIN_EC800M_DM GPIOA, GPIO_PIN_11

/****************************************** MCU驱动函数定义 ******************************************/

/// @brief mcu重启
void mcu_restart(void);

/// @brief mcu初始化，主要是option bytes的设置
void mcu_init(void);

/// @brief ec800m开启usb vbus
void ec800m_vbus_on(void);

/// @brief ec800m关闭usb vbus
void ec800m_vbus_off(void);

/// @brief ec800m开启gnss天线电源
void ec800m_gnss_ant_on(void);

/// @brief ec800m关闭gnss天线电源
void ec800m_gnss_ant_off(void);

/// @brief ec800m开启vbat
void ec800m_vbat_on(void);

/// @brief ec800m关闭vbat
void ec800m_vbat_off(void);

/// @brief ec800m重启
void ec800m_reset(void);

/// @brief 开启ec800m
void ec800m_on(void);

/// @brief 关闭ec800m
void ec800m_off(void);

/****************************************** 延时函数定义 ******************************************/

/**
 * @brief  微秒级延时
 * @param  xus 延时时长，范围：0~233015
 * @retval 无
 */
void delay_us(uint32_t us);

/**
 * @brief  毫秒级延时
 * @param  xms 延时时长，范围：0~4294967295
 * @retval 无
 */
void delay_ms(uint32_t ms);

/**
 * @brief  秒级延时
 * @param  xs 延时时长，范围：0~4294967295
 * @retval 无
 */
void delay_s(uint32_t s);

#endif
