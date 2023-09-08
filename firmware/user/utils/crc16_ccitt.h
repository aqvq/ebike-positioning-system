
#ifndef _CRC16_CCITT_H_
#define _CRC16_CCITT_H_

#include <stdint.h>

/**
 *
 * @brief CRC16校验
 *
 * @param input   待校验的数据
 * @param len     待校验的数据的长度
 * @return        CRC校验结果
 *
 **/
uint16_t crc16_ccitt(const uint8_t *input, uint16_t len);

#endif