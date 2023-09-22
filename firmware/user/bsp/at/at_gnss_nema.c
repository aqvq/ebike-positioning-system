/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 14:53:33
 * @FilePath: \firmware\user\bsp\at\at_gnss_nema.c
 * @Description: at gnss nema命令文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "at.h"

#define TAG "AT_GNSS_NEMA"

// rsp handler
extern at_rsp_result_t gnss_nmea_rsp_handler(char *rsp);


int32_t ec800m_at_gnss_nmea_enable()
{
#define gnss_nmea_enable_cmd "AT+QGPSCFG=\"nmeasrc\",1\r\n"
    const static core_at_cmd_item_t at_gnss_nmea_enable_cmd_table[] = {
        {
            /* 配置多系统混合定位 NMEA 语句的输出类型 */
            .cmd     = gnss_nmea_enable_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_nmea_enable_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nmea_enable_cmd_table, array_size(at_gnss_nmea_enable_cmd_table));

    return res;
}

int32_t ec800m_at_gnss_nmea_disable()
{
#define gnss_nmea_disable_cmd "AT+QGPSCFG=\"nmeasrc\",0"
    const static core_at_cmd_item_t at_gnss_nmea_disable_cmd_table[] = {
        {
            /* 禁用通过 AT+QGPSGNME 获取 NMEA 语句 */
            .cmd     = gnss_nmea_disable_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_nmea_disable_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nmea_disable_cmd_table, array_size(at_gnss_nmea_disable_cmd_table));

    return res;
}

int32_t ec800m_at_gnss_nmea_query()
{
#define gnss_nmea_query_cmd1 "AT+QGPSGNMEA=\"GSA\"\r\n"
#define gnss_nmea_query_cmd2 "AT+QGPSGNMEA=\"GGA\"\r\n"
#define gnss_nmea_query_cmd3 "AT+QGPSGNMEA=\"RMC\"\r\n"
#define gnss_nmea_query_cmd4 "AT+QGPSGNMEA=\"VTG\"\r\n"

    const static core_at_cmd_item_t at_gnss_nmea_query_cmd_table[] = {
        {
            /* 获取 GSA 语句 */
            .cmd        = gnss_nmea_query_cmd1,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nmea_query_cmd1) - 1,
        },
        {
            /* 获取 GGA 语句 */
            .cmd        = gnss_nmea_query_cmd2,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nmea_query_cmd2) - 1,
        },
        {
            /* 获取 RMC 语句 */
            .cmd        = gnss_nmea_query_cmd3,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nmea_query_cmd3) - 1,
        },
        {
            /* 获取 VTG 语句 */
            .cmd        = gnss_nmea_query_cmd4,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nmea_query_cmd4) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nmea_query_cmd_table, array_size(at_gnss_nmea_query_cmd_table));

    return res;
}
