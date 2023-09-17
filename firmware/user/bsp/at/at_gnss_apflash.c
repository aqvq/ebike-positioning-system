#include "at.h"
#define TAG "AT_GNSS_APFLASH"

static uint8_t gnss_apflash_state;

/**
 * @brief 启用/禁用 AP-Flash 快速热启动功能
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_enable_apflash()
{
#define gnss_apflash_open_cmd "AT+QGPSCFG=\"apflash\",1\r\n"
    const static core_at_cmd_item_t at_gnss_enable_apflash_cmd_table[] = {
        {
            /* 配_apflash的相关参数 */
            .cmd     = gnss_apflash_open_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_apflash_open_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_enable_apflash_cmd_table, array_size(at_gnss_enable_apflash_cmd_table));

    return res;
}

/**
 * @brief 启用/禁用 AP-Flash 快速热启动功能
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_disable_apflash()
{
#define gnss_apflash_close_cmd "AT+QGPSCFG=\"apflash\",0\r\n"
    const static core_at_cmd_item_t at_gnss_disable_apflash_cmd_table[] = {
        {
            /* 配_apflash的相关参数 */
            .cmd     = gnss_apflash_close_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_apflash_close_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_disable_apflash_cmd_table, array_size(at_gnss_disable_apflash_cmd_table));

    return res;
}

/*
+QGPSCFG: "apflash",1

OK
*/
static at_rsp_result_t gnss_apflash_state_rsp_handler(char *rsp)
{
    if (rsp != NULL) {
        if (!sscanf(rsp, "%*[^,],%hhd", &gnss_apflash_state)) {
            LOGE(TAG, "format error (%s)", rsp);
            return AT_RSP_FAILED;
        }
    }

    return AT_RSP_SUCCESS;
}

/**
 * @brief apflash是否打开
 * @param state =0 apflash关闭或者异常, =1 apflash打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_apflash_state(uint8_t *state)
{
#define gnss_apflash_state_cmd "AT+QGPSCFG=\"apflash\"\r\n"
    /* GNSS模块命令表 */
    const static core_at_cmd_item_t at_gnss_apflash_state_cmd_table[] = {
        {
            /* 查询GNSS状态 */
            .cmd        = gnss_apflash_state_cmd,
            .rsp        = "OK",
            .timeout_ms = 300,
            .handler    = gnss_apflash_state_rsp_handler,
            .cmd_len    = strlen(gnss_apflash_state_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_apflash_state_cmd_table, array_size(at_gnss_apflash_state_cmd_table));
    if (state) {
        *state = gnss_apflash_state;
    }
    return res;
}
