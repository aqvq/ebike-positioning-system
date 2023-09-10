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
        .fmt = "AT+QSSLOPEN=1,1,%d,\"%s\",%d,1\r\n",
        .rsp = "+QSSLOPEN",
    },
};

static core_at_cmd_item_t at_send_cmd_table[] = {
    {   /* 第一个%d=socket_id, 第二个%d=send_len*/
        .fmt = "AT+QSSLSEND=%d,%d\r\n",
        .rsp = ">",
    },
    {
        /*.fmt 为纯数据，没有格式*/
        .fmt = NULL,
        .cmd = NULL,
        .rsp = "SEND OK",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {   /* 建立TCP连接 */
        .fmt = "AT+QSSLCLOSE=%d\r\n",
        .rsp = "OK",
    }
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_ssl_cmd_table[] = {
    /* 配置ssl参数 */
    {
        .cmd = "AT+QSSLCFG=\"sslversion\",1,1\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+QSSLCFG=\"ciphersuite\",1,0X0035\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+QSSLCFG=\"cacert\",1,\"UFS:cacert.pem\"\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+QSSLCFG=\"seclevel\",1,1\r\n",
        .rsp = "OK",
    },

    /* 写服务器的跟证书*/
    {
        .cmd = "AT+QFOPEN=\"cacert.pem\",1,1\r\n",
        .rsp = "OK",
    },
    {   /*%d=证书的长度*/
        .fmt = "AT+QFWRITE=1,%d\r\n",
        .rsp = "CONNECT",
    },
    {   /*.fmt和.cmd都为NULL,为写证书数据*/
        .fmt = NULL,
        .cmd = NULL,
        .rsp = "+QFWRITE:",
    },
    {
        .cmd = "AT+QFCLOSE=1\r\n",
        .rsp = "OK",
    },
};

static core_at_recv_data_prefix at_recv = {
    .prefix = "+QSSLURC: \"recv\"",
    .fmt = "+QSSLURC: \"recv\",%d,%d\r\n",
};

at_device_t ec200_at_cmd_ssl = {
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
    .error_prefix = "ERROR",
};
