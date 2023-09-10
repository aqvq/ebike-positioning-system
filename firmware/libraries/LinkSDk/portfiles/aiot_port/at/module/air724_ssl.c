#include "aiot_at_api.h"
#include <stdio.h>

/* 模块初始化命令表 */
static core_at_cmd_item_t at_ip_init_cmd_table[] = {
    {   /* 手动网络附着 */
        .cmd = "AT+CGATT=1\r\n",
        .rsp = "OK",
    },
    {   /* 设置APN */
        .cmd = "AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n",
        .rsp = "OK",
    },
    {   /* 先关闭TCP连接 */
        .cmd = "AT+CIPSHUT\r\n",
        .rsp = "SHUT OK",
    },
    {   /* 打开TCP多连接 */
        .cmd = "AT+CIPMUX=1\r\n",
        .rsp = "OK",
    },
    {   /* 设置快速发送 */
        .cmd = "AT+CIPQSEND=1\r\n",
        .rsp = "OK",
    },
    {   /* 启动任务 */
        .cmd = "AT+CSTT\r\n",
        .rsp = "OK",
    },
    {   /* 激活移动场景,发起GPRS/CSD无线连接 */
        .cmd = "AT+CIICR\r\n",
        .rsp = "OK",
    },
    {   /* 查询本地IP地址 */
        .cmd = "AT+CIFSR\r\n",
        .rsp = ".",
    },
    {   /* 2 发送报文显示'>', "SEND OK"/"DATA ACCEPT" */
        .cmd = "AT+CIPSPRT=1\r\n",
        .rsp = "OK",
    },
};

/* TCP建立连接AT命令表 */
static core_at_cmd_item_t at_connect_cmd_table[] = {
    {   /* 建立TCP连接, TODO: aiot_at_nwk_connect接口会组织此AT命令 */
        .fmt = "AT+CIPSTART=%d,TCP,%s,%d\r\n",
        .rsp = "CONNECT OK",
    },
};

/* TCP发送数据AT命令表*/
static core_at_cmd_item_t at_send_cmd_table[] = {
    {
        .fmt = "AT+CIPSEND=%d,%d\r\n",
        .rsp = ">",    /* 收到`>`字符后做短延时处理, N720模组要求 */
    },
    {
        /*.fmt 为纯数据，没有格式*/
        .rsp = "DATA ACCEPT",
    },
};

/* TCP关闭连接AT命令表 */
static core_at_cmd_item_t at_disconn_cmd_table[] = {
    {   /* 建立TCP连接 */
        .fmt = "AT+CIPCLOSE=%d\r\n",
        .rsp = "CLOSE OK",
    }
};

/* 设置ssl命令表*/
static core_at_cmd_item_t at_ssl_cmd_table[] = {
    {
        .cmd = "AT+SSLCFG=\"sslversion\",1,1\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+SSLCFG=\"ciphersuite\",1,0X0035\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+SSLCFG=\"cacert\",1,cacert.pem\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+FSCREATE=cacert.pem\r\n",
        .rsp = "OK",
    },
    {
        .fmt = "AT+FSWRITE=cacert.pem,0,%d,10\r\n",
        .rsp = ">",
    },
    {
        .rsp = "OK",
    },
    {
        .cmd = "AT+CIPSSL=1\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+SSLCFG=\"seclevel\",0,2\r\n",
        .rsp = "OK",
    },
    {
        .cmd = "AT+SSLCFG=\"clientrandom\",0,101B12C3141516171F19202122232425262728293031323334353637\r\n",
        .rsp = "OK",
    },
};

/* 数据接收识别头 */
static core_at_recv_data_prefix at_recv = {
    .prefix = "+RECEIVE",
    .fmt = "+RECEIVE,%d,%d:\r\n",
};

/* 定义air724设备，支持ssl功能*/
at_device_t air724_at_cmd_ssl = {
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
