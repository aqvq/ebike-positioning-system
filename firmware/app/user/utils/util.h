/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:12:41
 * @FilePath: \firmware\user\utils\util.h
 */

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief 判断mac是否是telink的mac地址
 *
 * @param mac mac地址
 * @return uint8_t 是: 1, 否:0
 */
uint8_t is_telink_mac(const uint8_t *mac);

/**
 * @brief 判断mac地址是否相等
 *
 * @param mac_a
 * @param mac_b
 * @return uint8_t 是: 1, 否:0
 */
uint8_t is_mac_equal(const uint8_t *mac_a, const uint8_t *mac_b);

/**
 * @brief 将mac地址转换为字符串
 *
 * @param mac mac地址
 * @param delimiter 分隔符
 * @param output 输出字符数组
 * @param output_len 输出字符数组长度
 */
void mac_to_str(const uint8_t *mac, const char delimiter, char *output, uint8_t *output_len);

/**
 * @brief 获取设备名(设备名就是mac地址的大写字符串)
 *
 * @return char*
 */
char *get_device_name();

uint8_t is_str_empty(const char *str, uint8_t len);
uint8_t is_str_equal(const char *str_a, const char *str_b, uint8_t len);

/// @brief CRC8校验，与传感器中的检验函数代码一致
/// @param ptr
/// @param len
/// @return
uint8_t CRC8(uint8_t *ptr, uint8_t len);

/// @brief 计算平均值
/// @param data
/// @param len
/// @return
float mean(float *data, uint16_t len);

/// @brief 设置系统时间
int set_time(int year, int mon, int day, int hour, int min, int sec);

/// @brief 将字符串数组转换为字节数组
/// @param in 输入字符串
/// @param len 字符串长度
/// @param out 输出字节目的地址
/// @return 状态码
int hex_string_to_byte(const char *in, uint16_t len, uint8_t *out);

uint8_t str_start_with(const char *str, const char *start);
uint8_t str_end_with(const char *str, const char *end);

#endif // _UTIL_H_