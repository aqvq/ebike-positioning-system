/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:40:20
 * @FilePath: \firmware\user\aliyun\aliyun_message_handle.c
 * @Description: 本文件负责处理需要上传阿里云的数据
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "aliyun_message_handle.h"
#include "log/log.h"
#include "aliyun_config.h"
#include "aiot_mqtt_api.h"
#include "utils/util.h"
#include "aliyun_config.h"
#include "utils/time.h"

#define TAG  "ALIYUN_MESSAGE_HADLE"

/**
 * @description: 上传gnss数据
 * @param {void} *handle
 * @param {void} *data
 * @return {*}
 */
static void send_gnss_data_msg(void *handle, void *data)
{
    if (NULL == handle || data == NULL) {
        return;
    }

    char *gnss_string = (char *)data;

    char pub_topic[128];
    char *pub_payload;

    memset(pub_topic, 0, sizeof(pub_topic));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "gps", gnss_string);

    pub_payload = cJSON_Print(root);
    sprintf(pub_topic, "/%s/%s/%s/%s", PRODUCT_KEY, get_device_name(), "user", "gnss");

    int32_t res = aiot_mqtt_pub(handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);

    cJSON_Delete(root);
    cJSON_free(pub_payload);

    if (res < 0) {
        LOGE(TAG, "aiot_mqtt_pub failed, res: -0x%04X", -res);
    }
}

void handle_aliyun_message(void *handle, general_message_t *general_message)
{
    if (handle == NULL || general_message == NULL || general_message->data == NULL)
        return;

    switch (general_message->type) {
        case GNSS_DATA:
            send_gnss_data_msg(handle, general_message->data);
            break;
        default:
            LOGE(TAG, "unknow data type");
            break;
    }
}
