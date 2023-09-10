#include "main.h"
#include <stdio.h>
#include "aiot_at_api.h"
#include "os_net_interface.h"
#include <string.h>
#include "ec800m_at_api.h"
#include "bsp/mcu/mcu.h"

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
#define _uart_recv_complete_cb      HAL_UART_RxCpltCallback
#define _uart_recv_half_complete_cb HAL_UART_RxHalfCpltCallback
#define _uart1_irq_cb               USART1_IRQHandler

/*recv data to at module*/
static void _usart_recv_user(uint8_t *data, uint32_t size)
{
    if (NULL == data || 0 == size) {
        return;
    }
    /* 为调试方便，在中断接收中打印了数据，正式生产需删除 */
    printf("<<< %s", data);
    aiot_at_hal_recv_handle(data, size);
    return;
}

/*uart recv callback*/
static void _usart_recv_isr(UART_HandleTypeDef *huart)
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

/*uart1 rx dma full complete interrupt callback*/
void _uart_recv_complete_cb(UART_HandleTypeDef *huart)
{
    _usart_recv_isr(huart);
}

/*uart1 rx dma half complete interrupt callback*/
void _uart_recv_half_complete_cb(UART_HandleTypeDef *huart)
{
    _usart_recv_isr(huart);
}
/*uart1 interrupt*/
void _uart1_irq_cb(void)
{
    /*check and process idle interrupt*/
    if ((__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)) {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        _usart_recv_isr(&huart1);
    }

    HAL_UART_IRQHandler(&huart1);
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
    printf(">>> %s", p_data);
    if (HAL_OK == K_UART_Transmit(&huart1, (uint8_t *)p_data, len)) {
        return len;
    } else {
        return 0;
    }
}

#ifdef __GNUC__
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 0xFFFF);
    return len;
}

#else

int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF); // 输出指向串口USART3
    return ch;
}

#endif

int32_t at_hal_init(void)
{
    printf("at_hal_init \r\n");

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
        printf("aiot_at_bootstrap failed\r\n");
        return -1;
    }

    return 0;
}

error_t ec800m_init()
{
    ec800m_vbat_on();
    ec800m_gnss_ant_on();
    ec800m_vbus_off();
    delay_ms(30);
    ec800m_on();
}
