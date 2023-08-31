
#ifndef __IIC_H
#define __IIC_H
#include "main.h"
#include "utils/Delay.h"

// 软件IIC SDA引脚
#define IIC_SDA_PIN GPIOB, GPIO_PIN_11
// 软件IIC SCL引脚
#define IIC_SCL_PIN GPIOB, GPIO_PIN_10
// IIC SCL每个电平延时时间
#define IIC_DELAY 2

// iic初始化
void storage_iic_init(void);
// iic开始
void storage_iic_start(void);
// iic停止
void storage_iic_stop(void);
// iic发送字节
void storage_iic_send_byte(uint8_t Byte);
// iic接收字节
uint8_t storage_iic_receive_byte(void);
// iic发送ack
void storage_iic_send_ack(uint8_t AckBit);
// iic读ack
uint8_t storage_iic_receive_ack(void);

// 写SCL
static void IIC_W_SCL(uint8_t BitValue);
// 写SDA
static void IIC_W_SDA(uint8_t BitValue);
// 读SDA
static uint8_t IIC_R_SDA(void);
#endif
