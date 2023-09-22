/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 14:51:59
 * @FilePath: \firmware\user\bsp\at\at_gnss_autogps.c
 * @Description: at gnss autogps命令文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "at.h"
#define TAG "AT_GNSS_AUTOGPS"

static uint8_t gnss_autogps_state;

int32_t ec800m_at_gnss_enable_autogps()
{
#define gnss_autogps_open_cmd "AT+QGPSCFG=\"autogps\",1\r\n"
    const static core_at_cmd_item_t at_gnss_enable_autogps_cmd_table[] = {
        {
            /* 配_autogps的相关参数 */
            .cmd     = gnss_autogps_open_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_autogps_open_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_enable_autogps_cmd_table, array_size(at_gnss_enable_autogps_cmd_table));

    return res;
}

int32_t ec800m_at_gnss_disable_autogps()
{
#define gnss_autogps_close_cmd "AT+QGPSCFG=\"autogps\",0\r\n"
    const static core_at_cmd_item_t at_gnss_disable_autogps_cmd_table[] = {
        {
            /* 配_autogps的相关参数 */
            .cmd     = gnss_autogps_close_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_autogps_close_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_disable_autogps_cmd_table, array_size(at_gnss_disable_autogps_cmd_table));

    return res;
}
/*
正常情况下，收到的回复格式如下
+QGPSCFG: "autogps",1

OK
*/
static at_rsp_result_t gnss_autogps_state_rsp_handler(char *rsp)
{
    if (rsp != NULL) {
        if (!sscanf(rsp, "%*[^,],%hhd", &gnss_autogps_state)) {
            LOGE(TAG, "format error (%s)", rsp);
            return AT_RSP_FAILED;
        }
    }

    return AT_RSP_SUCCESS;
}

int32_t ec800m_at_gnss_autogps_state(uint8_t *state)
{
#define gnss_autogps_state_cmd "AT+QGPSCFG=\"autogps\"\r\n"
    /* GNSS模块命令表 */
    const static core_at_cmd_item_t at_gnss_autogps_state_cmd_table[] = {
        {
            /* 查询GNSS状态 */
            .cmd        = gnss_autogps_state_cmd,
            .rsp        = "OK",
            .timeout_ms = 300,
            .handler    = gnss_autogps_state_rsp_handler,
            .cmd_len    = strlen(gnss_autogps_state_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_autogps_state_cmd_table, array_size(at_gnss_autogps_state_cmd_table));
    if (state) {
        *state = gnss_autogps_state;
    }
    return res;
}
