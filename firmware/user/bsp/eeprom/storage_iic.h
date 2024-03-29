/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:00:15
 * @FilePath: \firmware\user\bsp\eeprom\storage_iic.h
 * @Description: 软件iic实现头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef __IIC_H
#define __IIC_H
#include "main.h"
#include "bsp/mcu/mcu.h"

// 软件IIC SDA引脚
#define IIC_SDA_PIN GPIOA, GPIO_PIN_10
// 软件IIC SCL引脚
#define IIC_SCL_PIN GPIOA, GPIO_PIN_9
// IIC SCL每个电平延时时间
#define IIC_DELAY (2)

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
