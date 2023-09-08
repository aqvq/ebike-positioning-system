
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "log/log.h"
#include "utils/linked_list.h"
#include "4g/at_api.h"
#include "os_net_interface.h"
#include "4g/hal_adapter.h"
#include "main.h"
#include "gpio.h"
#include "main/app_config.h"
#include "main.h"
#include "aiot_state_api.h"

#define UART_TAG        "UART"

#define UART_FULL_PRINT (0)

//------------------------------------全局变量-------------------------------------------
extern uint8_t tcp_context_id;
extern uint8_t tcp_context_state;
extern uint8_t tcp_connect_id;
extern uint8_t tcp_connect_state;

// UART
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;

/* 移远模块设备类型 */
extern char QuectelDeviceType[20];

// #define UART_NUM1 (UART_NUM_1)
#define BUF_SIZE         (1600)
#define UART1_RX_PIN     (5)
#define UART1_TX_PIN     (4)
#define UART_BAUD_11520  (11520)
#define UART_BAUD_115200 (115200)
#define TOLERANCE        (0.02) // baud rate error tolerance 2%.

#define UART1_CTS_PIN    (13)
// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define UART1_RTS_PIN (18)

// Number of packets to be send during test
#define PACKETS_NUMBER (10)

// Wait timeout for uart driver
#define PACKET_READ_TICS            (1000 / portTICK_RATE_MS)

#define DEFAULT_CLK                 UART_SCLK_APB

#define I2C_MASTER_NUM              I2C_NUM_0 /*!< I2C port number for master dev */
#define I2C_MASTER_SCL_IO           19        /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           18        /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQ_HZ          100000    /*!< I2C master clock frequency */
#define I2C_PCA9557_SLAVE_ADDR      0x18

#define WRITE_BIT                   I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT                    I2C_MASTER_READ  /*!< I2C master read */

#define ACK_CHECK_EN                0x1 /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS               0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL                     0x0 /*!< I2C ack value */
#define NACK_VAL                    0x1 /*!< I2C nack value */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */

#define PCA9557_INPUT_PORT_REG      0
#define PCA9557_OUTPUT_PORT_REG     1
#define PCA9557_POLARITY_INVERT_REG 2
#define PCA9557_CONFIG_REG          3

static const char *TAG = "HAL_ADAPTER";

extern aiot_os_al_t g_aiot_freertos_api;
extern aiot_net_al_t g_aiot_net_at_api;

static QueueHandle_t uart_queue_lte;

/*AT module*/
extern at_device_t ec200_at_cmd;
extern at_device_t ec200_at_cmd_ssl;
extern at_device_t l610_at_cmd;
extern at_device_t l610_at_cmd_ssl;
extern at_device_t air724_at_cmd;
extern at_device_t air724_at_cmd_ssl;

// 使用EC200U
at_device_t *device = &ec200_at_cmd;

/* i2c_master是否已经初始化 */
static uint8_t is_i2c_master_initialized = 0;

/* AT HAL层是否已经初始化 */
static uint8_t is_at_hal_init = 0;

// >>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/*linkkit process tast id*/

#define RING_BUFFER_SIZE (AIOT_AT_DATA_RB_SIZE_DEFAULT)

typedef struct
{
    uint8_t data[RING_BUFFER_SIZE];
    uint16_t tail;
    uint16_t head;
} uart_ring_buffer_t;

uart_ring_buffer_t uart_dma_rx_buf = {0};

/*=================uart recv start=====================================*/

/*recv data to at module*/
static void _usart_recv_user(uint8_t *data, uint32_t size)
{
    if (NULL == data || 0 == size) {
        return;
    }
    /* 为调试方便，在中断接收中打印了数据，正式生产需删除 */
    // LOGT("AT", "<<< (size: %d) %s", size, data);
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

// >>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

// esp_err_t i2c_master_init(void)
// {
//     if (!is_i2c_master_initialized)
//     {
//         int i2c_master_port = I2C_MASTER_NUM;
//         i2c_config_t conf = {
//             .mode = I2C_MODE_MASTER,
//             .sda_io_num = I2C_MASTER_SDA_IO,
//             .sda_pullup_en = GPIO_PULLUP_DISABLE,
//             .scl_io_num = I2C_MASTER_SCL_IO,
//             .scl_pullup_en = GPIO_PULLUP_DISABLE,
//             .master.clk_speed = I2C_MASTER_FREQ_HZ,
//             // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
//         };

//         esp_err_t err = i2c_param_config(i2c_master_port, &conf);
//         if (err != ESP_OK)
//         {
//             return err;
//         }

//         err = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
//         if (err == ESP_OK)
//         {
//             is_i2c_master_initialized = 1;
//         }
//         return err;
//     }
//     return ESP_OK;
// }
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
    // if (HAL_OK == HAL_UART_Transmit_DMA(&huart1, (uint8_t *)p_data, len))
    if (HAL_OK == K_UART_Transmit(&huart1, (uint8_t *)p_data, len)) {
        // LOGT(TAG, ">>> (len: %d) %s", len, p_data);
        // printf("Heap Size: %d\n", xPortGetFreeHeapSize());
        return len;
    } else {
        return 0;
    }
}

// esp_err_t pca9557_write_register(uint8_t addr, uint8_t value)
// {
//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();

//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (I2C_PCA9557_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);
//     i2c_master_write(cmd, &addr, 1, ACK_CHECK_EN);
//     i2c_master_write(cmd, &value, 1, ACK_CHECK_EN);
//     i2c_master_stop(cmd);

//     esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
//     i2c_cmd_link_delete(cmd);

//     return ret;
// }

// static esp_err_t pca9557_read_register(uint8_t addr, uint8_t *value)
// {
//     esp_err_t ret = ESP_OK;

//     if (NULL == value)
//     {
//         return ESP_FAIL;
//     }

//     i2c_cmd_handle_t cmd = i2c_cmd_link_create();

//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (I2C_PCA9557_SLAVE_ADDR << 1) | WRITE_BIT, ACK_CHECK_EN);

//     i2c_master_write(cmd, &addr, 1, ACK_CHECK_EN);

//     i2c_master_start(cmd);
//     i2c_master_write_byte(cmd, (I2C_PCA9557_SLAVE_ADDR << 1) | READ_BIT, ACK_CHECK_EN);

//     i2c_master_read_byte(cmd, value, NACK_VAL);
//     i2c_master_stop(cmd);

//     ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
//     i2c_cmd_link_delete(cmd);

//     return ret;
// }

void power_on_ec200(void)
{

    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8) == GPIO_PIN_RESET) {
        LOGI("AT", "Starting EC200");
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
        HAL_Delay(3000);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
    } else {
        LOGI("AT", "EC200 is already on");
    }

    // ==========previous version===========

    // uint8_t register_value = 0;
    // // 配置(名词)寄存器, 用于控制IO方向, 全0表示所有IO口均为输出
    // pca9557_write_register(PCA9557_CONFIG_REG, 0x40);
    // pca9557_read_register(PCA9557_OUTPUT_PORT_REG, &register_value);

    // // 接下来控制EC200的PWRKEY为低电平, 延迟3s, 然后设置为高电平
    // register_value |= 0x08;
    // pca9557_write_register(PCA9557_OUTPUT_PORT_REG, register_value);
    // vTaskDelay(pdMS_TO_TICKS(2000));

    // register_value = 0;
    // pca9557_write_register(PCA9557_OUTPUT_PORT_REG, register_value);
    // pca9557_read_register(PCA9557_OUTPUT_PORT_REG, &register_value);

    // vTaskDelay(pdMS_TO_TICKS(200));
}

#define TMP_BUF_LEN (1024 * 5)
uint8_t tmp_buf[TMP_BUF_LEN] = {0};

static void uart_recieve_handler(void *pvParameters)
{
#if 0
    int len;
    uart_event_t event;
#endif
    // #define TMP_BUF_LEN 10240//1600
    //  uint8_t tmp_buf[TMP_BUF_LEN] = {0};

    for (;;) {
#if 0
        //Waiting for UART event.
        if (xQueueReceive(uart_queue_lte, (void *)&event, (portTickType)portMAX_DELAY))
        {
            memset(tmp_buf, 0, TMP_BUF_LEN);
            switch (event.type)
            {
            //Event of UART receving data
            case UART_DATA:
            {
                len = uart_read_bytes(UART_NUM1, tmp_buf, event.size, 100000);
                LOGW(TAG, "recv size: %d", event.size);
                tmp_buf[len] = 0;
                // LOGW(TAG, "recv content: %s", tmp_buf);
                aiot_at_hal_recv_handle(tmp_buf, len);
                break;
            }
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
            {
                LOGI(TAG, "hw fifo overflow");
                break;
            }
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
            {
                LOGI(TAG, "ring buffer full");
                break;
            }
            //Event of UART RX break detected
            case UART_BREAK:
            {
                LOGI(TAG, "uart rx break");
                break;
            }
            //Event of UART parity check error
            case UART_PARITY_ERR:
            {
                LOGI(TAG, "uart parity error");
                break;
            }
            //Event of UART frame error
            case UART_FRAME_ERR:
            {
                LOGI(TAG, "uart frame error");
                break;
            }
            //Others
            default:
                break;
            }
        }
#endif

#if 1
        // Read data from the UART
        /*
            uart_num – UART port number, the max port number is (UART_NUM_MAX -1).
            buf – pointer to the buffer.
            length – data length
            ticks_to_wait – sTimeout, count in RTOS ticks
        */
//         int len = uart_read_bytes(UART_NUM1, tmp_buf, TMP_BUF_LEN - 1, 1);
//         if (len)
//         {
// #if (UART_FULL_PRINT)
//             tmp_buf[len] = '\0';
//             LOGW(TAG, "recv size: %d", len);
//             LOGW(TAG, "recv: %s", tmp_buf);
// #endif
//             aiot_at_hal_recv_handle(tmp_buf, len);
//         }
#endif
        //         // vTaskDelay(1 / portTICK_PERIOD_MS);
        //         vTaskDelay(2 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/**
 * @brief 初始化和AT模块交互的UART
 *
 */
static int8_t aiot_at_uart_init(void)
{
    uart_dma_rx_buf.head = uart_dma_rx_buf.tail = 0;
    if (HAL_OK != HAL_UART_Receive_DMA(&huart1, uart_dma_rx_buf.data, RING_BUFFER_SIZE)) {
        Error_Handler();
    }

    // const int uart_num = UART_NUM1;
    // uint8_t *rd_data = (uint8_t *)malloc(1600);
    // if (rd_data == NULL)
    // {
    //     LOGI(TAG, "rx buffer malloc fail");
    // }

    // uart_config_t uart_config = {
    //     .baud_rate = UART_BAUD_115200,
    //     .data_bits = UART_DATA_8_BITS,
    //     .parity = UART_PARITY_DISABLE,
    //     .stop_bits = UART_STOP_BITS_1,
    //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //     .source_clk = DEFAULT_CLK,
    // };

    // esp_err_t ret = ESP_OK;
    // ret = uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue_lte, 0);

    // if (ESP_OK != ret)
    // {
    //     LOGE(TAG, "uart_driver_install faile[%d] ", ret);
    //     return ESP_FAIL;
    // }

    // uart_param_config(uart_num, &uart_config);
    // uart_set_pin(uart_num, UART1_TX_PIN, UART1_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // uart_wait_tx_done(uart_num, portMAX_DELAY);
    // vTaskDelay(1 / portTICK_PERIOD_MS); // make sure last byte has flushed from TX FIFO
    // uart_flush_input(uart_num);

    int32_t res = 0;
    // res = xTaskCreate(uart_recieve_handler, "uTask", 4096, NULL, 12, NULL);
    // if (res < 0)
    // {
    //     vTaskDelete(NULL);
    //     return res;
    // }

    return res;
    // return ret;
}

/**
 * @brief 初始化AT模块
 *
 * @return int32_t
 */
int32_t task_4g(void)
{
    power_on_ec200(); /* 4g模块上电在uart初始化后做 */
    if (is_at_hal_init) {
        LOGI(TAG, "at_hal_init before");
        return 0;
    }

    /*设置设备系统接口及网络接口*/
    aiot_install_os_api(&g_aiot_freertos_api);
    aiot_install_net_api(&g_aiot_net_at_api);

    // esp_err_t ret = ESP_OK;
    // ret = i2c_master_init();
    // if (ret != ESP_OK)
    // {
    //     LOGE(TAG, "i2c_master_init failed, error code: %d", ret);
    //     return -1;
    // }

    int8_t ret = aiot_at_uart_init();
    if (ret != 0) {
        LOGE(TAG, "aiot_at_uart_init failed, error code: %d", ret);
        return -1;
    }

    /*at_module_init*/
    int res = aiot_at_init();
    if (res < 0) {
        LOGE(TAG, "aiot_at_init failed, error code: %d", res);
        return -1;
    }

    /*设置发送接口*/
    aiot_at_setopt(AIOT_ATOPT_UART_TX_FUNC, at_uart_send);

    /*设置模组*/
    aiot_at_setopt(AIOT_ATOPT_DEVICE, device);

    res = aiot_at_bootstrap();

    if (res < STATE_SUCCESS) {
        LOGE(TAG, "at_hal_init failed, restart");
        ec200_poweroff_and_mcu_restart();
        return -1;
    }

    LOGI(TAG, "at hal init ok");

#if MQTT_ENABLED
    extern int32_t iot_connect(void);
    iot_connect();
    // xTaskCreate(iot_connect, "iot", 512, NULL, 3, NULL);
#endif

#if GNSS_ENABLED
    extern void gnss_init(void);
    gnss_init();
    // xTaskCreate(gnss_init, "gnss_init", 512, NULL, 3, NULL);
#endif

#if 0
    /*查询场景状态*/
    res = aiot_at_context_state();
    if (res >= 0 && tcp_context_id == 1 && tcp_context_state == 1) // 场景1激活
    {
        LOGI(TAG, "context actived.");

        /*查询TCP状态*/
        res = aiot_at_tcp_state();
        if (res >= 0 && tcp_connect_state == 2) // TCP已经连接
        {
            LOGI(TAG, "tcp %d connected.", tcp_connect_id);
            res = aiot_at_tcp_close();

            if (res < 0) {
                LOGW(TAG, "close tcp failed, res=%d.", res);
            }
        } else {
            LOGI(TAG, "tcp closed.");
        }
    } else {
        LOGI(TAG, "context not actived, res=%d.", res);

        power_on_ec200(); /* 4g模块上电在uart初始化后做 */

        /*初始化模组及获取到IP网络*/
        uint8_t retry = 0;
        do {
            vTaskDelay(1000 / portTICK_PERIOD_MS);

            res = aiot_at_bootstrap();
            if (res < 0) {
                LOGE(TAG, "aiot_at_bootstrap failed, error code: %d", res);
                // vTaskDelay(3000 / portTICK_PERIOD_MS);
                retry++;
            }
        } while (res < 0 && retry < 9);

        if (res < 0) {
            return -1;
        } else {
            LOGI(TAG, "Quectel Device Type:%s", QuectelDeviceType);
        }
    }

    is_at_hal_init = 1;
#endif

    vTaskDelete(NULL);
    return 0;
}
