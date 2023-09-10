
#ifndef _ALIYUN_DYNREG_H_
#define _ALIYUN_DYNREG_H_

#include "data/device_info.h"

/** 判断设备是否已经注册
 *
 * @return uint8_t
 */
uint8_t is_registered();

/**
 * @brief 动态注册
 *
 * @return int8_t
 */
int8_t dynamic_register();

/**
 * @brief 获取device secret
 *
 * @return uint8_t* device_secret指针
 */
char *get_device_secret();

#endif // _ALIYUN_DYNREG_H_
