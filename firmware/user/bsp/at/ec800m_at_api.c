#include <stdio.h>
#include <string.h>
#include "aiot_at_api.h"
#include "log/log.h"
#include "utils/linked_list.h"
#include "aiot_state_api.h"
#include "utils/macros.h"

//----------------------------------全局变量---------------------------------------------
/* GPS定位信息字符串长度 */
#define GNSS_STRING_LEN (128)

/* GPS定位信息字符串 */
char gnss_string[GNSS_STRING_LEN];

/* GNSS模块状态: 1-打开 0-关闭 */
uint8_t gnss_state = 0;

/* TCP通讯场景ID */
uint8_t tcp_context_id = 0;

/* TCP通讯场景1状态: 0 去激活 1 激活 */
uint8_t tcp_context_state = 0;

/* TCP通讯场景1中连接ID */
uint8_t tcp_connect_id = 0;

/* TCP通讯场景1中连接1TCP状态:
 * 0 "Initial"：尚未建立连接
 * 1 "Opening"：客户端正在连接或者服务器正尝试监听
 * 2 "Connected"：客户端连接已建立
 * 3 "Listening"：服务器正在监听
 * 4 "Closing"：连接断开
 */
uint8_t tcp_connect_state = 0;

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

// APN参数，由main.c设定
extern char g_ec200_apn_cmd[256];

extern core_at_handle_t at_handle;
// GNSS定位NMEA数据
// TODO: multiple definition of `gnss_nmea_rsp_handler';
extern at_rsp_result_t gnss_nmea_rsp_handler(char *rsp); // EDITED
// at_rsp_result_t gnss_nmea_rsp_handler(char *rsp){return 0;}

static const char *TAG = "EC800M";

#if 0
#define UTC_TIME_LEN       (10)
#define DATE_LEN           (6)
#define MAX_GNSS_CACHE_LEN (10)

linked_list_t *g_gnss_response_list = NULL;
#endif

/**
 * 兼容AC6编译器
 */
void _scanf_mbtowc()
{
    ;
}

static at_rsp_result_t gnss_rsp_handler(char *rsp);
static at_rsp_result_t gnss_state_rsp_handler(char *rsp);
static at_rsp_result_t context_state_rsp_handler(char *rsp);
static at_rsp_result_t tcp_state_rsp_handler(char *rsp);

/********************************************************************
 * 基本命令定义
 *********************************************************************/

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {
        /* 设置身份认证参数 */
        .cmd = "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n",
        .rsp = "OK",
    },
    {
        /* 去场景激活 */
        .cmd = "AT+QIDEACT=1\r\n",
        .rsp = "OK",
    },
    {
        /* 场景激活 */
        .cmd = "AT+QIACT=1\r\n",
        .rsp = "OK",
    },
    {
        /* 查询场景激活 */
        .cmd = "AT+QIACT?\r\n",
        .rsp = "OK",
    },
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {
        /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        .fmt = "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1\r\n",
        .rsp = "+QIOPEN",
    },
};

/* 发送数据AT命令表 */
static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+QISEND=%d,%d\r\n",
        .rsp = ">",
    },
    {
        /* 纯数据，没有格式*/
        .rsp = "SEND OK",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {
        /* 关闭TCP连接 */
        .fmt = "AT+QICLOSE=%d\r\n",
        .rsp = "OK",
    },
};

static core_at_recv_data_prefix at_recv = {
    .prefix = "+QIURC: \"recv\"",
    .fmt    = "+QIURC: \"recv\",%d,%d\r\n",
};

at_device_t ec800m_at_cmd = {
    .ip_init_cmd      = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd      = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .send_cmd      = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd      = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),

    .recv         = &at_recv,
    .error_prefix = "ERROR",
};

/********************************************************************
 * 特有命令定义
 *********************************************************************/

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
 * @brief GNSS是否打开
 *
 * @return int32_t < 0 GNSS关闭或者异常, >=0 GNSS打开
 */
int32_t ec800m_at_gnss_state()
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

    return res;
}

/**
 * @brief 场景是否打开
 *
 * @return int32_t < 0 场景关闭或者异常, >=0 场景打开
 */
int32_t ec800m_at_context_state()
{
/* 查询场景激活的命令 */
#define context_state_cmd "AT+QIACT?\r\n"
    const static core_at_cmd_item_t at_context_state_cmd_table[] = {
        {
            .cmd = context_state_cmd,
            //  .rsp = "OK",
            .handler    = context_state_rsp_handler,
            .timeout_ms = 200,
            .cmd_len    = strlen(context_state_cmd) - 1,
        },
    };
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_context_state_cmd_table, array_size(at_context_state_cmd_table));

    return res;
}

/**
 * @brief TCP是否打开
 *
 * @return int32_t < 0 TCP关闭或者异常, >=0 TCP打开
 */
int32_t ec800m_at_tcp_state()
{

/* 查询场景激活的命令 */
#define tcp_state_cmd "AT+QISTATE=0,1\r\n"
    const static core_at_cmd_item_t at_tcp_state_cmd_table[] = {
        {
            .cmd = tcp_state_cmd,
            //  .rsp = "OK",
            .handler    = tcp_state_rsp_handler,
            .timeout_ms = 200,
            .cmd_len    = strlen(tcp_state_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_tcp_state_cmd_table, array_size(at_tcp_state_cmd_table));

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
 * @brief GNSS AGPS功能打开
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_agps_open()
{
#define gnss_agps_open_cmd1 "AT+QAGPSCFG=1,\"http://quectel-api1.rx-networks.cn/rxn-api/locationApi/rtcm\",\"wLgWwv6JQt\",\"Quectel\",\"aFltUERDZzZxeTY5cEp2eA==\"\r\n"
#define gnss_agps_open_cmd2 "AT+QAGPS=1\r\n"
    const static core_at_cmd_item_t at_gnss_agps_open_cmd_table[] = {
        {
            /* 配置AGPS的相关参数 */
            .cmd     = gnss_agps_open_cmd1,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_open_cmd1) - 1,
        },
        {
            /* 开启 AGPS 功能 */
            .cmd     = gnss_agps_open_cmd2,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_open_cmd2) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_agps_open_cmd_table, array_size(at_gnss_agps_open_cmd_table));

    return res;
}

/**
 * @brief GNSS AGPS下载星历数据
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_agps_download()
{
#define gnss_agps_download_cmd "AT+QFLST=\"*\"\r\n"
    const static core_at_cmd_item_t at_gnss_agps_download_cmd_table[] = {
        {
            /* 获取星历数据 */
            .cmd     = gnss_agps_download_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_agps_download_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_agps_download_cmd_table, array_size(at_gnss_agps_download_cmd_table));

    return res;
}

/**
 * @brief GNSS 使能通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nema_enable()
{
#define gnss_nema_enable_cmd1 "AT+QGPSCFG=\"gnssnmeatype\",15\r\n"
#define gnss_nema_enable_cmd2 "AT+QGPSCFG=\"nmeasrc\",1\r\n"
    const static core_at_cmd_item_t at_gnss_nema_enable_cmd_table[] = {
        {
            /* 配置多系统混合定位 NMEA 语句的输出类型 */
            .cmd     = gnss_nema_enable_cmd1,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_nema_enable_cmd1) - 1,
        },
        {
            /* 使能通过 AT+QGPSGNME 获取 NMEA 语句 */
            .cmd     = gnss_nema_enable_cmd2,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_nema_enable_cmd2) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nema_enable_cmd_table, array_size(at_gnss_nema_enable_cmd_table));

    return res;
}

/**
 * @brief GNSS 禁用通过 AT+QGPSGNME 获取 NMEA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nema_disable()
{
#define gnss_nema_disable_cmd "AT+QGPSCFG=\"nmeasrc\",0\r\n"
    const static core_at_cmd_item_t at_gnss_nema_disable_cmd_table[] = {
        {
            /* 禁用通过 AT+QGPSGNME 获取 NMEA 语句 */
            .cmd     = gnss_nema_disable_cmd,
            .rsp     = "OK",
            .cmd_len = strlen(gnss_nema_disable_cmd) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nema_disable_cmd_table, array_size(at_gnss_nema_disable_cmd_table));

    return res;
}

/**
 * @brief GNSS 获取 GGA 语句
 *
 * @return int32_t
 */
int32_t ec800m_at_gnss_nema_query()
{
#define gnss_nema_query_cmd1 "AT+QGPSGNMEA=\"GSA\"\r\n"
#define gnss_nema_query_cmd2 "AT+QGPSGNMEA=\"GGA\"\r\n"
#define gnss_nema_query_cmd3 "AT+QGPSGNMEA=\"RMC\"\r\n"
#define gnss_nema_query_cmd4 "AT+QGPSGNMEA=\"VTG\"\r\n"

    const static core_at_cmd_item_t at_gnss_nema_query_cmd_table[] = {
        {
            /* 获取 GSA 语句 */
            .cmd        = gnss_nema_query_cmd1,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nema_query_cmd1) - 1,
        },
        {
            /* 获取 GGA 语句 */
            .cmd        = gnss_nema_query_cmd2,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nema_query_cmd2) - 1,
        },
        {
            /* 获取 RMC 语句 */
            .cmd        = gnss_nema_query_cmd3,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nema_query_cmd3) - 1,
        },
        {
            /* 获取 VTG 语句 */
            .cmd        = gnss_nema_query_cmd4,
            .handler    = gnss_nmea_rsp_handler,
            .timeout_ms = 500,
            .rsp        = "OK",
            .cmd_len    = strlen(gnss_nema_query_cmd4) - 1,
        },
    };

    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_gnss_nema_query_cmd_table, array_size(at_gnss_nema_query_cmd_table));

    return res;
}

/// @brief 关闭TCP连接
/// @param
/// @return
int32_t ec800m_at_tcp_close(void)
{
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    char cmd_buf[AIOT_AT_CMD_LEN_MAXIMUM] = {0};
    // 此处默认关闭connectID=1的TCP连接
    snprintf(cmd_buf, sizeof(cmd_buf), at_handle.device->close_cmd[0].fmt, 1);
    at_handle.device->close_cmd[0].cmd     = cmd_buf;
    at_handle.device->close_cmd[0].cmd_len = strlen(at_handle.device->close_cmd[0].cmd) - 1;
    res                                    = core_at_commands_send_sync(at_handle.device->close_cmd, at_handle.device->close_cmd_size);

    return res;
}

/********************************************************************
 * 命令回调函数定义
 *********************************************************************/

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

/* 4G模块场景状态和TCP状态说明：
      1. 若TBOX为冷启动（上电启动）：此时4G模块无场景状态
      2. 若TBOX主动重启，且主动关闭4G模块：此时4G模块无场景状态
      3. 若TBOX主动重启，且关闭TCP连接：此时有4G模块场景，无TCP连接状态
      4. 若TBOX异常重启（或者按复位按钮重启）：此时有4G模块场景，有TCP连接状态
    */

/// @brief 查询场景信息回调函数
static at_rsp_result_t context_state_rsp_handler(char *rsp)
{
    // 1.查询场景激活反馈
    // +QIACT: 1,1,1,"10.205.121.24"
    // +QIACT: 1,<context_state>,<context_type>[,<IPv4_address>][,<IPv6_address>]
    // <context_state> 整型。场景状态。0 去激活 1 激活
    // <context_type> 整型。协议类型。 1 IPv4 2 IPv6 3 IPv4v6
    // <IPv4_address> 字符串类型。场景激活后的本地 IPv4 地址。
    // <IPv6_address> 字符串类型。场景激活后的本地 IPv6 地址。
    char *line = NULL;
    line       = strstr(rsp, "+QIACT");
    LOGD(TAG, "%s", rsp);
    if (line != NULL) {
        int context_type;
        char IPv4_address[127] = {0};
        if (sscanf(line, "+QIACT: %d,%d,%d,%s\r\n", &tcp_context_id, &tcp_context_state, &context_type, IPv4_address)) {
            LOGI(TAG, "contextID=%d", tcp_context_id);
            LOGI(TAG, "context_state=%d", tcp_context_state);
            LOGI(TAG, "context_type=%d", context_type);
            LOGI(TAG, "IPv4_address=%s", IPv4_address);
        } else {
            LOGE(TAG, "format error (%s)", line);
        }
    }

    return AT_RSP_SUCCESS;
}

/// @brief 查询TCP状态回调函数
static at_rsp_result_t tcp_state_rsp_handler(char *rsp)
{
    char *line = NULL;
    line       = strstr(rsp, "+QISTATE");
    // LOGI(TAG, "%s", rsp);
    if (line != NULL) {
        line = strtok(line, "\r\n");
        LOGD(TAG, "line=%s", line);
        if (line) {
            // int connectID;
            char service_type[10];
            char IP_address[128];
            int remote_port;
            int local_port;
            // int socket_state; /*Socket 服务状态。
            //                     0 "Initial"：尚未建立连接
            //                     1 "Opening"：客户端正在连接或者服务器正尝试监听
            //                     2 "Connected"：客户端连接已建立
            //                     3 "Listening"：服务器正在监听
            //                     4 "Closing"：连接断开*/
            int contextID; // 场景 ID
            int serverID;
            int access_mode; // 数据访问模式。
            char AT_port[10];
            // +QISTATE: 1,"TCP","a17S6soVoCJ.iot-as-mqtt.cn-shanghai.aliyuncs.com",443,0,2,1,1,1,"uart1"
            if (sscanf(line,
                       "%*s%d,%*[\"]%[^\"]%*[\"]%*[,]%*[\"]%[^\"]%*[\"],%d,%d,%d,%d,%d,%d%*[,]%*[\"]%[^\"]%*[\"]",
                       &tcp_connect_id, service_type, IP_address, &remote_port, &local_port, &tcp_connect_state, &contextID, &serverID, &access_mode, AT_port)) {
                // printf("connectID=%d\r\n", connectID);
                // printf("service_type=%s\r\n", service_type);
                // printf("IP_address=%s\r\n", IP_address);
                // printf("remote_port=%d\r\n", remote_port);
                // printf("local_port=%d\r\n", local_port);
                // printf("socket_state=%d\r\n", socket_state);
                // printf("contextID=%d\r\n", contextID);
                // printf("serverID=%d\r\n", serverID);
                // printf("access_mode=%d\r\n", access_mode);
                // printf("AT_port=%s\r\n", AT_port);

                // if (contextID == 1 && socket_state == 2) // 场景1TCP是连接状态
                // {
                //     tcp_connectID = connectID; // 记录用于主动关闭TCP连接用
                // }
            } else {
                LOGE(TAG, "format error (%s)", line);
            }
        }
    }

    return AT_RSP_SUCCESS;
}

//------------------------------ end ------------------------------

#if 0
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {
        /* 获取SIM卡IMSI */
        .cmd = "AT+CIMI\r\n",
        .rsp = "OK",
    },
    {
        /* 获取SIM卡IMSI */
        .cmd = "AT+QCCID\r\n",
        .rsp = "OK",
    },
    {
        /* 检查信号强度 */
        .cmd = "AT+CSQ\r\n",
        .rsp = "OK",
    },
#if 0
    {
        .cmd = "AT+CREG?\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+CEREG?\r\n",
        .rsp = "OK",
    },
#endif
    // {
    //     /* 设置身份认证参数 */
    //     // AT+QICSGP 配置 TCP/IP 场景参数
    //     // 该命令用于配置<APN>、<username>和<password>以及其他 TCP/IP 场景参数。
    //     // AT+QICSGP=<contextID>[,<context_type>,<APN>[,<username>,<password>)[,<authentication>]]]
    //     // <contextID> 整型。场景 ID。范围：1~7。
    //     // <context_type> 整型。协议类型。1 IPv4 2 IPv6 3 IPv4v6
    //     // <APN> 字符串类型。接入点名称。
    //     // <username> 字符串类型。用户名。最大长度：127 字节。
    //     // <password> 字符串类型。密码。最大长度：127 字节。
    //     // <authentication> 整型。APN 鉴权方式。0 None 1 PAP 2 CHAP
    //     .cmd = g_ec200_apn_cmd, //"AT+QICSGP=1,1,\"cmiot\",\"\",\"\",1\r\n",
    //     .rsp = "OK",
    // },
    // {   /* 去场景激活 */
    //     .cmd = "AT+QIDEACT=1\r\n",
    //     .rsp = "OK",
    // },
    {
        /* 场景激活 */
        .cmd        = "AT+QIACT=1\r\n",
        .rsp        = "OK",
        .timeout_ms = 150000,
    },
    {
        /* 查询场景激活 */
        .cmd = "AT+QIACT?\r\n",
        .rsp = "OK",
    },
    // {
    //     /* 配置DNS */
    //     .cmd = "AT+QIDNSCFG=1\r\n",
    //     .rsp = "OK",
    // },
    // {
    //     /* 配置DNS */
    //     .cmd = "AT+QIDNSCFG=1,\"114.114.114.114\",\"8.8.8.8\"\r\n",
    //     .rsp = "OK",
    // },
    //     {
    //     /* 配置DNS */
    //     .cmd = "AT+QIDNSCFG=1\r\n",
    //     .rsp = "OK",
    // },
    // {
    //     /* 查询DNS */
    //     .cmd = "AT+QIDNSGIP=1,\"a17S6soVoCJ.iot-as-mqtt.cn-shanghai.aliyuncs.com\"\r\n",
    //     .rsp = "OK",
    // },
    // {
    //     /* ping DNS */
    //     .cmd = "AT+QPING=1,\"114.114.114.114\"\r\n",
    //     .rsp = "OK",
    // },
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {
        /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        .fmt        = "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,1\r\n",
        .rsp        = "+QIOPEN",
        .timeout_ms = 150 * 1000,
    },
};

/* 发送数据AT命令表 */
static core_at_cmd_item_t at_send_cmd_table[] =
    {
        {
            .fmt        = "AT+QISEND=%d,%d\r\n",
            .rsp        = "",
            .timeout_ms = 2000,
        },
        // {
        //     /* 纯数据，没有格式*/
        //     .rsp = "",
        // },
        {
            .cmd        = "AT+QIGETERROR\r\n",
            .rsp        = "OK",
            .timeout_ms = 2000,
        },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {
        /* 建立TCP连接 */
        .fmt = "AT+QICLOSE=%d\r\n",
        .rsp = "OK",
    }};

static core_at_recv_data_prefix at_recv = {
    .prefix = "+QIURC: \"recv\"",
    .fmt    = "+QIURC: \"recv\",%d,%d\r\n",
};

/* 4G模块关机 */
static core_at_cmd_item_t at_poweroff_cmd_table[] = {
    {
        .cmd = "AT+QPOWD=1\r\n",
        .rsp = "OK",
    },
};

/* GNSS模块命令表 */
static core_at_cmd_item_t at_gnss_open_cmd_table[] = {
    {
        /* 打开GNSS */
        .cmd = "AT+QGPS=1\r\n",
        .rsp = "OK",
    },
};

static core_at_cmd_item_t at_gnss_state_cmd_table[] = {
    {
        /* 查询GNSS状态 */
        .cmd        = "AT+QGPS?\r\n",
        .rsp        = "OK",
        .timeout_ms = 2000,
        .handler    = gnss_state_rsp_handler,
    },
};

static core_at_cmd_item_t at_gnss_location_cmd_table[] = {
    {/* 获取位置 */
     .cmd        = "AT+QGPSLOC=2\r\n",
     .rsp        = "OK",
     .handler    = gnss_rsp_handler,
     .timeout_ms = 2000},
};

static core_at_cmd_item_t at_gnss_close_cmd_table[] = {
    {
        /* 关闭GNSS */
        .cmd = "AT+QGPSEND\r\n",
        .rsp = "OK",
    },
};

static core_at_cmd_item_t at_gnss_agps_open_cmd_table[] = {
    {
        /* 配置AGPS的相关参数 */
        .cmd = "AT+QAGPSCFG=1,\"http://quectel-api1.rx-networks.cn/rxn-api/locationApi/rtcm\",\"wLgWwv6JQt\",\"Quectel\",\"aFltUERDZzZxeTY5cEp2eA==\"\r\n",
        .rsp = "OK",
    },
    {
        /* 开启 AGPS 功能 */
        .cmd = "AT+QAGPS=1\r\n",
        .rsp = "OK",

    },
};

static core_at_cmd_item_t at_gnss_agps_download_cmd_table[] = {
    {
        /* 获取星历数据 */
        .cmd = "AT+QFLST=\"*\"\r\n",
        .rsp = "OK",
    },
};

static core_at_cmd_item_t at_gnss_nema_enable_cmd_table[] = {
    {
        /* 配置多系统混合定位 NMEA 语句的输出类型 */
        .cmd = "AT+QGPSCFG=\"gnssnmeatype\",15\r\n",
        .rsp = "OK",
    },
    {
        /* 使能通过 AT+QGPSGNME 获取 NMEA 语句 */
        .cmd = "AT+QGPSCFG=\"nmeasrc\",1\r\n",
        .rsp = "OK",
    },
};

static core_at_cmd_item_t at_gnss_nema_disable_cmd_table[] = {
    {
        /* 禁用通过 AT+QGPSGNME 获取 NMEA 语句 */
        .cmd = "AT+QGPSCFG=\"nmeasrc\",0\r\n",
        .rsp = "OK",
    },
};

static core_at_cmd_item_t at_gnss_nema_query_cmd_table[] = {
    {
        /* 获取 GSA 语句 */
        .cmd        = "AT+QGPSGNMEA=\"GSA\"\r\n",
        .handler    = gnss_nmea_rsp_handler,
        .timeout_ms = 500,
        .rsp        = "OK",
    },
    {
        /* 获取 GGA 语句 */
        .cmd        = "AT+QGPSGNMEA=\"GGA\"\r\n",
        .handler    = gnss_nmea_rsp_handler,
        .timeout_ms = 500,
        .rsp        = "OK",
    },
    {
        /* 获取 RMC 语句 */
        .cmd        = "AT+QGPSGNMEA=\"RMC\"\r\n",
        .handler    = gnss_nmea_rsp_handler,
        .timeout_ms = 500,
        .rsp        = "OK",
    },
    {
        /* 获取 VTG 语句 */
        .cmd        = "AT+QGPSGNMEA=\"VTG\"\r\n",
        .handler    = gnss_nmea_rsp_handler,
        .timeout_ms = 500,
        .rsp        = "OK",
    },
};

/* 查询场景激活的命令 */
static core_at_cmd_item_t at_context_state_cmd_table[] = {
    {.cmd = "AT+QIACT?\r\n",
     //  .rsp = "OK",
     .handler    = context_state_rsp_handler,
     .timeout_ms = 200},
};

/* 查询Socket服务状态-查询场景1下的连接状态 */
static core_at_cmd_item_t at_tcp_state_cmd_table[] = {
    {.cmd = "AT+QISTATE=0,1\r\n",
     //  .rsp = "OK",
     .handler    = tcp_state_rsp_handler,
     .timeout_ms = 200},
};

#endif
