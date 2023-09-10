#include "at.h"

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
static at_rsp_result_t gnss_rsp_handler(char *rsp)
{
#if 0
    if (strstr(rsp, "QGPSLOC"))
    {
        gnss_response_t gnss_response;
        sscanf((const char *)(rsp + strlen("+QGPSLOC: ")), "%10s,%fN/S,%fE/W,%f,%f,%d,%f,%f,%f,%6s,%d",
            &gnss_response.utc, &gnss_response.latitude, &gnss_response.longitude, &gnss_response.hdop,
            &gnss_response.altitude, &gnss_response.fix, &gnss_response.cog,
            &gnss_response.spkm, &gnss_response.spkn, gnss_response.date, &gnss_response.nsat);

        if (g_gnss_response_list == NULL)
        {
            g_gnss_response_list = linked_list_new();
        }

        if (g_gnss_response_list->len >= MAX_GNSS_CACHE_LEN)
        {
            linked_list_lpop(g_gnss_response_list);
        }

        linked_list_rpush(g_gnss_response_list, &gnss_response);
        LOGI(TAG, "gnss list len: %d", g_gnss_response_list->len);

        return AT_RSP_SUCCESS;
    }
    return AT_RSP_FAILED;
#endif

    // LOGI(TAG, "gnss_string len=%d", strlen(rsp));

    memset(gnss_string, 0, GNSS_STRING_LEN);
    memcpy(gnss_string, rsp, strlen(rsp));
    return AT_RSP_SUCCESS;
}

static at_rsp_result_t gnss_state_rsp_handler(char *rsp)
{
    char *line = NULL;
    line       = strstr(rsp, "+QGPS");

    if (line != NULL) {
        if (!sscanf(line, "+QGPS: %d\r\n", &gnss_state)) {
            LOGE(TAG, "format error (%s)", line);
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
