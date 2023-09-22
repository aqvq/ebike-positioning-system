/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 14:42:19
 * @FilePath: \firmware\user\bsp\at\at_basic.c
 * @Description: at基础命令实现
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "at.h"

/**
 * 兼容AC6编译器，可忽略
 */
void _scanf_mbtowc()
{
    ;
}

extern at_rsp_result_t at_csq_handler(char *rsp);

/*模组初始化命令集*/
static core_at_cmd_item_t at_module_init_cmd_table[] = {
    {
        /* UART通道测试 */
        .cmd = "AT\r\n",
        .rsp = "OK",
    },
    {
        /* 关闭回显 */
        .cmd = "ATE0\r\n",
        .rsp = "OK",
    },
    {
        /* 获取模组型号 */
        .cmd = "ATI\r\n",
        .rsp = "OK",
    },
    {
        /* 获取通信模块 IMEI 号 */
        .cmd = "AT+CGSN\r\n",
        .rsp = "OK",
    },
    {
        /* 获取模组固件版本号 */
        .cmd = "AT+CGMR\r\n",
        .rsp = "OK",
    },
    {
        /* 检查SIM卡 */
        .cmd = "AT+CPIN?\r\n",
        .rsp = "OK",
    },
    {
        /* 获取SIM卡IMSI */
        .cmd = "AT+CIMI\r\n",
        .rsp = "OK",
    },
    {
        /* 检查信号强度 */
        .cmd     = "AT+CSQ\r\n",
        .rsp     = "OK",
        .handler = at_csq_handler,
    },
};

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
    .module_init_cmd      = at_module_init_cmd_table,
    .module_init_cmd_size = sizeof(at_module_init_cmd_table) / sizeof(core_at_cmd_item_t),

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
