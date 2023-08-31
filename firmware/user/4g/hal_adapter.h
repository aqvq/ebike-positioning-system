#ifndef _HAL_ADAPTER_H_
#define _HAL_ADAPTER_H_

// /**
//  * @brief 设备功能模式
//  *
//  */
// typedef enum {
//     TCP_MODE,
//     GNSS_MODE,
// } at_device_function_mode_t;

/**
 * @brief I2C初始化
 *
 * @return esp_err_t
 */
// esp_err_t i2c_master_init(void);

/**
 * @brief 初始化AT HAL层
 *
 * @return int32_t
 */
int32_t task_4g(void);

/**
 * @brief
 *
 * @param p_data
 * @param len
 * @param timeout
 * @return int32_t
 */
int32_t at_uart_send(uint8_t *p_data, uint16_t len, uint32_t timeout);

#endif // _HAL_ADAPTER_H_