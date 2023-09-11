
#ifndef __BSP_MCU_H
#define __BSP_MCU_H
#include <stdint.h>

#define GPIO_PIN_EC800M_POWER       GPIOD, GPIO_PIN_0
#define GPIO_PIN_EC800M_RESET       GPIOD, GPIO_PIN_1
#define GPIO_PIN_EC800M_VBAT_EN     GPIOD, GPIO_PIN_2
#define GPIO_PIN_EC800M_GNSS_ANT_EN GPIOD, GPIO_PIN_3
#define GPIO_PIN_EC800M_VBUS_CTRL   GPIOD, GPIO_PIN_4
#define GPIO_PIN_EC800M_DTR         GPIOB, GPIO_PIN_2
#define GPIO_PIN_EC800M_RI          GPIOB, GPIO_PIN_1
#define GPIO_PIN_EC800M_DCD         GPIOB, GPIO_PIN_0
#define GPIO_PIN_EC800M_RXD         GPIOC, GPIO_PIN_5
#define GPIO_PIN_EC800M_TXD         GPIOC, GPIO_PIN_4
#define GPIO_PIN_EC800M_RTS         GPIOB, GPIO_PIN_3
#define GPIO_PIN_EC800M_CTS         GPIOB, GPIO_PIN_4
#define GPIO_PIN_EC800M_DP          GPIOA, GPIO_PIN_12
#define GPIO_PIN_EC800M_DM          GPIOA, GPIO_PIN_11

// us延时函数
void delay_us(uint32_t us);
// ms延时函数
void delay_ms(uint32_t ms);
// s延时函数
void delay_s(uint32_t s);

void mcu_restart(void);
void mcu_init(void);
void ec800m_vbus_on(void);
void ec800m_vbus_off(void);
void ec800m_gnss_ant_on(void);
void ec800m_gnss_ant_off(void);
void ec800m_vbat_on(void);
void ec800m_vbat_off(void);
void ec800m_reset(void);
void ec800m_on(void);
void ec800m_off(void);

#endif
