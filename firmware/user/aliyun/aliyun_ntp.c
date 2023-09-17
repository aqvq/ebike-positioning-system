

#include "aliyun_ntp.h"
#include "aiot_state_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_ntp_api.h"
#include "log/log.h"
#include "utils/util.h"

#define TAG  "ALIYUN_NTP"

static void *ntp_handle = NULL;

/* 事件处理回调,  */
void demo_ntp_event_handler(void *handle, const aiot_ntp_event_t *event, void *userdata)
{
    switch (event->type) {
        case AIOT_NTPEVT_INVALID_RESPONSE: {
            LOGI(TAG, "AIOT_NTPEVT_INVALID_RESPONSE");
        } break;
        case AIOT_NTPEVT_INVALID_TIME_FORMAT: {
            LOGI(TAG, "AIOT_NTPEVT_INVALID_TIME_FORMAT");
        } break;
        default: {
        }
    }
}

/* TODO: 数据处理回调, 当SDK从网络上收到ntp消息时被调用 */
void demo_ntp_recv_handler(void *handle, const aiot_ntp_recv_t *packet, void *userdata)
{
    switch (packet->type) {
        /* TODO: 结构体 aiot_ntp_recv_t{} 中包含当前时区下, 年月日时分秒的数值, 可在这里把它们解析储存起来 */
        case AIOT_NTPRECV_LOCAL_TIME: {
            LOGW(TAG, "local time: %llu, %02d/%02d/%02d-%02d:%02d:%02d:%d\n",
                 (long long unsigned int)packet->data.local_time.timestamp,
                 packet->data.local_time.year,
                 packet->data.local_time.mon,
                 packet->data.local_time.day,
                 packet->data.local_time.hour,
                 packet->data.local_time.min,
                 packet->data.local_time.sec,
                 packet->data.local_time.msec);

            // 设置时间
            int res = set_time(packet->data.local_time.year,
                               packet->data.local_time.mon,
                               packet->data.local_time.day,
                               packet->data.local_time.hour,
                               packet->data.local_time.min,
                               packet->data.local_time.sec);

            if (res != 0) {
                LOGE(TAG, "set time err = %d.", res);
            }
        } break;

        default: {
        }
    }
}

int32_t aliyun_ntp_init(void **handle)
{
    /*
     * 这里用中国所在的东8区演示功能, 因此例程运行时打印的是北京时间
     *
     * TODO: 若是其它时区, 可做相应调整, 如西3区则把8改为-3, time_zone的合理取值是-12到+12的整数
     *
     */
    int8_t time_zone = 8;

    /* 创建1个ntp客户端实例并内部初始化默认参数 */
    ntp_handle = aiot_ntp_init();
    if (ntp_handle == NULL) {
        LOGE(TAG, "aiot_ntp_init failed");
        return -1;
    }

    int32_t res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_MQTT_HANDLE, *handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ntp_setopt AIOT_NTPOPT_MQTT_HANDLE failed, res: -0x%04X", -res);
        aiot_ntp_deinit(&ntp_handle);
        // demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_TIME_ZONE, (int8_t *)&time_zone);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ntp_setopt AIOT_NTPOPT_TIME_ZONE failed, res: -0x%04X", -res);
        aiot_ntp_deinit(&ntp_handle);
        // demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /* TODO: NTP消息回应从云端到达设备时, 会进入此处设置的回调函数 */
    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_RECV_HANDLER, (void *)demo_ntp_recv_handler);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ntp_setopt AIOT_NTPOPT_RECV_HANDLER failed, res: -0x%04X", -res);
        aiot_ntp_deinit(&ntp_handle);
        // demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    res = aiot_ntp_setopt(ntp_handle, AIOT_NTPOPT_EVENT_HANDLER, (void *)demo_ntp_event_handler);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ntp_setopt AIOT_NTPOPT_EVENT_HANDLER failed, res: -0x%04X", -res);
        aiot_ntp_deinit(&ntp_handle);
        // demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    /* 发送NTP查询请求给云平台 */
    res = aiot_ntp_send_request(ntp_handle);
    if (res < STATE_SUCCESS) {
        aiot_ntp_deinit(&ntp_handle);
        // demo_mqtt_stop(&mqtt_handle);
        return -1;
    }

    return 0;
}

int32_t aliyun_ntp_deinit(void)
{
    if (ntp_handle == NULL) {
        return -1;
    }

    int32_t res = aiot_ntp_deinit(&ntp_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ntp_deinit failed: -0x%04X\r\n", -res);
        return -2;
    }

    return 0;
}
