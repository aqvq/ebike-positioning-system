#include "at.h"

#define TAG "AT_GNSS"

/* GPS定位信息字符串长度 */
#define GNSS_STRING_LEN (128)

/* GPS定位信息字符串 */
extern char gnss_string[GNSS_STRING_LEN];

/* GNSS模块状态: 1-打开 0-关闭 */
static uint8_t gnss_state = 0;

// typedef struct gnss_response_t
// {
//     float longitude;
//     float latitude;
//     float altitude;
//     uint8_t fix; // 定位模式
//     float cog;   // 对地航向
//     float spkm;  // 对地速度
//     float spkn;
//     uint8_t utc[UTC_TIME_LEN]; // 格式：hhmmss.sss
//     uint8_t date[DATE_LEN];    // 格式：ddmmyy
//     uint8_t nsat;              // 卫星数量
//     float hdop;                // 水平精度因子, 范围：0.5~99.9
// } gnss_response_t;

/**
 * @brief GNSS AT 响应处理
 *
 * @param rsp
 * @return at_rsp_result_t
 */
at_rsp_result_t gnss_rsp_handler(char *rsp)
{
    memset(gnss_string, 0, sizeof(gnss_string));
    if (rsp != NULL) {
        if (strstr(rsp, "+QGPSLOC:") == NULL) {
            return AT_RSP_FAILED;
        }
        if (!sscanf(rsp, "%*[^+]%[^\r]", gnss_string)) {
            LOGE(TAG, "format error (%s)", rsp);
            return AT_RSP_FAILED;
        }
        // LOGD(TAG, "gnss: %s", gnss_string);
    }

    return AT_RSP_SUCCESS;
}

/*
+QGPS: 1

OK
*/
static at_rsp_result_t gnss_state_rsp_handler(char *rsp)
{
    if (rsp != NULL) {
        if (!sscanf(rsp, "%*s%hhd", &gnss_state)) {
            LOGE(TAG, "format error (%s)", rsp);
        }
    }

    return AT_RSP_SUCCESS;
}

/**
 * @brief GNSS是否打开
 * @param state < 0 GNSS关闭或者异常, >=0 GNSS打开
 * @return int32_t
 */
int32_t ec800m_at_gnss_state(uint8_t *state)
{
#define gnss_state_cmd "AT+QGPS?\r\n"
    /* GNSS模块命令表 */
    const static core_at_cmd_item_t at_gnss_state_cmd_table[] = {
        {
            /* 查询GNSS状态 */
            .cmd        = gnss_state_cmd,
            .rsp        = "OK",
            .timeout_ms = 2000,
            .handler    = gnss_state_rsp_handler,
            .cmd_len    = strlen(gnss_state_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_state_cmd_table, array_size(at_gnss_state_cmd_table));
    if (state) {
        *state = gnss_state;
    }
    return res;
}

/**
 * @brief 打开 GNSS
 *
 * @return int32_t < 0 操作失败, >=0 操作成功,返回已经消费掉的数据
 */
int32_t ec800m_at_gnss_open()
{
#define gnss_open_cmd "AT+QGPS=1\r\n"
    /* GNSS模块命令表 */
    const static core_at_cmd_item_t at_gnss_open_cmd_table[] = {
        {
            /* 打开GNSS */
            .cmd     = gnss_open_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_open_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_open_cmd_table, array_size(at_gnss_open_cmd_table));

    return res;
}

/**
 * @brief 关闭 GNSS
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_close()
{
#define gnss_close_cmd "AT+QGPSEND\r\n"
    const static core_at_cmd_item_t at_gnss_close_cmd_table[] = {
        {
            /* 关闭GNSS */
            .cmd     = gnss_close_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_close_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_close_cmd_table, array_size(at_gnss_close_cmd_table));

    return res;
}

/**
 * @brief GNSS 定位
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_location()
{
/* 查询场景激活的命令 */
#define gnss_location_cmd "AT+QGPSLOC=2\r\n"
    const static core_at_cmd_item_t at_gnss_location_cmd_table[] = {
        {
            /* 获取位置 */
            .cmd        = gnss_location_cmd,
            .rsp        = "OK",
            .handler    = gnss_rsp_handler,
            .timeout_ms = 2000,
            .cmd_len    = strlen(gnss_location_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_location_cmd_table, array_size(at_gnss_location_cmd_table));

    return res;
}

/**
 * @brief GNSS 定位设置
 * @param mode
 * 0 GPS
 * 1 GPS + BeiDou
 * 3 GPS + GLONASS + Galileo
 * 4 GPS + GLONASS
 * 5 GPS + BeiDou + Galileo
 * 6 GPS + Galileo
 * 7 BeiDou
 * @return int32_t
 */
int32_t ec800m_at_gnss_config(uint8_t mode)
{
    /* 配置支持的 GNSS 卫星导航系统 */
#define gnss_config_cmd "AT+QGPSCFG=\"gnssconfig\",%d\r\n"

    char tmp[AIOT_AT_CMD_LEN_MAXIMUM]                    = {0};
    static core_at_cmd_item_t at_gnss_config_cmd_table[] = {
        {
            .fmt        = gnss_config_cmd,
            .rsp        = "OK",
            .timeout_ms = 300,
        },
    };

    for (int i = 0; i < array_size(at_gnss_config_cmd_table); i++) {
        if (at_gnss_config_cmd_table[i].fmt != NULL) {
            snprintf(tmp, sizeof(tmp), at_gnss_config_cmd_table[i].fmt, mode);
            at_gnss_config_cmd_table[i].cmd     = tmp;
            at_gnss_config_cmd_table[i].cmd_len = strlen(tmp);
        }
    }

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_config_cmd_table, array_size(at_gnss_config_cmd_table));

    return res;
}
