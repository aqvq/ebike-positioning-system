#ifndef _ALIYUN_SHADOW_H_
#define _ALIYUN_SHADOW_H_

#include "aiot_shadow_api.h"

void demo_shadow_recv_handler(void *handle, const aiot_shadow_recv_t *recv, void *userdata);

// 报告当前的配置信息
int8_t update_shadow(void *shadow_handle);

/* 发送获取设备影子的请求 */
int32_t demo_get_shadow(void *shadow_handle);

#endif