#include "at.h"

// APN参数
#define IMEI_STRING_LENGTH (20)
static char imei_string[IMEI_STRING_LENGTH];

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

static at_rsp_handler_t ec800m_imei_rsp_handler(char *rsp)
{
    char *line = rsp;
    uint16_t i = 0;
    if (line[0] == '\r' && line[1] == '\n') {
        // 如果开头是空行
        line += 2;
    }
    while (*line != '\0' && *line != '\r' && *line != '\n') {
        imei_string[i++] = *line++;
    }
    imei_string[i] = '\0';

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
