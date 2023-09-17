#include "at.h"

#define TAG "AT_GNSS_NEMA"

extern at_rsp_result_t gnss_nmea_rsp_handler(char *rsp); // EDITED

/**
 * @brief GNSS 使能通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
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

/**
 * @brief GNSS 禁用通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
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

/**
 * @brief GNSS 获取 GGA 语句
 *
 * @return int32_t
 */
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
