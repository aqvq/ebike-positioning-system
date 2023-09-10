#include "at.h"

/* GNSS AGPS状态: 1-打开 0-关闭 */
static uint8_t gnss_agps_state = 0;

static at_rsp_result_t gnss_agps_state_rsp_handler(char *rsp)
{
    char *line = NULL;
    line       = strstr(rsp, "+QAGPS");
    LOGD(TAG, "%s", rsp);
    if (line != NULL) {
        if (!sscanf(line, "+QAGPS: %d\r\n", &gnss_agps_state)) {
            LOGE(TAG, "format error (%s)", line);
        }
    }

    return AT_RSP_SUCCESS;
}

/**
 * @brief AGPS是否打开
 * @param state < 0 AGPS关闭或者异常, >=0 AGPS打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_agps_state(uint8_t *state)
{
#define gnss_agps_state_cmd "AT+QAGPS?\r\n"
    /* GNSS模块命令表 */
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

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_agps_state_cmd_table, array_size(at_gnss_agps_state_cmd_table));
    if (state) {
        *state = gnss_agps_state;
    }
    return res;
}

/**
 * @brief GNSS AGPS功能打开
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_enable_agps()
{
#define gnss_agps_open_cmd "AT+QAGPS=1\r\n"
    const static core_at_cmd_item_t at_gnss_enable_agps_cmd_table[] = {
        {
            /* 配置AGPS的相关参数 */
            .cmd     = gnss_agps_open_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_open_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_enable_agps_cmd_table, array_size(at_gnss_enable_agps_cmd_table));

    return res;
}

/**
 * @brief GNSS AGPS功能关闭
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_disable_agps()
{
#define gnss_agps_close_cmd "AT+QAGPS=0\r\n"
    const static core_at_cmd_item_t at_gnss_disable_agps_cmd_table[] = {
        {
            /* 配置AGPS的相关参数 */
            .cmd     = gnss_agps_close_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_close_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_disable_agps_cmd_table, array_size(at_gnss_disable_agps_cmd_table));

    return res;
}
