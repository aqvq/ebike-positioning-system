/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-23 11:27:41
 * @FilePath: \firmware\user\bsp\mcu\mcu.c
 * @Description: mcu驱动代码
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "main.h"
#include "mcu.h"
#include "bsp/flash/boot.h"
#include "log/log.h"
#include "FreeRTOS.h"
#include "task.h"

#define TAG "MCU"

void mcu_restart(void)
{
    __ASM volatile("cpsid i"); // 关中断
    HAL_NVIC_SystemReset();    // 重启
}

void mcu_init(void)
{
    boot_configure_boot_from_flash();
    boot_enable_dual_bank();
}

void ec800m_vbus_on(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_VBUS_CTRL, GPIO_PIN_SET);
}

void ec800m_vbus_off(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_VBUS_CTRL, GPIO_PIN_RESET);
}

void ec800m_gnss_ant_on(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_GNSS_ANT_EN, GPIO_PIN_SET);
}

void ec800m_gnss_ant_off(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_GNSS_ANT_EN, GPIO_PIN_RESET);
}

void ec800m_vbat_on(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_VBAT_EN, GPIO_PIN_RESET);
}

void ec800m_vbat_off(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_VBAT_EN, GPIO_PIN_SET);
}

void ec800m_reset(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_RESET, GPIO_PIN_SET);
    delay_ms(500); // at least 300ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_RESET, GPIO_PIN_RESET);
}

void ec800m_on(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_SET);
    delay_ms(900); // at least 700ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_RESET);
}

void ec800m_off(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_SET);
    delay_ms(850); // at least 650ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_RESET);
}

void delay_us(uint32_t xus)
{
    // 72MHz
    // TODO: 这里需要修改
    SysTick->LOAD = 72 * xus;   // 设置定时器重装值
    SysTick->VAL  = 0x00;       // 清空当前计数值
    SysTick->CTRL = 0x00000005; // 设置时钟源为HCLK，启动定时器
    while (!(SysTick->CTRL & 0x00010000))
        ;                       // 等待计数到0
    SysTick->CTRL = 0x00000004; // 关闭定时器
}

void delay_ms(uint32_t xms)
{
    // vTaskDelay(xms / portTICK_PERIOD_MS); // 延时
    while (xms--) {
        delay_us(1000);
    }
}

void delay_s(uint32_t xs)
{
    // vTaskDelay(xs * 1000 / portTICK_PERIOD_MS); // 延时
    while (xs--) {
        delay_ms(1000);
    }
}
