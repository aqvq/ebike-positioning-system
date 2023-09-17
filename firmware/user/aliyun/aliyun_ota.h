
#ifndef _ALIYUN_OAT_H_
#define _ALIYUN_OAT_H_

#define OTA_RECV_BUFFER_SIZE (1024)

/// @brief 阿里云OTA升级初始化
/// @param mqtt_handle
/// @return
int32_t aliyun_ota_init(void *mqtt_handle);

/// @brief 阿里云OTA升级结束会话
/// @return
int32_t aliyun_ota_deinit(void);

#endif
