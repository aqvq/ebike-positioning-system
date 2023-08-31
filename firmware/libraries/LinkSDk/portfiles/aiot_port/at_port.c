/*
 * 这个移植示例适用于MCU+模组使用AT指令的方式接入，它实现了移植SDK所需要的接口。
 * 移植接口大体可以分为两类：网络接口(TCP)、系统接口(OS)
 *
 * + 网络接口：TCP/UDP的socket适配，请勿在此实现TLS传输，TLS实现已封装在SDK内部。
 * + 系统接口：包含内存管理、随机数、系统时间、互斥锁等功能移植。
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "os_net_interface.h"


aiot_os_al_t  *os_api = NULL;
aiot_net_al_t *net_api = NULL;

/* socket建联时间默认最大值 */
#define CORE_SYSDEP_DEFAULT_CONNECT_TIMEOUT_MS (10 * 1000)

typedef struct {
    int fd;
    core_sysdep_socket_type_t socket_type;
    char *host;
    char backup_ip[16];
    uint16_t port;
    uint32_t connect_timeout_ms;
} core_network_handle_t;


void aiot_install_os_api(aiot_os_al_t *os)
{
    if(os != NULL) {
        os_api = os;
    }
}
void aiot_install_net_api(aiot_net_al_t *net)
{
    if(net != NULL) {
        net_api = net;
    }
}

void *core_sysdep_malloc(uint32_t size, char *name)
{
    void *res = NULL;
    if(os_api == NULL) {
        return NULL;
    }
    
    res = os_api->malloc(size);
    if(res == NULL) {
        printf("sysdep malloc failed \n");
    }
    return res;
}

void core_sysdep_free(void *ptr)
{
    if(os_api == NULL) {
        return;
    }
    os_api->free(ptr);
}

uint64_t core_sysdep_time(void)
{
    if(os_api == NULL) {
        return 0;
    }
    return os_api->time();
}

void core_sysdep_sleep(uint64_t time_ms)
{
    if(os_api == NULL) {
        return;
    }
    os_api->sleep(time_ms);
}

static void *core_sysdep_network_init(void)
{
    if(os_api == NULL || net_api == NULL) {
        return NULL;
    }
    core_network_handle_t *handle = os_api->malloc(sizeof(core_network_handle_t));
    if (handle == NULL) {
        return NULL;
    }
    memset(handle, 0, sizeof(core_network_handle_t));
    handle->connect_timeout_ms = CORE_SYSDEP_DEFAULT_CONNECT_TIMEOUT_MS;

    return handle;
}

static int32_t core_sysdep_network_setopt(void *handle, core_sysdep_network_option_t option, void *data)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;
    if(os_api == NULL || net_api == NULL) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (handle == NULL || data == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    if (option >= CORE_SYSDEP_NETWORK_MAX) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    switch (option) {
    case CORE_SYSDEP_NETWORK_SOCKET_TYPE: {
        network_handle->socket_type = *(core_sysdep_socket_type_t *)data;
    }
    break;
    case CORE_SYSDEP_NETWORK_HOST: {
        network_handle->host = os_api->malloc(strlen(data) + 1);
        if (network_handle->host == NULL) {
            printf("malloc failed\n");
            return STATE_PORT_MALLOC_FAILED;
        }
        memset(network_handle->host, 0, strlen(data) + 1);
        memcpy(network_handle->host, data, strlen(data));
    }
    break;
    case CORE_SYSDEP_NETWORK_BACKUP_IP: {
        memcpy(network_handle->backup_ip, data, strlen(data));
    }
    break;
    case CORE_SYSDEP_NETWORK_PORT: {
        network_handle->port = *(uint16_t *)data;
    }
    break;
    case CORE_SYSDEP_NETWORK_CONNECT_TIMEOUT_MS: {
        network_handle->connect_timeout_ms = *(uint32_t *)data;
    }
    break;
    default: {
        break;
    }
    }

    return STATE_SUCCESS;
}

static int32_t core_sysdep_network_establish(void *handle)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;
    if(os_api == NULL || net_api == NULL) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }
    if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT) {
        if (network_handle->host == NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        return net_api->establish(network_handle->socket_type, network_handle->host, network_handle->port, network_handle->connect_timeout_ms, &network_handle->fd);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_SERVER) {
        return STATE_PORT_TCP_SERVER_NOT_IMPLEMENT;
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) {
        if (network_handle->host == NULL) {
            return STATE_PORT_MISSING_HOST;
        }
        return net_api->establish(network_handle->socket_type, network_handle->host, network_handle->port, network_handle->connect_timeout_ms, &network_handle->fd);
    } else if (network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_SERVER) {
        return STATE_PORT_UDP_SERVER_NOT_IMPLEMENT;
    }

    printf("unknown nwk type or tcp host absent\n");
    return STATE_PORT_NETWORK_UNKNOWN_SOCKET_TYPE;
}

static int32_t core_sysdep_network_recv(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;
    if(os_api == NULL || net_api == NULL) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }
    return net_api->recv(network_handle->fd, buffer, len, timeout_ms, addr);
}

static int32_t core_sysdep_network_send(void *handle, uint8_t *buffer, uint32_t len, uint32_t timeout_ms,
                                        core_sysdep_addr_t *addr)
{
    core_network_handle_t *network_handle = (core_network_handle_t *)handle;
    if(os_api == NULL || net_api == NULL) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }
    return net_api->send(network_handle->fd, buffer, len, timeout_ms, addr);
}

static int32_t core_sysdep_network_deinit(void **handle)
{
    core_network_handle_t *network_handle = NULL;

    if(os_api == NULL || net_api == NULL) {
        return STATE_PORT_INPUT_OUT_RANGE;
    }

    if (handle == NULL || *handle == NULL) {
        return STATE_PORT_INPUT_NULL_POINTER;
    }

    network_handle = *(core_network_handle_t **)handle;

    /* Shutdown both send and receive operations. */
    if ((network_handle->socket_type == CORE_SYSDEP_SOCKET_TCP_CLIENT ||
            network_handle->socket_type == CORE_SYSDEP_SOCKET_UDP_CLIENT) && network_handle->host != NULL) {
        net_api->close(network_handle->fd);
    }

    if (network_handle->host != NULL) {
        os_api->free(network_handle->host);
        network_handle->host = NULL;
    }

    os_api->free(network_handle);
    *handle = NULL;

    return 0;
}

void core_sysdep_rand(uint8_t *output, uint32_t output_len)
{
    if(os_api == NULL) {
        return;
    }
    os_api->rand(output, output_len);
}

void *core_sysdep_mutex_init(void)
{
    if(os_api == NULL) {
        return NULL;
    }
    return os_api->mutex_init();
}

void core_sysdep_mutex_lock(void *mutex)
{
    if(os_api == NULL) {
        return;
    }
    os_api->mutex_lock(mutex);
}

void core_sysdep_mutex_unlock(void *mutex)
{
    if(os_api == NULL) {
        return;
    }
    os_api->mutex_unlock(mutex);
}

void core_sysdep_mutex_deinit(void **mutex)
{
    if(os_api == NULL) {
        return;
    }
    os_api->mutex_deinit(mutex);
}

aiot_sysdep_portfile_t g_aiot_sysdep_portfile = {
    .core_sysdep_malloc = core_sysdep_malloc,
    .core_sysdep_free = core_sysdep_free,
    .core_sysdep_time = core_sysdep_time,
    .core_sysdep_sleep = core_sysdep_sleep,
    .core_sysdep_network_init = core_sysdep_network_init,
    .core_sysdep_network_setopt = core_sysdep_network_setopt,
    .core_sysdep_network_establish = core_sysdep_network_establish,
    .core_sysdep_network_recv = core_sysdep_network_recv,
    .core_sysdep_network_send = core_sysdep_network_send,
    .core_sysdep_network_deinit = core_sysdep_network_deinit,
    .core_sysdep_rand = core_sysdep_rand,
    .core_sysdep_mutex_init = core_sysdep_mutex_init,
    .core_sysdep_mutex_lock = core_sysdep_mutex_lock,
    .core_sysdep_mutex_unlock = core_sysdep_mutex_unlock,
    .core_sysdep_mutex_deinit = core_sysdep_mutex_deinit,
};

