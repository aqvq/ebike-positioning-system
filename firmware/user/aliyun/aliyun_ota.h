/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:46:17
 * @FilePath: \firmware\user\aliyun\aliyun_ota.h
 * @Description: 阿里云OTA头文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _ALIYUN_OAT_H_
#define _ALIYUN_OAT_H_

#define OTA_RECV_BUFFER_SIZE (1024 * 10)

/// @brief 阿里云OTA升级初始化
/// @param mqtt_handle
/// @return
int32_t aliyun_ota_init(void *mqtt_handle);

/// @brief 阿里云OTA升级结束会话
/// @return
int32_t aliyun_ota_deinit(void);

#endif
