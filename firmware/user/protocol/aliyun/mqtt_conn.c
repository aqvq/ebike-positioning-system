/*
 * 这个例程适用于`Linux`这类支持pthread的POSIX设备, 它演示了用SDK配置MQTT参数并建立连接, 之后创建2个线程
 *
 * + 一个线程用于保活长连接
 * + 一个线程用于接收消息, 并在有消息到达时进入默认的数据回调, 在连接状态变化时进入事件回调
 *
 * 接着在MQTT连接上发送设备端日志到云端, 注意日志上云通道默认是关闭的, 用户需要到云端控制台打开设备端日志服务开关。
 *
 * 需要用户关注或修改的部分, 已经用 TODO 在注释中标明
 *
 */
#include <stdio.h>
#include <string.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "cmsis_os.h"

/* TODO: 用户需要替换为自己的认证信息
 */
#define PRODUCT_KEY 	"a18wPzZJzNG"
#define DEVICE_NAME 	"stm32l4_mbedtls_cat1"
#define DEVICE_SECRET "1888aeb62f4a86aa8922d3f8045865d8"

/* 位于portfiles/aiot_port文件夹下的系统适配函数集合 */
extern aiot_sysdep_portfile_t g_aiot_sysdep_portfile;

/* 位于external/ali_ca_cert.c中的服务器证书 */
extern const char *ali_ca_cert;

static osThreadId g_mqtt_process_thread;
static osThreadId g_mqtt_recv_thread;
static uint8_t g_mqtt_process_thread_running = 0;
static uint8_t g_mqtt_recv_thread_running = 0;

/* 日志回调函数, SDK的日志会从这里输出, 禁止在此函数中调用SDK API */
static int32_t demo_state_logcb(int32_t code, char *message)
{
    printf("%s", message);
    return 0;
}

/* MQTT事件回调函数, 当网络连接/重连/断开时被触发, 事件定义见core/aiot_mqtt_api.h */
void demo_mqtt_event_handler(void *handle, const aiot_mqtt_event_t *event, void *userdata)
{
    switch (event->type) {
    /* SDK因为用户调用了aiot_mqtt_connect()接口, 与mqtt服务器建立连接已成功 */
    case AIOT_MQTTEVT_CONNECT: {
        printf("AIOT_MQTTEVT_CONNECT\r\n");
    }
    break;

    /* SDK因为网络状况被动断连后, 自动发起重连已成功 */
    case AIOT_MQTTEVT_RECONNECT: {
        printf("AIOT_MQTTEVT_RECONNECT\r\n");
    }
    break;

    /* SDK因为网络的状况而被动断开了连接, network是底层读写失败, heartbeat是没有按预期得到服务端心跳应答 */
    case AIOT_MQTTEVT_DISCONNECT: {
        char *cause = (event->data.disconnect == AIOT_MQTTDISCONNEVT_NETWORK_DISCONNECT) ? ("network disconnect") :
                      ("heartbeat disconnect");
        printf("AIOT_MQTTEVT_DISCONNECT: %s\r\n", cause);
    }
    break;

    default: {

    }
    }
}

/* MQTT默认消息处理回调, 当SDK从服务器收到MQTT消息时, 且无对应用户回调处理时被调用 */
void demo_mqtt_default_recv_handler(void *handle, const aiot_mqtt_recv_t *packet, void *userdata)
{
    switch (packet->type) {
    case AIOT_MQTTRECV_HEARTBEAT_RESPONSE: {
        printf("heartbeat response\r\n");
    }
    break;

    case AIOT_MQTTRECV_SUB_ACK: {
        printf("suback, res: -0x%04X, packet id: %d, max qos: %d\r\n",
               -packet->data.sub_ack.res, packet->data.sub_ack.packet_id, packet->data.sub_ack.max_qos);
    }
    break;

    case AIOT_MQTTRECV_PUB: {
        printf("pub, qos: %d, topic: %.*s\r\n", packet->data.pub.qos, packet->data.pub.topic_len, packet->data.pub.topic);
        //printf("pub, payload: %.*s\r\n", packet->data.pub.payload_len, packet->data.pub.payload);
    }
    break;

    case AIOT_MQTTRECV_PUB_ACK: {
        printf("puback, packet id: %d\r\n", packet->data.pub_ack.packet_id);
    }
    break;

    default: {

    }
    }
}

/* 执行aiot_mqtt_process的线程, 包含心跳发送和QoS1消息重发 */
void demo_mqtt_process_thread(void const *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_process_thread_running) {
        res = aiot_mqtt_process((void *)args);
        if (res == STATE_USER_INPUT_EXEC_DISABLED) {
            break;
        }
        osDelay(1000);
    }
}

/* 执行aiot_mqtt_recv的线程, 包含网络自动重连和从服务器收取MQTT消息 */
void demo_mqtt_recv_thread(void const *args)
{
    int32_t res = STATE_SUCCESS;

    while (g_mqtt_recv_thread_running) {
        res = aiot_mqtt_recv((void *)args);
        if (res < STATE_SUCCESS) {
            if (res == STATE_USER_INPUT_EXEC_DISABLED) {
                break;
            }
            osDelay(100);
        } else {
            osDelay(10);
        }
    }
}

int32_t demo_mqtt_start(void **handle, char *product_key, char *device_name, char *device_secret)
{
    int32_t     res = STATE_SUCCESS;
    void       *mqtt_handle = NULL;
    char       *sysinfo = "mid=ec200s,os=freertos,cpu=stm32l476rg";
    char        host[100] = {0}; /* 用这个数组拼接设备连接的云平台站点全地址, 规则是 ${productKey}.iot-as-mqtt.cn-shanghai.aliyuncs.com */

    aiot_sysdep_network_cred_t cred; /* 安全凭据结构体, 如果要用TLS, 这个结构体中配置CA证书等参数 */

    /* 配置SDK的底层依赖 */
    aiot_sysdep_set_portfile(&g_aiot_sysdep_portfile);
    /* 配置SDK的日志输出 */
    aiot_state_set_logcb(demo_state_logcb);

    /* 创建SDK的安全凭据, 用于建立TLS连接 */
    char       *url = "iot-as-mqtt.cn-shanghai.aliyuncs.com"; /* 阿里云平台上海站点的域名后缀 */
    uint16_t    port = 443;      /* 无论设备是否使用TLS连接阿里云平台, 目的端口都是443 */
    memset(&cred, 0, sizeof(aiot_sysdep_network_cred_t));
    cred.option = AIOT_SYSDEP_NETWORK_CRED_SVRCERT_CA;  /* 使用RSA证书校验MQTT服务端 */
    cred.max_tls_fragment = 4096; /* 最大的分片长度为16K, 其它可选值还有4K, 2K, 1K, 0.5K */
    cred.sni_enabled = 1;                               /* TLS建连时, 支持Server Name Indicator */
    cred.x509_server_cert = ali_ca_cert;                 /* 用来验证MQTT服务端的RSA根证书 */
    cred.x509_server_cert_len = strlen(ali_ca_cert);     /* 用来验证MQTT服务端的RSA根证书长度 */


    /* 创建1个MQTT客户端实例并内部初始化默认参数 */
    mqtt_handle = aiot_mqtt_init();
    if (mqtt_handle == NULL) {
        printf("aiot_mqtt_init failed\r\n");
        return -1;
    }

    snprintf(host, 100, "%s.%s", product_key, url);
    /* 配置MQTT服务器地址 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_HOST, (void *)host);
    /* 配置MQTT服务器端口 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PORT, (void *)&port);
    /* 配置设备productKey */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_PRODUCT_KEY, (void *)product_key);
    /* 配置设备deviceName */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_NAME, (void *)device_name);
    /* 配置设备deviceSecret */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_DEVICE_SECRET, (void *)device_secret);
    /* 配置网络连接的安全凭据, 上面已经创建好了 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_NETWORK_CRED, (void *)&cred);
    /* 配置MQTT默认消息接收回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_RECV_HANDLER, (void *)demo_mqtt_default_recv_handler);
    /* 配置MQTT事件回调函数 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EVENT_HANDLER, (void *)demo_mqtt_event_handler);
    /* 配置设备系统信息,用于网络优化 */
    aiot_mqtt_setopt(mqtt_handle, AIOT_MQTTOPT_EXTEND_CLIENTID, (void *)sysinfo);

    /* 与服务器建立MQTT连接 */
    res = aiot_mqtt_connect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        /* 尝试建立连接失败, 销毁MQTT实例, 回收资源 */
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_connect failed: -0x%04X\r\n", -res);
        return -1;
    }

    /* 创建一个单独的线程, 专用于执行aiot_mqtt_process, 它会自动发送心跳保活, 以及重发QoS1的未应答报文*/
    g_mqtt_process_thread_running = 1;
    osThreadDef(processTask, demo_mqtt_process_thread, osPriorityAboveNormal, 0, 1024);
    g_mqtt_process_thread = osThreadCreate(osThread(processTask), mqtt_handle);
    //res = pthread_create(&g_mqtt_process_thread, NULL, demo_mqtt_process_thread, mqtt_handle);
    if (g_mqtt_process_thread < 0) {
        printf("pthread_create demo_mqtt_process_thread failed: %d\r\n", res);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }


    /* 创建一个单独的线程用于执行aiot_mqtt_recv, 它会循环收取服务器下发的MQTT消息, 并在断线时自动重连 */
    g_mqtt_recv_thread_running = 1;
    osThreadDef(recvTask, demo_mqtt_recv_thread, osPriorityHigh, 0, 1024);
    g_mqtt_recv_thread = osThreadCreate(osThread(recvTask), mqtt_handle);
    //res = pthread_create(&g_mqtt_recv_thread, NULL, demo_mqtt_recv_thread, mqtt_handle);
    if (g_mqtt_recv_thread < 0) {
        printf("pthread_create demo_mqtt_recv_thread failed: %d\r\n", res);
        g_mqtt_recv_thread_running = 0;
        //pthread_join(g_mqtt_process_thread, NULL);
        aiot_mqtt_deinit(&mqtt_handle);
        return -1;
    }

    *handle = mqtt_handle;

    return 0;
}

int32_t demo_mqtt_stop(void **handle)
{
    int32_t res = STATE_SUCCESS;
    void *mqtt_handle = NULL;

    mqtt_handle = *handle;

    g_mqtt_process_thread_running = 0;
    g_mqtt_recv_thread_running = 0;
    //pthread_join(g_mqtt_process_thread, NULL);
    //pthread_join(g_mqtt_recv_thread, NULL);

    /* 断开MQTT连接 */
    res = aiot_mqtt_disconnect(mqtt_handle);
    if (res < STATE_SUCCESS) {
        aiot_mqtt_deinit(&mqtt_handle);
        printf("aiot_mqtt_disconnect failed: -0x%04X\r\n", -res);
        return -1;
    }

    /* 销毁MQTT实例 */
    res = aiot_mqtt_deinit(&mqtt_handle);
    if (res < STATE_SUCCESS) {
        printf("aiot_mqtt_deinit failed: -0x%04X\r\n", -res);
        return -1;
    }

    return 0;
}

extern int32_t at_hal_init(void);
/*LinkSDK测试Demo入口*/
int link_main(int argc, void *argv[])
{
    int32_t     res = STATE_SUCCESS;

    /* TODO: 替换为自己设备的三元组 */
    char *product_key       = PRODUCT_KEY;
    char *device_name       = DEVICE_NAME;
    char *device_secret     = DEVICE_SECRET;
    void *mqtt_handle = NULL;

    /* 硬件AT模组初始化 */
    res = at_hal_init();
    if(res < STATE_SUCCESS) {
        printf("link_main at_hal_init failed\r\n");
        return -1;
    }
    /* 建立MQTT连接, 并开启保活线程和接收线程 */
    res = demo_mqtt_start(&mqtt_handle, product_key, device_name, device_secret);
    if (res < STATE_SUCCESS) {
        printf("link_main mqtt start failed\r\n");
        return -1;
    }

    while (1) {
        osDelay(5000);

        /* 示例：向云平台上报设备属性*/
        char *pub_topic = "/sys/"PRODUCT_KEY"/"DEVICE_NAME"/thing/event/property/post";
        char *pub_payload = "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"LightSwitch\":0}}";

        res = aiot_mqtt_pub(mqtt_handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);
        if (res < 0) {
            printf("aiot_mqtt_sub failed, res: -0x%04X\n", -res);
            return -1;
        }
    }

    /* 销毁MQTT实例, 退出线程, 一般不会运行到这里 */
    res = demo_mqtt_stop(&mqtt_handle);
    if (res < 0) {
        printf("demo_start_stop failed\r\n");
        return -1;
    }

    return 0;
}

