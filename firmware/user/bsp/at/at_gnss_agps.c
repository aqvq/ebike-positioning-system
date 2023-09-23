/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-23 11:05:41
 * @FilePath: \firmware\user\bsp\at\at_gnss_agps.c
 * @Description: at gnss agps命令
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#include "at.h"

#define TAG "AT_GNSS_AGPS"

/* GNSS AGPS状态: 1-打开 0-关闭 */
static uint8_t gnss_agps_state = 0;

/// @brief gnss agps状态回调函数
/// @param rsp 正常情况下，收到的回复格式如下
///                   +QAGPS: 1
///
///                   OK
/// @return 状态码 AT_RSP_SUCCESS or AT_RSP_FAILED
static at_rsp_result_t gnss_agps_state_rsp_handler(char *rsp)
{
    if (rsp != NULL) {
        // %*[^:]过滤前置字符串，%hhd获取8位数字存放到gnss_agps_state变量中
        if (!sscanf(rsp, "%*[^:]: %hhd", &gnss_agps_state)) {
            LOGE(TAG, "format error (%s)", rsp);
        }
    }

    return AT_RSP_SUCCESS;
}

int32_t ec800m_at_gnss_agps_state(uint8_t *state)
{
// at命令
#define gnss_agps_state_cmd "AT+QAGPS?\r\n"

    /* GNSS模块命令表 封装at命令*/
    const static core_at_cmd_item_t at_gnss_agps_state_cmd_table[] = {
        {
            /* 查询GNSS状态 */
            .cmd        = gnss_agps_state_cmd,
            .rsp        = "OK",
            .timeout_ms = 300,
            .handler    = gnss_agps_state_rsp_handler,
            .cmd_len    = strlen(gnss_agps_state_cmd) - 1,
        },
    };

    // 判断at模块是否完成初始化
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    // 发送at命令
    res = core_at_commands_send_sync(at_gnss_agps_state_cmd_table, array_size(at_gnss_agps_state_cmd_table));
    if (state) {
        // 如果state不为NULL，返回state值
        *state = gnss_agps_state;
    }

    // 返回状态码，一般为STATE_SUCCESS
    return res;
}

int32_t ec800m_at_gnss_enable_agps()
{
// at命令
#define gnss_agps_open_cmd "AT+QAGPS=1\r\n"
    const static core_at_cmd_item_t at_gnss_enable_agps_cmd_table[] = {
        {
            /* 配置AGPS的相关参数 */
            .cmd     = gnss_agps_open_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_open_cmd) - 1,
        },
    };

    // 判断at模块是否完成初始化
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    // 发送at命令
    res = core_at_commands_send_sync(at_gnss_enable_agps_cmd_table, array_size(at_gnss_enable_agps_cmd_table));

    // 返回状态码，一般为STATE_SUCCESS
    return res;
}

int32_t ec800m_at_gnss_disable_agps()
{
// at命令
#define gnss_agps_close_cmd "AT+QAGPS=0\r\n"
    const static core_at_cmd_item_t at_gnss_disable_agps_cmd_table[] = {
        {
            /* 配置AGPS的相关参数 */
            .cmd     = gnss_agps_close_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_close_cmd) - 1,
        },
    };

    // 判断at模块是否完成初始化
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    // 发送at命令
    res = core_at_commands_send_sync(at_gnss_disable_agps_cmd_table, array_size(at_gnss_disable_agps_cmd_table));

    // 返回状态码，一般为STATE_SUCCESS
    return res;
}
