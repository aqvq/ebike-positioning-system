/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 14:42:37
 * @FilePath: \firmware\user\bsp\at\at_device.c
 * @Description: 设备相关的at控制命令
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "at.h"

#define TAG "AT_DEVICE"

// APN参数
#define IMEI_STRING_LENGTH (20)
static char imei_string[IMEI_STRING_LENGTH];
// rsp handler声明
static at_rsp_handler_t ec800m_imei_rsp_handler(char *rsp);

int32_t ec800m_at_poweroff()
{
#define poweroff_cmd "AT+QPOWD=1\r\n"
    const static core_at_cmd_item_t at_poweroff_cmd_table[] = {
        {
            .cmd     = poweroff_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(poweroff_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_poweroff_cmd_table, array_size(at_poweroff_cmd_table));
    return res;
}

/*
正常情况下，收到的回复格式如下
861197068734963

OK
*/
static at_rsp_handler_t ec800m_imei_rsp_handler(char *rsp)
{
    if (rsp != NULL) {
        if (!sscanf(rsp, "%*[^\n]\n%[^\r]", imei_string)) {
            LOGE(TAG, "format error (%s)", rsp);
        }
    }
    // LOGD(TAG, "imei: %s", imei_string);
    return AT_RSP_SUCCESS;
}

int32_t ec800m_at_imei(char *imei)
{
#define gnss_imei_cmd "AT+CGSN\r\n"
    const static core_at_cmd_item_t at_gnss_imei_cmd_table[] = {
        {
            /* 禁用通过 AT+QGPSGNME 获取 NMEA 语句 */
            .cmd     = gnss_imei_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_imei_cmd) - 1,
            .handler = (at_rsp_handler_t)ec800m_imei_rsp_handler,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    memset(imei_string, 0, sizeof(imei_string));
    res = core_at_commands_send_sync(at_gnss_imei_cmd_table, array_size(at_gnss_imei_cmd_table));

    if (imei) {
        strcpy(imei, imei_string);
    }

    return res;
}

int32_t ec800m_state()
{
#define state_cmd "AT\r\n"
    /* GNSS模块命令表 */
    const static core_at_cmd_item_t state_cmd_table[] = {
        {
            /* 查询GNSS状态 */
            .cmd        = state_cmd,
            .rsp        = "OK",
            .timeout_ms = 100,
            .cmd_len    = strlen(state_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(state_cmd_table, array_size(state_cmd_table));
    return res;
}
