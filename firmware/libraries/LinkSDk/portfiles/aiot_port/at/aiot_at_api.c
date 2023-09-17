/**
 * @file aiot_at_api.c
 * @brief 提供了外部通信模组对接到SDK网络适配层的接口实现
 * @date 2020-01-20
 *
 * @copyright Copyright (C) 2015-2020 Alibaba Group Holding Limited
 *
 * @details
 * 1. 用户需将具体的模组设备结构化数据按照at_device_t格式传进来
 * 2. 本实现提供了基于TCP AT通信指令的网络数据收发能力, 示例代码不区分udp和tcp
 * 3. 支持多条数据链路同时收发的情况
 * 4. 用户应根据应用的实际数据吞吐量合理配置ringbuf大小, ringbuf写入溢出会导致报文不完整, 设备会重新建连
 */

#include "core_stdinc.h"
#include "core_string.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_at_api.h"
#include "core_log.h"
#include "os_net_interface.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

extern aiot_os_al_t  *os_api;
extern const char *ali_ca_cert;

volatile core_at_handle_t at_handle = {
    .is_init = 0,
};

at_rsp_result_t at_csq_handler(char *rsp)
{
    at_rsp_result_t res = AT_RSP_WAITING;
    int rssi = 0, ber = 0;
    char *line = NULL;
    line = strstr(rsp, "+CSQ");
    /*获取信号强度，强度<5返回失败，否则返回成功*/
    if(line != NULL && sscanf(line, "+CSQ: %d,%d\r\n", &rssi, &ber)) {
        if(rssi < 5) {
            res =  AT_RSP_FAILED;
        } else {
            res =  AT_RSP_SUCCESS;
        }
    }

    return res;
}

/*模组初始化命令集*/
core_at_cmd_item_t at_module_init_cmd_table[] = {
    {   /* UART通道测试 */
        .cmd = "AT\r\n",
        .rsp = "OK",
    },
    {   /* 关闭回显 */
        .cmd = "ATE0\r\n",
        .rsp = "OK",
    },
    {   /* 获取模组型号 */
        .cmd = "ATI\r\n",
        .rsp = "OK",
    },
    {   /* 获取通信模块 IMEI 号 */
        .cmd = "AT+CGSN\r\n",
        .rsp = "OK",
    },
    {   /* 获取模组固件版本号 */
        .cmd = "AT+CGMR\r\n",
        .rsp = "OK",
    },
    {   /* 检查SIM卡 */
        .cmd = "AT+CPIN?\r\n",
        .rsp = "OK",
    },
    {   /* 获取SIM卡IMSI */
        .cmd = "AT+CIMI\r\n",
        .rsp = "OK",
    },
    {   /* 检查信号强度 */
        .cmd = "AT+CSQ\r\n",
        .rsp = "OK",
        .handler = at_csq_handler,
    },
};

/*** ringbuf start ***/
int32_t core_ringbuf_init(core_ringbuf_t *rbuf, uint32_t size)
{
    if (rbuf->buf) {
        return STATE_AT_ALREADY_INITED;
    }
    memset(rbuf, 0, sizeof(core_ringbuf_t));

    rbuf->buf = os_api->malloc(size);
    if (NULL == rbuf->buf) {
        return STATE_SYS_DEPEND_MALLOC_FAILED;
    }

    rbuf->head = rbuf->buf;
    rbuf->tail = rbuf->buf;
    rbuf->end = rbuf->buf + size;
    rbuf->size = size;

    return STATE_SUCCESS;
}
void core_ringbuf_reset(core_ringbuf_t *rbuf)
{
    rbuf->tail = rbuf->buf;
    rbuf->head = rbuf->buf;
}

int32_t core_ringbuf_get_occupy(core_ringbuf_t *rbuf)
{
    uint32_t used = 0;
    if(rbuf->buf == NULL) {
        return 0;
    }

    if (rbuf->tail >= rbuf->head) {
        used = rbuf->tail - rbuf->head;
    } else {
        used = rbuf->tail - rbuf->buf + rbuf->end - rbuf->head;
    }

    return used;
}

int32_t core_ringbuf_write(core_ringbuf_t *rbuf, const uint8_t *data, uint32_t len)
{
    if (len > (rbuf->size - core_ringbuf_get_occupy(rbuf))) {
        return STATE_AT_RINGBUF_OVERFLOW;
    }
    if (rbuf->tail + len >= rbuf->end) {
        uint32_t remain_len = rbuf->end - rbuf->tail;

        memcpy(rbuf->tail, data, remain_len);
        memcpy(rbuf->buf, data + remain_len, len - remain_len);
        rbuf->tail = rbuf->buf + len - remain_len;
    } else {
        memcpy(rbuf->tail, data, len);
        rbuf->tail += len;
    }

    return len;
}

int32_t core_ringbuf_read(core_ringbuf_t *rbuf, uint8_t *data, uint32_t len)
{
    int32_t used = core_ringbuf_get_occupy(rbuf);

    if (len > used) {
        return 0;
    }

    if (rbuf->head + len >= rbuf->end) {
        uint32_t remain_len = rbuf->end - rbuf->head;

        memcpy(data, rbuf->head, remain_len);
        memcpy(data + remain_len, rbuf->buf, len - remain_len);
        rbuf->head = rbuf->buf + len - remain_len;
    } else {
        memcpy(data, rbuf->head, len);
        rbuf->head += len;
    }

    return len;
}

void core_ringbuf_deinit(core_ringbuf_t *rbuf)
{
    if (NULL == rbuf) {
        return;
    }

    if (rbuf->buf) {
        os_api->free(rbuf->buf);
    }

    memset(rbuf, 0, sizeof(core_ringbuf_t));
}
/*** ringbuf end ***/

static int32_t core_at_uart_tx(const uint8_t *p_data, uint16_t len, uint32_t timeout)
{
    int32_t res = STATE_SUCCESS;

    if (at_handle.uart_tx_func == NULL) {
        return STATE_AT_UART_TX_FUNC_NULL;
    }

    res = at_handle.uart_tx_func(p_data, len, timeout);

    return res;
}

static at_rsp_result_t core_at_rsp_process_default(const core_at_cmd_item_t *cmd, char *rsp)
{
    at_rsp_result_t res = AT_RSP_WAITING;
    if(cmd == NULL || rsp == NULL) {
        return AT_RSP_FAILED;
    }

    /* 出现期望的数据或者是error时，退出*/
    if (strstr(rsp, cmd->rsp) != NULL) {
        res = AT_RSP_SUCCESS;
    } else if (strstr(rsp, at_handle.device->error_prefix) != NULL) {
        res = AT_RSP_FAILED;
    }

    return res;
}

static int32_t core_at_wait_resp(const core_at_cmd_item_t *cmd)
{
    int32_t res = STATE_AT_GET_RSP_FAILED;
    at_rsp_result_t rsp_result = AT_RSP_SUCCESS;
    char rsp[AIOT_AT_RSP_LEN_MAXIMUM] = {0};
    uint64_t timeout_start = os_api->time();
    uint32_t timeout_ms = 0;
    uint32_t used = 0, total_len = 0;

    if(cmd->timeout_ms == 0) {
        timeout_ms = AIOT_AT_RX_TIMEOUT_DEFAULT;
    } else {
        timeout_ms = cmd->timeout_ms;
    }
    memset(rsp, 0, sizeof(rsp));
    do {
        used = core_ringbuf_get_occupy(&at_handle.rsp_rb);
        if(used <= 0) {
            os_api->sleep(AIOT_AT_RINGBUF_RETRY_INTERVAL);
            continue;
        }

        total_len += core_ringbuf_read(&at_handle.rsp_rb, (uint8_t *)(rsp + total_len), used);
        /* call user handle */
        if (cmd->handler) {
            rsp_result = cmd->handler(rsp);
            if(rsp_result == AT_RSP_SUCCESS) {
                res = STATE_SUCCESS;
                break;
            } else if(rsp_result == AT_RSP_FAILED) {
                res = STATE_AT_GET_RSP_FAILED;
                break;
            }
        }

        /* call default handle*/
        rsp_result = core_at_rsp_process_default(cmd, rsp);
        if(rsp_result == AT_RSP_SUCCESS) {
            res = STATE_SUCCESS;
            break;
        } else if(rsp_result == AT_RSP_FAILED) {
            res = STATE_AT_GET_RSP_FAILED;
            break;
        }

    } while ((os_api->time() - timeout_start) < timeout_ms);

    return res;
}

int32_t core_at_commands_send_sync(const core_at_cmd_item_t *cmd_list, uint16_t cmd_num)
{
    uint16_t i = 0;
    int32_t res = STATE_SUCCESS;
    uint16_t retry_cnt = AIOT_AT_CMD_RETRY_TIME;

    if (NULL == cmd_list || 0 == cmd_num) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    os_api->mutex_lock(at_handle.cmd_mutex);
    core_ringbuf_reset(&at_handle.rsp_rb);
    for (; i < cmd_num; i++) {
        /* 发送AT命令 */
        /*
        core_log_hexdump(-1, 1, (unsigned char *)cmd_list[i].cmd, cmd_list[i].cmd_len);
        */
        at_handle.cmd_content = &cmd_list[i];
        res = core_at_uart_tx((uint8_t *)cmd_list[i].cmd, cmd_list[i].cmd_len, at_handle.tx_timeout);
        if (res != cmd_list[i].cmd_len) {
            res = STATE_AT_UART_TX_FAILED;
            break;
        }
        res = core_at_wait_resp(at_handle.cmd_content);
        if (res < 0) {
            if (--retry_cnt > 0) {
                i--;
                continue;
            } else {
                break;
            }
        }

        retry_cnt = AIOT_AT_CMD_RETRY_TIME;
        at_handle.cmd_content = NULL;
        res = STATE_SUCCESS;
    }
    os_api->mutex_unlock(at_handle.cmd_mutex);
    return res;
}

/**
 * AT moduel API start
 */
int32_t aiot_at_init(void)
{
    int32_t res = STATE_SUCCESS;

    if (at_handle.is_init != 0) {
        return STATE_AT_ALREADY_INITED;
    }

    memset(&at_handle, 0, sizeof(core_at_handle_t));
    at_handle.tx_timeout = AIOT_AT_TX_TIMEOUT_DEFAULT;
    at_handle.cmd_mutex = os_api->mutex_init();
    at_handle.send_mutex = os_api->mutex_init();

    if ((res = core_ringbuf_init(&at_handle.rsp_rb, AIOT_AT_DATA_RB_SIZE_DEFAULT)) < STATE_SUCCESS) {
        return res;
    }

    at_handle.is_init = 1;
    return res;
}

static int32_t core_at_set_device(at_device_t *device)
{
    int32_t res = STATE_SUCCESS;
    int32_t i = 0;

    at_handle.device = device;
    /* 未定义模组初始化命令，使用默认的 */
    if(device->module_init_cmd == NULL) {
        at_handle.device->module_init_cmd = at_module_init_cmd_table;
        at_handle.device->module_init_cmd_size = sizeof(at_module_init_cmd_table) / sizeof(core_at_cmd_item_t);
    }

    for(i = 0; i < at_handle.device->module_init_cmd_size; i++) {
        if(at_handle.device->module_init_cmd[i].cmd != NULL) {
            at_handle.device->module_init_cmd[i].cmd_len = strlen(at_handle.device->module_init_cmd[i].cmd);
        }
    }
    for(i = 0; i < at_handle.device->ip_init_cmd_size; i++) {
        if(at_handle.device->ip_init_cmd[i].cmd != NULL) {
            at_handle.device->ip_init_cmd[i].cmd_len = strlen(at_handle.device->ip_init_cmd[i].cmd);
        }
    }
    for(i = 0; i < at_handle.device->open_cmd_size; i++) {
        if(at_handle.device->open_cmd[i].cmd != NULL) {
            at_handle.device->open_cmd[i].cmd_len = strlen(at_handle.device->open_cmd[i].cmd);
        }
    }
    for(i = 0; i < at_handle.device->send_cmd_size; i++) {
        if(at_handle.device->send_cmd[i].cmd != NULL) {
            at_handle.device->send_cmd[i].cmd_len = strlen(at_handle.device->send_cmd[i].cmd);
        }
    }
    for(i = 0; i < at_handle.device->close_cmd_size; i++) {
        if(at_handle.device->close_cmd[i].cmd != NULL) {
            at_handle.device->close_cmd[i].cmd_len = strlen(at_handle.device->close_cmd[i].cmd);
        }
    }

    for(i = 0; i < at_handle.device->ssl_cmd_size; i++) {
        if(at_handle.device->ssl_cmd[i].cmd != NULL) {
            at_handle.device->ssl_cmd[i].cmd_len = strlen(at_handle.device->ssl_cmd[i].cmd);
        }
    }

    return res;
}

int32_t aiot_at_setopt(aiot_at_option_t opt, void *data)
{
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }
    if (NULL == data) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (opt >= AIOT_ATOPT_MAX) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    switch (opt) {
    case AIOT_ATOPT_UART_TX_FUNC: {
        at_handle.uart_tx_func = (aiot_at_uart_tx_func_t)data;
    }
    break;
    case AIOT_ATOPT_USER_DATA: {
        at_handle.user_data = data;
    }
    break;
    case AIOT_ATOPT_DEVICE: {
        core_at_set_device((at_device_t *)data);
    }
    break;
    default:
        break;
    }

    return STATE_SUCCESS;
}

int32_t aiot_at_bootstrap(void)
{
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    res = core_at_commands_send_sync(at_handle.device->module_init_cmd, at_handle.device->module_init_cmd_size);
    if(STATE_SUCCESS != res) {
        return res;
    }

    res = core_at_commands_send_sync(at_handle.device->ip_init_cmd, at_handle.device->ip_init_cmd_size);
    if(STATE_SUCCESS != res) {
        return res;
    }

    return res;
}

int32_t aiot_at_nwk_open(uint8_t *socket_id)
{
    uint8_t i = 0;
    int32_t res = STATE_SUCCESS;

    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    for (i = 0; i < sizeof(at_handle.fd) / sizeof(at_handle.fd[0]); i++) {
        if (at_handle.fd[i].link_status == CORE_AT_LINK_DISABLE) {
            if ((res = core_ringbuf_init(&at_handle.fd[i].data_rb, AIOT_AT_RSP_RB_SIZE_DEFAULT)) < STATE_SUCCESS) {
                return res;
            }

            *socket_id = i;
            at_handle.fd[i].link_status = CORE_AT_LINK_DISCONN;
            return STATE_SUCCESS;
        }
    }

    return STATE_AT_NO_AVAILABLE_LINK;
}

int32_t aiot_at_set_ssl(uint8_t socket_id, const char *ca_cert)
{
    int32_t res = STATE_SUCCESS, i = 0, data_index = 0;
    char tmp[AIOT_AT_CMD_LEN_MAXIMUM] = {0};


    for(i = 0; i < at_handle.device->ssl_cmd_size; i++) {
        if(at_handle.device->ssl_cmd[i].fmt != NULL) {
            if(strstr(at_handle.device->ssl_cmd[i].fmt, "%d")) {
                memset(tmp, 0, sizeof(tmp));
                snprintf(tmp, sizeof(tmp),at_handle.device->ssl_cmd[i].fmt, strlen(ca_cert));
                at_handle.device->ssl_cmd[i].cmd = tmp;
                at_handle.device->ssl_cmd[i].cmd_len = strlen(tmp);
            }
        } else if(at_handle.device->ssl_cmd[i].fmt == NULL && at_handle.device->ssl_cmd[i].cmd == NULL) {
            at_handle.device->ssl_cmd[i].cmd = (char *)ca_cert;
            at_handle.device->ssl_cmd[i].cmd_len = strlen(ca_cert);
            data_index = i;
        }
    }

    res = core_at_commands_send_sync(at_handle.device->ssl_cmd, at_handle.device->ssl_cmd_size);
    if (res == STATE_SUCCESS) {
        at_handle.fd[socket_id].link_status = CORE_AT_LINK_CONN;
    }
    if(data_index != 0) {
        at_handle.device->ssl_cmd[data_index].cmd = NULL;
    }

    return res;
}

int32_t aiot_at_nwk_connect(uint8_t socket_id, const char *host, uint16_t port, uint32_t timeout)
{
    (void)timeout;      /* 超时时间不由外部指定, 使用AT组件配置的时间 */
    char conn_cmd_open[AIOT_AT_CMD_LEN_MAXIMUM] = {0};
    int32_t res = STATE_SUCCESS;

    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }
    if (socket_id >= sizeof(at_handle.fd) / sizeof(at_handle.fd[0])) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if(at_handle.device->ssl_cmd != NULL) {
        aiot_at_set_ssl(socket_id + AT_SOCKET_ID_START, ali_ca_cert);
    }
    /* 拼装host, port */
    memset(conn_cmd_open, 0, sizeof(conn_cmd_open));

    snprintf(conn_cmd_open, sizeof(conn_cmd_open),at_handle.device->open_cmd[0].fmt, socket_id + AT_SOCKET_ID_START, host, port);
    at_handle.device->open_cmd[0].cmd = conn_cmd_open;
    at_handle.device->open_cmd[0].cmd_len = strlen(conn_cmd_open);
    /* send tcp setup command */
    res = core_at_commands_send_sync(at_handle.device->open_cmd, at_handle.device->open_cmd_size);
    if (res == STATE_SUCCESS) {
        at_handle.fd[socket_id].link_status = CORE_AT_LINK_CONN;
    }

    return res;
}

static int32_t core_at_send_package(uint8_t socket_id, const uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    (void)timeout;          /* 发送超时有at组件内部参数决定 */
    char cmd_buf[AIOT_AT_CMD_LEN_MAXIMUM] = {0};
    int32_t res = STATE_SUCCESS;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }
    if (NULL == buffer) {
        return STATE_USER_INPUT_NULL_POINTER;
    }
    if (0 == len) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (socket_id >= sizeof(at_handle.fd) / sizeof(at_handle.fd[0])) {
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if (at_handle.fd[socket_id].link_status <= CORE_AT_LINK_DISCONN) {
        return STATE_AT_LINK_IS_DISCONN;
    }

    memset(cmd_buf, 0, sizeof(cmd_buf));
    /* 组装'TCP数据发送'AT命令 */
    snprintf(cmd_buf, sizeof(cmd_buf), at_handle.device->send_cmd[0].fmt, socket_id + AT_SOCKET_ID_START, (int)len);
    at_handle.device->send_cmd[0].cmd = cmd_buf;
    at_handle.device->send_cmd[0].cmd_len = strlen(at_handle.device->send_cmd[0].cmd);

    at_handle.device->send_cmd[1].cmd = (char *)buffer;
    at_handle.device->send_cmd[1].cmd_len = len;

    res = core_at_commands_send_sync(at_handle.device->send_cmd, at_handle.device->send_cmd_size);

    if (res >= STATE_SUCCESS) {
        return len;
    }

    return res;
}

int32_t aiot_at_nwk_send(uint8_t socket_id, const uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    int32_t res = 0;
    int32_t ret = 0;
    int32_t slice_len = 0;
    os_api->mutex_lock(at_handle.send_mutex);
    while(len > 0) {
        /* 发送数据长度大于单帧最大长度,分片发送*/
        if(len > AIOT_AT_MAX_PACKAGE_SIZE) {
            slice_len = AIOT_AT_MAX_PACKAGE_SIZE;
        } else {
            slice_len = len;
        }
        len -= slice_len;
        ret = core_at_send_package(socket_id, buffer + res, slice_len, timeout);
        if(ret > 0) {
            res += ret;
        } else {
            os_api->mutex_unlock(at_handle.send_mutex);
            return ret;
        }
    }
    os_api->mutex_unlock(at_handle.send_mutex);
    return res;
}

int32_t aiot_at_nwk_recv(uint8_t socket_id, uint8_t *buffer, uint32_t len, uint32_t timeout_ms)
{
    uint64_t timeout_start = os_api->time();
    int32_t res = STATE_SUCCESS;
    int32_t used = 0;
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }

    if (socket_id >= sizeof(at_handle.fd) / sizeof(at_handle.fd[0])) {
        return STATE_USER_INPUT_OUT_RANGE;
    }

    if (at_handle.fd[socket_id].link_status == CORE_AT_LINK_DISABLE) {
        return STATE_AT_LINK_IS_DISCONN;
    }

    /* 对于已经断开的socket，允许先把数据读走，再返回已断开 */
    do {
        used = core_ringbuf_get_occupy(&at_handle.fd[socket_id].data_rb);
        if(used <= 0) {
            if (at_handle.fd[socket_id].link_status < CORE_AT_LINK_CONN) {
                return STATE_AT_LINK_IS_DISCONN;
            }
            os_api->sleep(AIOT_AT_RINGBUF_RETRY_INTERVAL);
            continue;
        } else if(used <= len - res) {
            res += core_ringbuf_read(&at_handle.fd[socket_id].data_rb, buffer + res, used);
        } else {
            res += core_ringbuf_read(&at_handle.fd[socket_id].data_rb, buffer + res, len - res);
        }

        if (res == len) {
            break;
        }
    } while ((os_api->time() - timeout_start) < timeout_ms);
    return res;
}

int32_t aiot_at_nwk_close(uint8_t socket_id)
{
    (void)socket_id;
    char cmd_buf[AIOT_AT_CMD_LEN_MAXIMUM] = {0};
    if (at_handle.is_init != 1) {
        return STATE_AT_NOT_INITED;
    }
    os_api->mutex_lock(at_handle.send_mutex);
    if(at_handle.fd[socket_id].link_status == CORE_AT_LINK_DISABLE) {
        os_api->mutex_unlock(at_handle.send_mutex);
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if (socket_id >= sizeof(at_handle.fd) / sizeof(at_handle.fd[0])) {
        os_api->mutex_unlock(at_handle.send_mutex);
        return STATE_USER_INPUT_OUT_RANGE;
    }
    if(at_handle.fd[socket_id].link_status == CORE_AT_LINK_CONN) {
        memset(cmd_buf, 0, sizeof(cmd_buf));
        /* 组装'TCP数据发送'AT命令 */
        snprintf(cmd_buf, sizeof(cmd_buf), at_handle.device->close_cmd[0].fmt, socket_id + AT_SOCKET_ID_START);
        at_handle.device->close_cmd[0].cmd = cmd_buf;
        at_handle.device->close_cmd[0].cmd_len = strlen(at_handle.device->close_cmd[0].cmd);
        core_at_commands_send_sync(at_handle.device->close_cmd, at_handle.device->close_cmd_size);
    }

    /* reset ringbuf */
    core_ringbuf_deinit(&at_handle.fd[socket_id].data_rb);

    at_handle.fd[socket_id].link_status = CORE_AT_LINK_DISABLE;
    os_api->mutex_unlock(at_handle.send_mutex);
    return STATE_SUCCESS;
}

static int32_t core_at_recv_prefix_match(const char *data, uint32_t size)
{
    uint32_t res = STATE_SUCCESS;
    uint32_t data_id = 0, data_len = 0, match = 0, body_len = 0;
    char tmpbuf[128];
    const char *data_recv_urc_head = NULL;
    const char *last_line = NULL;
    uint32_t remain_len = 0;

    /*查找数据接收报文头*/
    data_recv_urc_head = strstr(data, at_handle.device->recv->prefix);
    if(data_recv_urc_head == NULL) {
        return res;
    }
    /*查到报文头，但是报文头前面还有数据没有没消费*/
    last_line = strstr(data, "\r\n");
    if(last_line != NULL && last_line < data_recv_urc_head) {
        return res;
    }

    /*查找数据接收报文包体*/
    match = sscanf(data_recv_urc_head, at_handle.device->recv->fmt, &data_id, &data_len);
    if(match != 2) {
        return res;
    }

    remain_len = size - (data_recv_urc_head - data);
    /*查找数据接收报文包尾*/
    body_len = snprintf(tmpbuf, 128, at_handle.device->recv->fmt, data_id, data_len);
    if(body_len > remain_len) {
        return res;
    }

    /*赋值待接收数据描述*/
    at_handle.reader.curr_link_id = data_id - AT_SOCKET_ID_START;
    at_handle.reader.data_len = data_len;
    at_handle.reader.remain_len = data_len;

    /*返回总长度*/
    res = body_len;
    return res;
}

static int32_t core_at_process_line(char *line, uint32_t len)
{
    if(at_handle.cmd_content) {
        core_ringbuf_write(&at_handle.rsp_rb, (uint8_t *)line, len);
    }

    for(int i = 0 ; i < at_handle.device->urc_register_size; i++) {
        if(strstr(line, at_handle.device->urc_register[i].prefix) != NULL) {
            at_handle.device->urc_register[i].handle(line);
            break;
        }
    }

    return STATE_SUCCESS;
}

static int32_t core_at_hal_process(uint8_t *data, uint32_t size)
{
    /*返回消费的数据长度*/
    int32_t res = size;

    if(at_handle.is_init != 1) {
        return res;
    }
    // printf("[UART] size %d, DATA %s\r\n", size, data);
    /*判断是否接收数据没结束*/
    if(at_handle.reader.remain_len != 0) {
        int32_t write_size = 0;
        if (at_handle.reader.remain_len > size) {
            write_size = size;
        } else {
            write_size = at_handle.reader.remain_len;
        }
        res = core_ringbuf_write(&at_handle.fd[at_handle.reader.curr_link_id].data_rb, data, write_size);
        at_handle.reader.remain_len -= write_size;
        // printf("[%d] remain_len %d, data_size %d\r\n", at_handle.reader.curr_link_id, at_handle.reader.remain_len, core_ringbuf_get_occupy(&at_handle.fd[at_handle.reader.curr_link_id].data_rb));
        return write_size;
    }

    /*更新接收缓存*/
    if (size > AIOT_AT_RSP_LEN_MAXIMUM - at_handle.rsp_buf_offset) {
        size = AIOT_AT_RSP_LEN_MAXIMUM - at_handle.rsp_buf_offset;
    }

    memcpy(&at_handle.rsp_buf[at_handle.rsp_buf_offset], data, size);
    at_handle.rsp_buf_offset += size;

    /*判断是否为数据接收*/
    int len = core_at_recv_prefix_match(at_handle.rsp_buf, at_handle.rsp_buf_offset);
    if(len > 0) {
        /*计算新的数据，被消费的长度*/
        len = len - (at_handle.rsp_buf_offset - size);
        printf("id %d, len %d, res %d\r\n", at_handle.reader.curr_link_id, at_handle.reader.data_len, len);
        memset(at_handle.rsp_buf, 0, at_handle.rsp_buf_offset);
        at_handle.rsp_buf_offset = 0;

        return len;
    }

    /*判断是否为一行数据*/
    char *postfix = strstr(at_handle.rsp_buf, "\r\n");
    if(postfix == NULL) {
        postfix = strstr(at_handle.rsp_buf, ">");
    }
    if(postfix != NULL) {
        int32_t line_len = postfix - at_handle.rsp_buf + 2;
        /* 行尾设置为0，避免多行粘包 */
        if(line_len < AIOT_AT_RSP_LEN_MAXIMUM) {
            at_handle.rsp_buf[line_len] = 0;
        }

        core_at_process_line(at_handle.rsp_buf, line_len);
        /*计算消费的数据长度*/
        res =  line_len - (at_handle.rsp_buf_offset - size);
        memset(at_handle.rsp_buf, 0, at_handle.rsp_buf_offset);
        at_handle.rsp_buf_offset = 0;
    }

    return res;
}

int32_t aiot_at_hal_recv_handle(uint8_t *data, uint32_t size)
{
    int32_t recv_size = size;
    uint8_t *pData = data;
    uint32_t used = 0;
    if (NULL == data || 0 == size) {
        return 0;
    }

    while (recv_size > 0) {
        used = core_at_hal_process(pData, recv_size);
        pData += used;
        recv_size -= used;
    }
    return size;
}
int32_t aiot_at_deinit(void)
{
    uint8_t i = 0;

    if (at_handle.is_init == 0) {
        return STATE_SUCCESS;
    }

    os_api->mutex_deinit(&at_handle.cmd_mutex);
    os_api->mutex_deinit(&at_handle.send_mutex);
    for (i = 0; i < sizeof(at_handle.fd) / sizeof(at_handle.fd[0]); i++) {
        core_ringbuf_deinit(&at_handle.fd[i].data_rb);
    }

    core_ringbuf_deinit(&at_handle.rsp_rb);
    memset(&at_handle, 0, sizeof(core_at_handle_t));

    return STATE_SUCCESS;
}

/*模组的状态变化*/
int32_t core_at_notify_ip_status(core_ip_status_t status)
{
    int8_t i = 0;
    if(IP_SUCCESS != status) {
        for (i = 0; i < sizeof(at_handle.fd) / sizeof(at_handle.fd[0]); i++) {
            at_handle.fd[i].link_status = CORE_AT_LINK_DISCONN;
        }
    }

    return STATE_SUCCESS;
}

/*socket的状态变化*/
int32_t core_at_socket_status(uint32_t id, core_at_link_status_t status)
{
    at_handle.fd[id - AT_SOCKET_ID_START].link_status = status;
    return STATE_SUCCESS;
}
