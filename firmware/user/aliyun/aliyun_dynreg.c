
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "aiot_state_api.h"
#include "aiot_dynregmq_api.h"
#include "dynregmq_private.h"
#include "log/log.h"
#include "aliyun_config.h"
#include "utils/util.h"
#include "aliyun_dynreg.h"
#include "storage/storage.h"
#include "FreeRTOS.h"
#include "data/device_info.h"
#include "error_type.h"
#include "data/device_info.h"

static const char *TAG = "DYN_REG";

/* 如果要免预注册, 需要将该值设置为1; 如果需要在控制台预先注册设备, 置为0 */
const uint8_t skip_pre_regist = 0;

#if (ALIYUN_VERSION == ALIYUN_VERSION_V1)
static const char nvs_namespce[]       = "device_info";
static const char device_name_key[]    = "device_name";
static const char device_secret_key[]  = "device_secret";
static const char conn_client_id_key[] = "conn_clientid";
static const char conn_username_key[]  = "conn_username";
static const char conn_password_key[]  = "conn_password";
#elif (ALIYUN_VERSION == ALIYUN_VERSION_V2)
static const char nvs_namespce[]       = "dynreg_v2";
static const char device_name_key[]    = "device_name";
static const char device_secret_key[]  = "device_secret";
static const char conn_client_id_key[] = "conn_clientid";
static const char conn_username_key[]  = "conn_username";
static const char conn_password_key[]  = "conn_password";
#endif

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

/* 用户保存白名单模式动态注册, 服务器返回的deviceSecret */
static devinfo_wl_t devinfo_wl = {0};

/* 用户保存免白名单模式动态注册, 服务器返回的mqtt建连信息 */
#if 0
static devinfo_nwl_t devinfo_nwl;
#endif
#if 0
/**
 * @brief 写入设备注册信息到flash
 *
 * @return int8_t < 0: 保存失败, 否则保存成功
 */
int8_t write_device_info()
{
    uint8_t err = 0;

    if (skip_pre_regist == 0) {
        LOGD(TAG, "devinfo_wl.device_name (size: %d): %s\n", strlen(devinfo_wl.device_name), devinfo_wl.device_name);
        err |= storage_write_data(storage_addr_device_name_key, devinfo_wl.device_name, 16);

        LOGD(TAG, "devinfo_wl.device_secret (size: %d): %s\n", strlen(devinfo_wl.device_secret), devinfo_wl.device_secret);
        err |= storage_write_data(storage_addr_device_secret_key, devinfo_wl.device_secret, 64);

    } else if (skip_pre_regist == 1) {
        LOGD(TAG, "devinfo_nwl.conn_clientid (size: %d): %s\n", strlen(devinfo_nwl.conn_clientid), devinfo_nwl.conn_clientid);
        err |= storage_write_data(storage_addr_conn_client_id_key, devinfo_nwl.conn_clientid, strlen(devinfo_nwl.conn_clientid));

        LOGD(TAG, "devinfo_nwl.conn_username (size: %d): %s\n", strlen(devinfo_nwl.conn_username), devinfo_nwl.conn_username);
        err |= storage_write_data(storage_addr_conn_username_key, devinfo_nwl.conn_username, strlen(devinfo_nwl.conn_username));

        LOGD(TAG, "devinfo_nwl.conn_password (size: %d): %s\n", strlen(devinfo_nwl.conn_password), devinfo_nwl.conn_password);
        err |= storage_write_data(storage_addr_conn_password_key, devinfo_nwl.conn_password, strlen(devinfo_nwl.conn_password));
    }

    if (err == 0) {
        LOGD(TAG, "write device info success");
        return 0;
    }

    return -1;
}

int8_t read_device_info()
{
    size_t len  = 0;
    uint8_t err = 0;

    if (skip_pre_regist == 0) {
        len = 16;
        err |= storage_read_data(storage_addr_device_name_key, devinfo_wl.device_name, len);
        LOGD(TAG, "devinfo_wl.device_name (size: %d): %s\n", strlen(devinfo_wl.device_name), devinfo_wl.device_name);
        if (devinfo_wl.device_name[0] == 0 || devinfo_wl.device_name[0] == 0xFF) {
            return -1;
        }
        len = 64;
        err |= storage_read_data(storage_addr_device_secret_key, devinfo_wl.device_secret, len);
        LOGD(TAG, "devinfo_wl.device_secret (size: %d): %s\n", strlen(devinfo_wl.device_secret), devinfo_wl.device_secret);
        if (devinfo_wl.device_secret[0] == 0 || devinfo_wl.device_secret[0] == 0xFF) {
            return -1;
        }

    } else if (skip_pre_regist == 1) {
        err |= storage_read_data(storage_addr_conn_client_id_key, devinfo_nwl.conn_clientid, len);
        LOGD(TAG, "devinfo_nwl.conn_clientid (size: %d): %s\n", strlen(devinfo_nwl.conn_clientid), devinfo_nwl.conn_clientid);
        if (devinfo_nwl.conn_clientid[0] == 0 || devinfo_nwl.conn_clientid[0] == 0xFF) {
            return -1;
        }

        err |= storage_read_data(storage_addr_conn_username_key, devinfo_nwl.conn_username, len);
        LOGD(TAG, "devinfo_nwl.conn_username (size: %d): %s\n", strlen(devinfo_nwl.conn_username), devinfo_nwl.conn_username);
        if (devinfo_nwl.conn_username[0] == 0 || devinfo_nwl.conn_username[0] == 0xFF) {
            return -1;
        }

        err |= storage_read_data(storage_addr_conn_password_key, devinfo_nwl.conn_password, len);
        LOGD(TAG, "devinfo_nwl.conn_password (size: %d): %s\n", strlen(devinfo_nwl.conn_password), devinfo_nwl.conn_password);
        if (devinfo_nwl.conn_password[0] == 0 || devinfo_nwl.conn_password[0] == 0xFF) {
            return -1;
        }
    }

    if (err == 0) {
        LOGD(TAG, "read device info success\n");
        return 0;
    }

    return -2;
}
#endif

uint8_t is_registered()
{
    devinfo_wl_t device;
    if (read_device_info(&device) == OK)
        return device.device_secret[0] != 0 && device.device_secret[0] != 0xFF;
    else
        return 0;
}

char *get_device_secret()
{
    devinfo_wl_t device;
    if (read_device_info(&device) == 0)
        return devinfo_wl.device_secret;
    else
        return NULL;
}

/* TODO: 如果要关闭日志, 就把这个函数实现为空, 如果要减少日志, 可根据code选择不打印
 *
 * 例如: [1580995015.811][LK-040B] > POST /auth/register/device HTTP/1.1
 *
 * 上面这条日志的code就是040B(十六进制), code值的定义见components/dynregmq/aiot_dynregmq_api.h
 *
 */
/* 日志回调函数, SDK的日志会从这里输出 */
static int32_t state_logcb(int32_t code, char *message)
{
    LOGD(TAG, "%s", message);
    LOGD(TAG, "Free heap size: %d", xPortGetFreeHeapSize());
    return 0;
}

/* 数据处理回调, 当SDK从网络上收到dynregmq消息时被调用 */
void dynreg_mq_recv_handler(void *handle, const aiot_dynregmq_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        /* TODO: 回调中需要将packet指向的空间内容复制保存好, 因为回调返回后, 这些空间就会被SDK释放 */
        case AIOT_DYNREGMQRECV_DEVICEINFO_WL: {
            if (strlen(packet->data.deviceinfo_wl.device_secret) >= sizeof(devinfo_wl.device_secret)) {
                break;
            }

            /* 白名单模式, 用户务必要对device_secret进行持久化保存 */
            memset(&devinfo_wl, 0, sizeof(devinfo_wl_t));
            memcpy(devinfo_wl.device_secret, packet->data.deviceinfo_wl.device_secret,
                   strlen(packet->data.deviceinfo_wl.device_secret));
            strcpy(devinfo_wl.device_name, get_device_name());
            write_device_info(&devinfo_wl);
        } break;
/* TODO: 回调中需要将packet指向的空间内容复制保存好, 因为回调返回后, 这些空间就会被SDK释放 */
#if 0
        case AIOT_DYNREGMQRECV_DEVICEINFO_NWL: {
            if (strlen(packet->data.deviceinfo_nwl.clientid) >= sizeof(devinfo_nwl.conn_clientid) ||
                strlen(packet->data.deviceinfo_nwl.username) >= sizeof(devinfo_nwl.conn_username) ||
                strlen(packet->data.deviceinfo_nwl.password) >= sizeof(devinfo_nwl.conn_password)) {
                break;
            }

            /* 免白名单模式, 用户务必要对MQTT的建连信息clientid, conn_username和conn_password进行持久化保存 */
            memset(&devinfo_nwl, 0, sizeof(devinfo_nwl_t));
            memcpy(devinfo_nwl.conn_clientid, packet->data.deviceinfo_nwl.clientid,
                   strlen(packet->data.deviceinfo_nwl.clientid));
            memcpy(devinfo_nwl.conn_username, packet->data.deviceinfo_nwl.username,
                   strlen(packet->data.deviceinfo_nwl.username));
            memcpy(devinfo_nwl.conn_password, packet->data.deviceinfo_nwl.password,
                   strlen(packet->data.deviceinfo_nwl.password));
            write_device_info();
        } break;
#endif
        default: {
        } break;
    }
}

int8_t dynamic_register()
{
    int32_t res           = STATE_SUCCESS;
    void *dynregmq_handle = NULL;
    char host[100]        = {0};
    char *product_key     = PRODUCT_KEY;
    char *product_secret  = PRODUCT_SECRET;

    uint16_t port = 443;             /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */
    char *device_name = get_device_name();

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);

    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option               = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA; /* 使用RSA证书校验DYNREGMQ服务端 */
    cred.max_tls_fragment     = 16384;                               /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled          = 1;                                   /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert     = ali_ca_cert;                         /* 用来验证服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);                 /* 用来验证服务端的RSA根证书长度 */

    /* 创建1个dynregmq客户端实例并内部初始化默认参数 */
    dynregmq_handle = aiot_dynregmq_init();
    if (dynregmq_handle == NULL) {
        LOGE(TAG, "aiot_dynregmq_init failed");
        return -1;
    }

    /* 配置连接的服务器地址 */
#if (ALIYUN_VERSION == ALIYUN_VERSION_V1)
    snprintf(host, 100, "%s.%s", PRODUCT_KEY, MQTT_HOST);
#elif (ALIYUN_VERSION == ALIYUN_VERSION_V2)
    snprintf(host, 100, "%s", MQTT_HOST);
#endif

    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_HOST, (void *)host);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_HOST failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置连接的服务器端口 */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PORT, (void *)&port);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PORT failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置设备productKey */

    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PRODUCT_KEY, (void *)product_key);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PRODUCT_KEY failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置设备productSecret */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_PRODUCT_SECRET, (void *)product_secret);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_PRODUCT_SECRET failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置设备deviceName */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_DEVICE_NAME, (void *)device_name);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_DEVICE_NAME failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_NETWORK_CRED, (void *)&cred);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_NETWORK_CRED failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置DYNREGMQ默认消息接收回调函数 */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_RECV_HANDLER, (void *)dynreg_mq_recv_handler);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_RECV_HANDLER failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 配置DYNREGMQ动态注册模式,
    1. 配置为0则为白名单模式, 用户必须提前在控制台录入deviceName, 动态注册完成后服务会返回deviceSecret, 用户可通过
       AIOT_DYNREGMQRECV_DEVICEINFO_WL类型数据回调获取到deviceSecret.
    2. 配置为1则为免白名单模式, 用户无需提前在控制台录入deviceName, 动态注册完成后服务会返回MQTT建连信息, 用户可通过
       AIOT_DYNREGMQRECV_DEVICEINFO_NWL类型数据回调获取到clientid, conn_username, conn_password. 用户需要将这三个参数通过
       aiot_mqtt_setopt接口以AIOT_MQTTOPT_CLIENTID, AIOT_MQTTOPT_USERNAME, AIOT_MQTTOPT_PASSWORD配置选项
       配置到MQTT句柄中。
    */
    res = aiot_dynregmq_setopt(dynregmq_handle, AIOT_DYNREGMQOPT_NO_WHITELIST, (void *)&skip_pre_regist);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_setopt AIOT_DYNREGMQOPT_NO_WHITELIST failed, res: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    dynregmq_handle_t *handle = (dynregmq_handle_t *)dynregmq_handle;
    LOGW(TAG, "device name: %s, host: %s, product_key: %s, product_secret: %s, port: %d, flag_nowhitelist: %d", handle->device_name, handle->host, handle->product_key, handle->product_secret, handle->port, handle->flag_nowhitelist);

    /* 发送动态注册请求 */
    res = aiot_dynregmq_send_request(dynregmq_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_send_request failed: -0x%04X", -res);
        LOGE(TAG, "please check variables like mqtt_host, product_key, device_name, product_secret");
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 接收动态注册请求 */
    res = aiot_dynregmq_recv(dynregmq_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_recv failed: -0x%04X", -res);
        aiot_dynregmq_deinit(&dynregmq_handle);
        return -1;
    }

    /* 把服务应答中的信息打印出来 */
    if (skip_pre_regist == 0) {
        LOGI(TAG, "device secret: %s", devinfo_wl.device_secret);
    } else {
#if 0
        LOGI(TAG, "clientid: %s", devinfo_nwl.conn_clientid);
        LOGI(TAG, "conn_username: %s", devinfo_nwl.conn_username);
        LOGI(TAG, "conn_password: %s", devinfo_nwl.conn_password);
#endif
    }

    /* 销毁动态注册会话实例 */
    res = aiot_dynregmq_deinit(&dynregmq_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_dynregmq_deinit failed: -0x%04X", -res);
        return -1;
    }

    return 0;
}
