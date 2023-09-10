#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "os_net_interface.h"
#include "aiot_at_api.h"

/**
 * @brief 建立1个网络会话, 作为MQTT/HTTP等协议的底层承载
 */
static int32_t __at_establish(int type, char *host, int port, int timeout_ms, int *fd_out)
{
    uint8_t fd;

    if(STATE_SUCCESS == aiot_at_nwk_open(&fd))
    {
        *fd_out = fd;
    }
    return aiot_at_nwk_connect(fd, host, port, timeout_ms);
}

/**
 * @brief 从指定的网络会话上读取
 */
static int32_t __at_recv(int fd, uint8_t *buffer, uint32_t len, uint32_t timeout_ms, core_sysdep_addr_t *addr) {
    return aiot_at_nwk_recv(fd, buffer, len, timeout_ms);
}

/**
 * @brief 在指定的网络会话上发送
 */
static int32_t __at_send(int fd, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                         core_sysdep_addr_t *addr) {
    return aiot_at_nwk_send(fd, buffer, len, timeout_ms);
}
/**
 * @brief 销毁1个网络会话
 */
static int32_t __at_close(int fd) {
    return aiot_at_nwk_close(fd);
}

aiot_net_al_t  g_aiot_net_at_api = {
    .establish = __at_establish,
    .recv = __at_recv,
    .send = __at_send,
    .close = __at_close,
};


