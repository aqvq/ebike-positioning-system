#include "aiot_at_api.h"
#include <stdio.h>

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {   /* 设置APN */
        .cmd = "AT+CGDCONT=1,\"IP\",\"UNINET\"\r\n",
        .rsp = "OK",
    },
    {   /* 查询运营商 */
        .cmd = "AT+COPS?\r\n",
        .rsp = "OK",
    },
    {   /* 手动网络附着 */
        .cmd = "AT+CGREG?\r\n",
        .rsp = "+CGREG: 0,1",
    },
    {   /* 去场景激活 */
        .cmd = "AT+MIPCALL=0\r\n",
        .rsp = "\r\n",
    },
    {   /* 场景激活 */
        .cmd = "AT+MIPCALL?\r\n",
        .rsp = "+MIPCALL",
    },
    {   /* 场景激活 */
        .cmd = "AT+MIPCALL=1\r\n",
        .rsp = "+MIPCALL",
    },
    {   /* 设置数据收发格式 */
        .cmd = "AT+GTSET=\"IPRFMT\",2\r\n",
        .rsp = "OK",
    },
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {   /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        /* %d = socket_id
        *  %s = ip/host
        *  %d = port
        */
        .fmt = "AT+MIPOPEN=%d,,\"%s\",%d,0\r\n",
        .rsp = "+MIPOPEN",
        .timeout_ms = 20000,
    },
};

static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+MIPSEND=%d,%d\r\n",
        .rsp = ">",    /* 收到`>`字符后做短延时处理, N720模组要求 */
    },
    {
        /*.fmt 为纯数据，没有格式*/
        .rsp = "+MIPSEND",
    },
};

static void socket_status_urc_handle(char *line)
{
    int socket_id = 0, status = 0;
    if(sscanf(line, "+MIPSTAT: %d,%d\r\n", &socket_id, &status)) {
        printf("URC id %d, status %d\r\n", socket_id, status);
        if(status == 2) {
            /* 通知AT驱动socket的状态变化*/
            core_at_socket_status(socket_id, CORE_AT_LINK_DISCONN);
        }
    }
    else if(sscanf(line, "+MIPCLOSED: %d,%d\r\n", &socket_id, &status)) {
        printf("URC id %d, status %d\r\n", socket_id, status);
        /* 通知AT驱动socket的状态变化*/
        core_at_socket_status(socket_id, CORE_AT_LINK_DISCONN);
    }
}

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {   /* 建立TCP连接 */
        .fmt = "AT+MIPCLOSE=%d\r\n",
        .rsp = "+MIPCLOSE:",
    }
};


static core_at_urc_item_t at_urc_register_table[] = {
    {
        .prefix = "+MIPSTAT:",
        .handle = socket_status_urc_handle,
    },
    {
        .prefix = "+MIPCLOSED:",
        .handle = socket_status_urc_handle,
    },
};

static core_at_recv_data_prefix at_recv = {
    .prefix = "+MIPRTCP",
    .fmt = "+MIPRTCP: %d,%d,",
};

at_device_t l610_at_cmd = {
    .ip_init_cmd = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .send_cmd = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),

    .recv = &at_recv,

    .urc_register = at_urc_register_table,
    .urc_register_size = sizeof(at_urc_register_table) / sizeof(core_at_urc_item_t),
    .error_prefix = "ERROR",
};
