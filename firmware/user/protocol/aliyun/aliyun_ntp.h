
#ifndef _ALIYUN_NTP_H_
#define _ALIYUN_NTP_H_

#include <stdlib.h>
#include <stdint.h>

/// @brief 阿里云NTP功能初始化
/// @param mqtt_handle
/// @return
int32_t aliyun_ntp_init(void **handle);

/// @brief 阿里云NTP结束会话
/// @return
int32_t aliyun_ntp_deinit(void);

#endif