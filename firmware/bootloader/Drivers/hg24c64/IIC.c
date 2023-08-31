#include "IIC.h"

static void IIC_W_SCL(uint8_t BitValue)
{
	HAL_GPIO_WritePin(IIC_SCL_PIN, BitValue);
	delay_us(IIC_DELAY);
}

static void IIC_W_SDA(uint8_t BitValue)
{
	HAL_GPIO_WritePin(IIC_SDA_PIN, BitValue);
	delay_us(IIC_DELAY);
}

static uint8_t IIC_R_SDA(void)
{
	uint8_t BitValue;
	BitValue = HAL_GPIO_ReadPin(IIC_SDA_PIN);
	delay_us(IIC_DELAY);
	return BitValue;
}

void iic_init(void)
{
	HAL_GPIO_WritePin(IIC_SCL_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(IIC_SDA_PIN, GPIO_PIN_SET);
}

void iic_start(void)
{
	IIC_W_SDA(1);
	IIC_W_SCL(1);
	IIC_W_SDA(0);
	IIC_W_SCL(0);
}

void iic_stop(void)
{
	IIC_W_SDA(0);
	IIC_W_SCL(1);
	IIC_W_SDA(1);
}

void iic_send_byte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)
	{
		IIC_W_SDA(Byte & (0x80 >> i));
		IIC_W_SCL(1);
		IIC_W_SCL(0);
	}
}

uint8_t iic_receive_byte(void)
{
	uint8_t i, Byte = 0x00;
	IIC_W_SDA(1);
	for (i = 0; i < 8; i ++)
	{
		IIC_W_SCL(1);
		if (IIC_R_SDA() == 1){Byte |= (0x80 >> i);}
		IIC_W_SCL(0);
	}
	return Byte;
}

void iic_send_ack(uint8_t AckBit)
{
	IIC_W_SDA(AckBit);
	IIC_W_SCL(1);
	IIC_W_SCL(0);
}

uint8_t iic_receive_ack(void)
{
	uint8_t AckBit;
	IIC_W_SDA(1);
	IIC_W_SCL(1);
	AckBit = IIC_R_SDA();
	IIC_W_SCL(0);
	return AckBit;
}
