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

/**
 * @brief  微秒级延时
 * @param  xus 延时时长，范围：0~233015
 * @retval 无
 */
void delay_us(uint32_t xus)
{
    SysTick->LOAD = 72 * xus;   // 设置定时器重装值
    SysTick->VAL  = 0x00;       // 清空当前计数值
    SysTick->CTRL = 0x00000005; // 设置时钟源为HCLK，启动定时器
    while (!(SysTick->CTRL & 0x00010000))
        ;                       // 等待计数到0
    SysTick->CTRL = 0x00000004; // 关闭定时器
}

/**
 * @brief  毫秒级延时
 * @param  xms 延时时长，范围：0~4294967295
 * @retval 无
 */
void delay_ms(uint32_t xms)
{
    vTaskDelay(xms / portTICK_PERIOD_MS); // 延时
    // while (xms--) {
    //     delay_us(1000);
    // }
}

/**
 * @brief  秒级延时
 * @param  xs 延时时长，范围：0~4294967295
 * @retval 无
 */
void delay_s(uint32_t xs)
{
    vTaskDelay(xs * 1000 / portTICK_PERIOD_MS); // 延时
    // while (xs--) {
    //     delay_ms(1000);
    // }
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
    delay_ms(400); // at least 300ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_RESET, GPIO_PIN_RESET);
}

void ec800m_on(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_SET);
    delay_ms(800); // at least 700ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_RESET);
}

void ec800m_off(void)
{
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_SET);
    delay_ms(750); // at least 650ms
    HAL_GPIO_WritePin(GPIO_PIN_EC800M_POWER, GPIO_PIN_RESET);
}
