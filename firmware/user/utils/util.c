/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:11:19
 * @FilePath: \firmware\user\utils\util.c
 * @Description: 一些工具函数
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "util.h"
#include "common/config.h"
#include "aiot_state_api.h"
#include "storage/storage.h"
#include "bsp/at/at.h"

#define TAG "UTILITY"

char *get_device_name()
{
    static char device_name[20] = {0};
    if (device_name[0] != 0) {
        return device_name;
    }

    devinfo_wl_t device = {0};
    int8_t err          = read_device_info(&device);
    if (err == OK) {
        if (device.device_name[0] == 0xFF || device.device_name[0] == 0) {
            if (ec800m_at_imei(device_name) == STATE_SUCCESS && device_name[0] != 0) {
                return device_name;
            }
        }
        strcpy(device_name, device.device_name);
        return device_name;
    } else {
        return NULL;
    }
}

float mean(float *data, uint16_t len)
{
    if (data == NULL || len <= 0) {
        return 0;
    }

    float sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }

    return sum / len;
}
