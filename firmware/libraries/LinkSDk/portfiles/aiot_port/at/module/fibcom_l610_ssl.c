#include "aiot_at_api.h"
#include <stdio.h>

/* 网络初始化命令表 */
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
        /* 最后一个参数为2才会使用ssl功能*/
        .fmt = "AT+MIPOPEN=%d,,\"%s\",%d,2\r\n",
        .rsp = "+MIPOPEN",
    },
};

/*数据发送命令列表*/
static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+MIPSEND=%d,%d\r\n",
        .rsp = ">",
    },
    {
        /*.fmt 为纯数据，没有格式*/
        .rsp = "+MIPSEND",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {
        .fmt = "AT+MIPCLOSE=%d\r\n",
        .rsp = "+MIPCLOSE:",
    }
};

/* socket状态变化上报处理回调函数*/
static void socket_status_urc_handle(char *line)
{
    int socket_id = 0, status = 0;
    if(sscanf(line, "+MIPSTAT: %d,%d\r\n", &socket_id, &status)) {
        printf("URC id %d, status %d\r\n", socket_id, status);
        if(status == 2) {
            /* 通知at模块socket网络已经发生变化*/
            core_at_socket_status(socket_id, CORE_AT_LINK_DISCONN);
        }
    }
    else if(sscanf(line, "+MIPCLOSED: %d,%d\r\n", &socket_id, &status)) {
        printf("URC id %d, status %d\r\n", socket_id, status);
        core_at_socket_status(socket_id, CORE_AT_LINK_DISCONN);
    }
}

/* 主动上报命令订阅处理 */
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

/* 设置ssl命令表 */
static core_at_cmd_item_t at_ssl_cmd_table[] = {
    {
        .cmd = "AT+GTSSLMODE=1\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+GTSSLVER=4\r\n",
        .rsp = "OK",
    },
    {
        .fmt = "AT+GTSSLFILE=\"TRUSTFILE\",%d\r\n",
        .rsp = ">",
    },
    {
        .rsp = "OK",
    },
    {
        .cmd = "AT+GTSSLFILE?\r\n",
        .rsp = "OK",
    },
};

/* 模组主动上报数据命令标识符 */
static core_at_recv_data_prefix at_recv = {
    .prefix = "+MIPRTCP",
    .fmt = "+MIPRTCP: %d,%d,",
};

/* 结构化描述AT模组 */
at_device_t l610_at_cmd_ssl = {
    .ip_init_cmd = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .send_cmd = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),

    .ssl_cmd = at_ssl_cmd_table,
    .ssl_cmd_size = sizeof(at_ssl_cmd_table) / sizeof(core_at_cmd_item_t),

    .recv = &at_recv,

    .urc_register = at_urc_register_table,
    .urc_register_size = sizeof(at_urc_register_table) / sizeof(core_at_urc_item_t),
    .error_prefix = "ERROR",
};
