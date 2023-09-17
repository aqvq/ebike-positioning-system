#include "at.h"
#define TAG "AT_GNSS_AUTOGPS"

static uint8_t gnss_autogps_state;

/**
 * @brief 启用/禁用 GNSS 自启动
 *
 * @return int32_t0
 */
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

/**
 * @brief 启用/禁用 GNSS 自启动
 *
 * @return int32_t
 */
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

static at_rsp_result_t gnss_autogps_state_rsp_handler(char *rsp)
{
    // char line[128] = {0};
    // strcpy(line, rsp);
    // // line       = strstr(rsp, "+QGPSCFG");
    // LOGD(TAG, "%s", line);
    if (rsp != NULL) {
        if (!sscanf(rsp, "%*[\r\n]+QGPSCFG: \"autogps\",%hhd\r\n", &gnss_autogps_state)) {
            LOGE(TAG, "format error (%s)", rsp);
            return AT_RSP_FAILED;
        }
    }

    return AT_RSP_SUCCESS;
}

/**
 * @brief autogps是否打开
 * @param state =0 autogps关闭或者异常, =1 autogps打开
 * @return int32_t
 */
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
