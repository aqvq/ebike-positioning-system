/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-23 11:26:21
 * @FilePath: \firmware\user\bsp\at\hal_adapter.c
 * @Description: AT串口驱动文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#include "main.h"
#include <stdio.h>
#include "aiot_at_api.h"
#include "os_net_interface.h"
#include <string.h>
#include "bsp/at/at.h"
#include "bsp/mcu/mcu.h"

#define TAG "AT_HAL"

/*huart1 for at module*/
extern UART_HandleTypeDef huart1;
/*huart2 for log printf*/
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

extern aiot_os_al_t g_aiot_freertos_api;
extern aiot_net_al_t g_aiot_net_at_api;

/*AT module*/
extern at_device_t ec800m_at_cmd;
extern at_device_t ec800m_at_cmd_ssl;
extern at_device_t l610_at_cmd;
extern at_device_t l610_at_cmd_ssl;
extern at_device_t air724_at_cmd;
extern at_device_t air724_at_cmd_ssl;
extern at_device_t ec800m_at_cmd;
at_device_t *device = &ec800m_at_cmd;

/*linkkit process tast id*/
#define RING_BUFFER_SIZE (AIOT_AT_DATA_RB_SIZE_DEFAULT)

typedef struct {
    uint8_t data[RING_BUFFER_SIZE];
    uint16_t tail;
    uint16_t head;
} uart_ring_buffer_t;

static uart_ring_buffer_t uart_dma_rx_buf = {0};

/*=================uart recv start=====================================*/

/*recv data to at module*/
static void _usart_recv_user(uint8_t *data, uint32_t size)
{
    if (NULL == data || 0 == size) {
        return;
    }
    /* 为调试方便，在中断接收中打印了数据，正式生产需删除 */
    // printf("<<< %s", data);
    aiot_at_hal_recv_handle(data, size);
    return;
}

/*uart recv callback*/
void _usart_recv_isr(UART_HandleTypeDef *huart)
{
    uint32_t recv_len = 1;

    if (huart == &huart1) {
        uart_dma_rx_buf.head = uart_dma_rx_buf.tail;
        uart_dma_rx_buf.tail = RING_BUFFER_SIZE - hdma_usart1_rx.Instance->CNDTR; // cndtr???????
        if (uart_dma_rx_buf.tail == uart_dma_rx_buf.head) {
            return;
        } else if (uart_dma_rx_buf.tail > uart_dma_rx_buf.head) {
            recv_len = uart_dma_rx_buf.tail - uart_dma_rx_buf.head;
            _usart_recv_user(&uart_dma_rx_buf.data[uart_dma_rx_buf.head], recv_len);
        } else {
            recv_len = RING_BUFFER_SIZE - uart_dma_rx_buf.head;
            _usart_recv_user(&uart_dma_rx_buf.data[uart_dma_rx_buf.head], recv_len);
            recv_len = uart_dma_rx_buf.tail;
            _usart_recv_user(&uart_dma_rx_buf.data[0], recv_len);
        }
    }
}

/*=================uart recv end=====================================*/

int g_tx_error = 0;
static HAL_StatusTypeDef K_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    HAL_StatusTypeDef status = HAL_OK;
    for (int32_t i = 1; i < 100000; ++i) {
        status = HAL_UART_Transmit_IT(huart, pData, Size);
        if (HAL_OK == status) {
            return status;
        } else if (HAL_BUSY == status) {
            if (HAL_UART_STATE_READY == huart->gState && HAL_LOCKED == huart->Lock && i % 50 == 0) {
                g_tx_error++;
                __HAL_UNLOCK(huart);
                continue;
            }
        } else if (HAL_ERROR == status) {
            return status;
        } else if (HAL_TIMEOUT == status) {
            continue;
        }
    }

    return status;
}

int32_t at_uart_send(uint8_t *p_data, uint16_t len, uint32_t timeout)
{
    // if (HAL_OK == HAL_UART_Transmit_DMA(&huart1, (uint8_t *)p_data, len)) {
    // printf(">>> %s", p_data);
    if (HAL_OK == K_UART_Transmit(&huart1, (uint8_t *)p_data, len)) {
        return len;
    } else {
        return 0;
    }
}

int32_t at_hal_init(void)
{
    LOGD(TAG, "at_hal_init");

    /*设置设备系统接口及网络接口*/
    aiot_install_os_api(&g_aiot_freertos_api);
    aiot_install_net_api(&g_aiot_net_at_api);

    // enable idle interrupt
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);

    /*enabled dma cycle recv*/
    uart_dma_rx_buf.head = uart_dma_rx_buf.tail = 0;
    if (HAL_OK != HAL_UART_Receive_DMA(&huart1, uart_dma_rx_buf.data, RING_BUFFER_SIZE)) {
        Error_Handler();
    }

    /*at_module_init*/
    int res = aiot_at_init();
    if (res < 0) {
        printf("aiot_at_init failed\r\n");
        return -1;
    }

    /*设置发送接口*/
    aiot_at_setopt(AIOT_ATOPT_UART_TX_FUNC, at_uart_send);
    /*设置模组*/
    aiot_at_setopt(AIOT_ATOPT_DEVICE, device);

    /*初始化模组及获取到IP网络*/
    res = aiot_at_bootstrap();
    if (res < 0) {
        printf("aiot_at_bootstrap failed (res: %d)\r\n", res);
        return -1;
    }

    return 0;
}

error_t ec800m_init()
{
    LOGD(TAG, "device init");
    // 开启ec800m模块电源
    ec800m_vbat_on();
    // 开启有源天线电源
    ec800m_gnss_ant_on();
    // 关闭usb_vbus电源（暂时用不到，默认关闭）
    ec800m_vbus_off();
    // 等待电源稳定
    delay_ms(30);
    // ec800m开机
    ec800m_on();
    // v1.0.0中stm32未接入ec800m的status引脚，无法获知ec800m开关机状态
    // 使用此方法强制每次启动时重启，保证开机后模块处于工作状态
    ec800m_reset();
}
