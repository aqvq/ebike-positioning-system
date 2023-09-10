#include "at.h"

#define TAG "AT_TCP"

/* TCP通讯场景ID */
static uint8_t tcp_context_id = 0;

/* TCP通讯场景1状态: 0 去激活 1 激活 */
static uint8_t tcp_context_state = 0;

/* TCP通讯场景1中连接ID */
static uint8_t tcp_connect_id = 0;

/* TCP通讯场景1中连接1TCP状态:
 * 0 "Initial"：尚未建立连接
 * 1 "Opening"：客户端正在连接或者服务器正尝试监听
 * 2 "Connected"：客户端连接已建立
 * 3 "Listening"：服务器正在监听
 * 4 "Closing"：连接断开
 */
static uint8_t tcp_connect_state = 0;

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

/**
 * @brief 场景是否打开
 *
 * @return int32_t < 0 场景关闭或者异常, >=0 场景打开
 */
int32_t ec800m_at_context_state(uint8_t *id, uint8_t *state)
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
    if (id) *id = tcp_context_id;
    if (state) *state = tcp_context_state;
    return res;
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

/**
 * @brief TCP是否打开
 *
 * @return int32_t < 0 TCP关闭或者异常, >=0 TCP打开
 */
int32_t ec800m_at_tcp_state(uint8_t *id, uint8_t *state)
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
    if (id) *id = tcp_connect_id;
    if (state) *state = tcp_connect_state;
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
