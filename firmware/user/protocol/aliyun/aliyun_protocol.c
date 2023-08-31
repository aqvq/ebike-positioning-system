/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:37:31
 * @FilePath: \firmware\user\protocol\aliyun\aliyun_protocol.c
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "cJSON.h"
#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "4g/at_api.h"
#include "aiot_ota_api.h"
#include "utils/util.h"
#include "log/log.h"
#include "aliyun_config.h"
#include "aliyun_protocol.h"
#include "aliyun_dynreg.h"
#include "storage/storage.h"
#include "aiot_logpost_api.h"
#include "4g/hal_adapter.h"
#include "main/app_config.h"
#include "protocol/aliyun/aliyun_message_handle.h"
#include "protocol/aliyun/aliyun_shadow.h"

#define RETRY_COUNT (3)

// 关闭EC200模块并重启ESP32, 定义在main中
extern void ec200_poweroff_and_mcu_restart(void);

static const char *TAG = "ALIYUN_PROTOCOL";

// extern int32_t at_hal_init(void);

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running    = 0;

/// @brief 回调函数指针
static iot_receive_callback receive_callback = NULL;

void *mqtt_handle           = NULL; // MQTT会话实例
static void *logpost_handle = NULL; // 日志上传会话实例

static uint8_t sys_log_switch = 1;

void *g_shadow_handle = NULL;

/* 日志回调函数, SDK的日志会从这里输出, 禁止在此函数中调用SDK API */
static int32_t aliyun_state_logcb(int32_t code, char *message)
{
    //  LOGE(TAG, "%s", message);
    // if (strstr(message, "pub") != NULL || strstr(message, "sub") != NULL)
    // {
    //     LOGI(TAG, "%s", message);
    // }

    LOG("%s", message);
    // LOGI(TAG, "Free Heap Size: %d", xPortGetFreeHeapSize());

    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void aliyun_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
        /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
        case AIOT_MQTTEVT_CONNECT: {
            LOGI(TAG, "AIOT_MQTTEVT_CONNECT");
        } break;

        /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
        case AIOT_MQTTEVT_RECONNECT: {
            LOGI(TAG, "AIOT_MQTTEVT_RECONNECT");
        } break;

        /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
        case AIOT_MQTTEVT_DISCONNECT: {
            char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") : ("heartbeat disconnect");
            LOGI(TAG, "AIOT_MQTTEVT_DISCONNECT: %s", cause);
        } break;

        default: {
        }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void aliyun_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
            LOGI(TAG, "heartbeat response");
        } break;

        case AIOT_MQTTRECV_SUB_ACK: {
            LOGI(TAG, "suback, res: -0x%04X, packet id: %d, max qos: %d",
                 -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
        } break;

        case AIOT_MQTTRECV_PUB: {
            if (strstr(packet->data.pub.topic, "sensor_position")) {
                LOGI(TAG, "pub, qos: %d, topic: %.*s", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
                LOGI(TAG, "pub, payload: %.*s", packet->data.pub.payload_len, packet->data.pub.payload);

                if (receive_callback != NULL) {
                    receive_callback((void *)&(packet->data.pub.payload));
                }
            }
        } break;

        case AIOT_MQTTRECV_PUB_ACK: {
            LOGI(TAG, "puback, packet id: %d", packet->data.pub_ack.packet_id);
        } break;

        default: {
        }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void aliyun_mqtt_process_thread(void *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process((void *)args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void aliyun_mqtt_recv_thread(void *args)
{
    int32_t res = STATE_SUCCESS;
    while (g_mqtt_recv_thread_running) {
        // LOGI(TAG, "aiot_mqtt_recv start ... ");
        res = aiot_mqtt_recv((void *)args);
        // LOGW(TAG, "aiot_mqtt_recv: %d", res);

        if (res < STATE_SUCCESS) {
            LOGE(TAG, "aiot_mqtt_recv: %d", res);
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            } else {
                // 物联网平台禁用时
                // 错误代码:STATE_MQTT_CONNACK_FMT_ERROR
                // STATE_MQTT_CONNACK_RCODE_NOT_AUTHORIZED

                // 重启
                LOGE(TAG, "aiot_mqtt_recv error, restart");

                // 若物联网平台禁用，则该日志可能不可以上传
                aliyun_iot_post_log(IOT_LOG_LEVEL_WARN, "ALIYUN", res, "aiot_mqtt_recv error, restart");

                ec200_poweroff_and_mcu_restart();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

/// @brief MQTT连接
/// @param handle MQTT会话实例
/// @param product_key 产品key
/// @param device_name 设备名称
/// @param device_secret 产品secret
/// @return 0-成功 其他-失败
static int32_t aliyun_mqtt_start(void **handle, char *product_key, char *device_name, char *device_secret)
{
    int32_t res       = STATE_SUCCESS;
    void *mqtt_handle = NULL;
    char *sysinfo     = "mid=ec200u-cn,os=freertos,cpu=esp32-c3";
    char host[100]    = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */

    uint16_t port = MQTT_PORT;
    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(aliyun_state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option               = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA; /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment     = 4096;                                /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled          = 1;                                   /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert     = ali_ca_cert;                         /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);                 /* 用来验证MQTT服务端的RSA根证书长度 */

    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        LOGE(TAG, "aiot_mqtt_init failed");
        return -1;
    }

    /* 配置连接的服务器地址 */
#if (ALIYUN_VERSION == ALIYUN_VERSION_V1)
    snprintf(host, 100, "%s.%s", PRODUCT_KEY, MQTT_HOST);
#elif (ALIYUN_VERSION == ALIYUN_VERSION_V2)
    snprintf(host, 100, "%s", MQTT_HOST);
#endif

    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)PRODUCT_KEY);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    // aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)aliyun_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)aliyun_mqtt_event_handler);
    /* 配置设备系统信息,用于网络优化 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EXTEND_CLIENTID, (void *)sysinfo);
    // --------------------------------------------------------------------------------------------
    // 设备影子相关
#if 1
    /* 创建1个shadow客户端实例并内部初始化默认参数 */
    g_shadow_handle = aiot_shadow_init();
    if (g_shadow_handle == NULL) {
        LOGD(TAG, "aiot_shadow_init failed\n");
        return -1;
    }

    /* 配置MQTT实例句柄 */
    aiot_shadow_setopt(g_shadow_handle, AIOT_SHADOWOPT_MQTT_HANDLE, mqtt_handle);
    /* 配置SHADOW默认消息接收回调函数 */
    aiot_shadow_setopt(g_shadow_handle, AIOT_SHADOWOPT_RECV_HANDLER, (void *)demo_shadow_recv_handler);
#endif
    // --------------------------------------------------------------------------------------------
    /* 与服务器建立MQTT连接 */
    uint8_t retry = 0;
    do {
        res = aiot_mqtt_connect(mqtt_handle);
        if (res < STATE_SUCCESS) {
            retry++;
            LOGE(TAG, "aiot_mqtt_connect failed: -0x%04X, retry: %d", -res, retry);
        } else {
            break;
        }
    } while (res < STATE_SUCCESS && retry < RETRY_COUNT);

    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_mqtt_connect failed, restart");
        aiot_mqtt_deinit(&mqtt_handle);
        ec200_poweroff_and_mcu_restart();
        return res;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文*/
    g_mqtt_process_thread_running = 1;
    if (xTaskCreate(aliyun_mqtt_process_thread, "aliyun_mqtt_process_thread", 5120, (void *)mqtt_handle, tskIDLE_PRIORITY, NULL) != pdPASS) {
        LOGE(TAG, "create aliyun_mqtt_process_thread failed");
        g_mqtt_process_thread_running = 0;
        aiot_mqtt_deinit(&mqtt_handle);
        ec200_poweroff_and_mcu_restart();
        return -1;
    }

    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;

    /* 当前任务会触发IDLE任务看门狗，暂时将优先级设置为0解决这个问题!!! */
    //
    if (xTaskCreate(aliyun_mqtt_recv_thread, "aliyun_mqtt_recv_thread", 5120, (void *)mqtt_handle, tskIDLE_PRIORITY, NULL) != pdPASS) {
        LOGE(TAG, "create aliyun_mqtt_recv_thread failed");
        g_mqtt_recv_thread_running = 0;
        aiot_mqtt_deinit(&mqtt_handle);
        ec200_poweroff_and_mcu_restart();
        return -2;
    }

    /* 订阅topic */
    char sub_topic[128] = {0};
    sprintf(sub_topic, "/%s/%s/%s/%s", PRODUCT_KEY, get_device_name(), "user", "sensor_position");
    aiot_mqtt_sub(mqtt_handle, sub_topic, NULL, 1, NULL);

    *handle = mqtt_handle;
    return 0;
}

int32_t aliyun_mqtt_stop(void **handle)
{
    int32_t res       = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running    = 0;

    /* 断开MQTT连接 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        LOGE(TAG, "aiot_mqtt_disconnect failed: -0x%04X", -res);
        return -1;
    }

    /* 销毁MQTT实例 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_mqtt_deinit failed: -0x%04X", -res);
        return -1;
    }

    return 0;
}

/* 事件处理回调, 用户可通过此回调获取日志上报通道的开关状态 */
void aliyun_logpost_event_handler(void *handle, const aiot_logpost_event_t *event, void *userdata)
{
    switch (event->type) {
        /* 日志配置事件, 当设备连云成功或者用户在控制台页面控制日志开关时会收到此事件 */
        case AIOT_LOGPOSTEVT_CONFIG_DATA: {
            sys_log_switch = event->data.config_data.on_off;
            LOGI(TAG, "user log switch state is: %d", event->data.config_data.on_off);
            // LOGD(TAG, "toggle it using the switch in device detail page in https://iot.console.aliyun.com\r\n");
        }
        default:
            break;
    }
}

int32_t aliyun_logpost_init(void)
{
    int32_t res = 0;

    /* 创建1个logpost客户端实例并内部初始化默认参数 */
    logpost_handle = aiot_logpost_init();
    if (logpost_handle == NULL) {
        aliyun_mqtt_stop(&mqtt_handle);
        LOGE(TAG, "aiot_logpost_init failed\r\n");
        return -1;
    }

    /* 配置logpost的系统日志开关, 打开后将上报网络延时信息 */
    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_SYS_LOG, (void *)&sys_log_switch);
    if (res < STATE_SUCCESS) {
        LOGD(TAG, "aiot_logpost_setopt AIOT_LOGPOSTOPT_SYS_LOG failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        aliyun_mqtt_stop(&mqtt_handle);
        return -2;
    }

    /* 配置logpost会话, 把它和MQTT会话的句柄关联起来 */
    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_MQTT_HANDLE, mqtt_handle);
    if (res < STATE_SUCCESS) {
        LOGD(TAG, "aiot_logpost_setopt AIOT_LOGPOSTOPT_MQTT_HANDLE failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        aliyun_mqtt_stop(&mqtt_handle);
        return -3;
    }

    res = aiot_logpost_setopt(logpost_handle, AIOT_LOGPOSTOPT_EVENT_HANDLER, (void *)aliyun_logpost_event_handler);
    if (res < STATE_SUCCESS) {
        LOGD(TAG, "aiot_logpost_setopt AIOT_LOGPOSTOPT_EVENT_HANDLER failed, res: -0x%04X\r\n", -res);
        aiot_logpost_deinit(&logpost_handle);
        aliyun_mqtt_stop(&mqtt_handle);
        return -4;
    }

    return 0;
}

/* 销毁logpost实例 */
int32_t aliyun_logpost_deinit(void)
{
    if (logpost_handle == NULL) {
        return -1;
    }

    int32_t res = aiot_logpost_deinit(&logpost_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_logpost_deinit failed: -0x%04X\r\n", -res);
        return -2;
    }

    return 0;
}

// /* 上报日志到云端 */
// void send_log(void *handle, char *log)
// {
//     int32_t res = 0;
//     aiot_logpost_msg_t msg;

//     memset(&msg, 0, sizeof(aiot_logpost_msg_t));
//     msg.timestamp = 0;                       /* 单位为ms的时间戳, 填写0则SDK将使用当前的时间戳 */
//     msg.loglevel = AIOT_LOGPOST_LEVEL_DEBUG; /* 日志级别 */
//     msg.module_name = "APP";                 /* 日志对应的模块 */
//     msg.code = 200;                          /* 状态码 */
//     msg.msg_id = 0;                          /* 云端下行报文的消息标示符, 若无对应消息可直接填0 */
//     msg.content = log;                       /* 日志内容 */

//     res = aiot_logpost_send(handle, &msg);
//     if (res < 0)
//     {
//         LOGE(TAG, "aiot_logpost_send failed: -0x%04X\r\n", -res);
//     }
// }

int32_t aliyun_iot_connect(iot_receive_callback func)
{
    int32_t res = STATE_SUCCESS;

    receive_callback = func;

    /* 获得设备名称 */
    char *device_name = get_device_name();
    char *device_secret;

    /* 硬件AT模组初始化 */
    // res = at_hal_init();
    // if (res < STATE_SUCCESS) {
    //     LOGE(TAG, "aliyun protocol at_hal_init failed, restart");
    //     ec200_poweroff_and_mcu_restart();
    //     return -1;
    // }

    /* 动态注册 */
    if (!is_registered()) {
        int8_t err = dynamic_register();
        int8_t num = 5;
        while (err != 0 && num >= 0) {
            LOGE(TAG, "dynamic register failed, error: %d", err);
            vTaskDelay(pdMS_TO_TICKS(8000));
            err = dynamic_register();
            num--;
        }

        LOGW(TAG, "dynamic register success, reboot");

        ec200_poweroff_and_mcu_restart(); // 成功动态注册，则重启
        return -2;
    }

    /* 获得设备secret */
    devinfo_wl_t device;
    read_device_info(&device);

    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    LOGI(TAG, "%s --- %s --- %s", PRODUCT_KEY, device.device_name, device.device_secret);
    res = aliyun_mqtt_start(&mqtt_handle, PRODUCT_KEY, (char *)device.device_name, device.device_secret);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aliyun protocol mqtt start failed (%d)", res);
        return -3;
    }

    // NTP模块初始化
    res = aliyun_ntp_init(&mqtt_handle);
    if (res != 0) {
        aliyun_mqtt_stop(&mqtt_handle);
        LOGE(TAG, "aliyun_ntp_init failed (%d)", res);
        return -5;
    }

    // OTA升级初始化
    res = aliyun_ota_init(mqtt_handle);
    if (res != 0) {
        aliyun_mqtt_stop(&mqtt_handle);
        LOGE(TAG, "aliyun_ota_init failed (%d)", res);
        return -4;
    }

    // 日志上传模块初始化
    res = aliyun_logpost_init();
    if (res != 0) {
        aliyun_mqtt_stop(&mqtt_handle);
        LOGE(TAG, "aliyun_logpost_init failed (%d)", res);
        return -6;
    }

#if 0
    // 报告当前配置信息
    update_shadow(g_shadow_handle);
#endif

#if 1
    // 设备开机后主动获取设备影子
    res = demo_get_shadow(g_shadow_handle);
    if (res < 0) {
        LOGE(TAG, "demo_get_shadow failed, res = -0x%04x\r\n", -res);
    } else {
        LOGI(TAG, "demo_get_shadow ok");
    }
#endif

    return 0;
}

int32_t aliyun_iot_disconnect(void)
{
    receive_callback = NULL;
    aliyun_ntp_deinit();
    aliyun_ota_deinit();
    aliyun_logpost_deinit();
    return aliyun_mqtt_stop(&mqtt_handle);
}

int32_t aliyun_iot_send(general_message_t *msg)
{
    if (mqtt_handle == NULL || msg == NULL) {
        return -1;
    }
#if 1
    handle_aliyun_message((void *)mqtt_handle, msg);
#endif
    return 0;
}

/// @brief 向Iot平台发送日志
/// @param level 日志级别
/// @param module_name 模块名称
/// @param code 状态码
/// @param content 日志内容
/// @return 0正常 其他异常
int32_t aliyun_iot_post_log(iot_log_level_t level, char *module_name, int32_t code, char *content)
{
    if (logpost_handle == NULL) {
        return -1;
    }

    if (sys_log_switch == 0) {
        LOGW(TAG, "sys_log_switch off");
        return -2;
    }

    int32_t res = 0;
    aiot_logpost_msg_t msg;

    memset(&msg, 0, sizeof(aiot_logpost_msg_t));
    msg.timestamp = 0; /* 单位为ms的时间戳, 填写0则SDK将使用当前的时间戳 */
    switch (level)     /* 日志级别 */
    {
        case IOT_LOG_LEVEL_FATAL:
            msg.loglevel = AIOT_LOGPOST_LEVEL_FATAL;
            break;
        case IOT_LOG_LEVEL_ERROR:
            msg.loglevel = AIOT_LOGPOST_LEVEL_ERR;
            break;
        case IOT_LOG_LEVEL_WARN:
            msg.loglevel = AIOT_LOGPOST_LEVEL_WARN;
            break;
        case IOT_LOG_LEVEL_INFO:
            msg.loglevel = AIOT_LOGPOST_LEVEL_INFO;
            break;
        default:
            msg.loglevel = AIOT_LOGPOST_LEVEL_DEBUG;
            break;
    }
    msg.module_name = module_name; /* 日志对应的模块 */
    msg.code        = code;        /* 状态码 */
    msg.msg_id      = 0;           /* 云端下行报文的消息标示符, 若无对应消息可直接填0 */
    msg.content     = content;     /* 日志内容 */

    res = aiot_logpost_send(logpost_handle, &msg);
    if (res < 0) {
        LOGE(TAG, "aiot_logpost_send failed: -0x%04X", -res);
    }

    return res;
}
