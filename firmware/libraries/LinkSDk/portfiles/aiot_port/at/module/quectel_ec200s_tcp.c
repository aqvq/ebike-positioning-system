#include "aiot_at_api.h"
#include <stdio.h>

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {   /* 设置身份认证参数 */
        .cmd = "AT+QICSGP=1,1,\"UNINET\",\"\",\"\",1\r\n",
        .rsp = "OK",
    },
    {   /* 去场景激活 */
        .cmd = "AT+QIDEACT=1\r\n",
        .rsp = "OK",
    },
    {   /* 场景激活 */
        .cmd = "AT+QIACT=1\r\n",
        .rsp = "OK",
    },
    {   /* 查询场景激活 */
        .cmd = "AT+QIACT?\r\n",
        .rsp = "OK",
    },
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {   /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
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
    {   /* 建立TCP连接 */
        .fmt = "AT+QICLOSE=%d\r\n",
        .rsp = "OK",
    }
};
static core_at_recv_data_prefix at_recv = {
    .prefix = "+QIURC: \"recv\"",
    .fmt = "+QIURC: \"recv\",%d,%d\r\n",
};

at_device_t ec200_at_cmd = {
    .ip_init_cmd = at_ip_init_cmd_table,
    .ip_init_cmd_size = sizeof(at_ip_init_cmd_table) / sizeof(core_at_cmd_item_t),

    .open_cmd = at_connect_cmd_table,
    .open_cmd_size = sizeof(at_connect_cmd_table) / sizeof(core_at_cmd_item_t),

    .send_cmd = at_send_cmd_table,
    .send_cmd_size = sizeof(at_send_cmd_table) / sizeof(core_at_cmd_item_t),

    .close_cmd = at_disconn_cmd_table,
    .close_cmd_size = sizeof(at_disconn_cmd_table) / sizeof(core_at_cmd_item_t),

    .recv = &at_recv,
    .error_prefix = "ERROR",
};
