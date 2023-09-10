/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:27:05
 * @FilePath: \firmware\user\protocol\aliyun\aliyun_shadow.c
 */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "aliyun_shadow.h"
#include "data/gateway_config.h"
// #include "can_screen_param.h"
// #include "sensor_position_manager.h"
#include "log/log.h"
#include "main/app_main.h"
#include "cJSON.h"

static const char *TAG = "ALIYUN_SHADOW";

extern void *g_shadow_handle;

static uint8_t shadow_report_res          = 0;
static uint8_t update_shadow_restart_flag = 0; // 报告设备影子当前参数后，是否需要重新启动

static uint8_t get_shadow_flag = 0; // 获取设备影子标志位

uint8_t g_flag_shadow_new_param = 0;

/* 发送获取设备影子的请求 */
int32_t demo_get_shadow(void *shadow_handle)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type = AIOT_SHADOWMSG_GET;

    int32_t res = aiot_shadow_send(shadow_handle, &message);

    if (res >= 0) {
        get_shadow_flag = 1;
    }

    return res;
}

/* 发送删除设备影子中特定reported值的请求 */
int32_t demo_delete_shadow_report(void *shadow_handle, char *reported, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type                         = AIOT_SHADOWMSG_DELETE_REPORTED;
    message.data.delete_reporte.reported = reported;
    message.data.delete_reporte.version  = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/* 发送清除设备影子中所有desired值的请求 */
int32_t demo_clean_shadow_desired(void *shadow_handle, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type                       = AIOT_SHADOWMSG_CLEAN_DESIRED;
    message.data.clean_desired.version = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/* 发送更新设备影子reported值的请求 */
int32_t demo_update_shadow(void *shadow_handle, char *reported_data, int64_t version)
{
    aiot_shadow_msg_t message;

    memset(&message, 0, sizeof(aiot_shadow_msg_t));
    message.type                 = AIOT_SHADOWMSG_UPDATE;
    message.data.update.reported = reported_data;
    message.data.update.version  = version;

    return aiot_shadow_send(shadow_handle, &message);
}

/// @brief 报告当前的配置信息
/// @param shadow_handle
/// @return >=0 向物联网平台报告了配置（反馈通过回调函数），否则说明读取配置失败或者发送失败
int8_t update_shadow(void *shadow_handle)
{
    char text[1200] = {0};
    uint16_t text_len;
    int8_t res;
    bool flag = false;

    cJSON *root = cJSON_CreateObject();

    // 1. 网关配置信息
    text_len = 128;
    res      = read_gateway_config_text(text, &text_len);
    if (res == 0) {
        cJSON_AddStringToObject(root, "gateway_config", text);
        flag = true;
    }

    if (flag) {
        char *json_string = cJSON_PrintUnformatted(root);

        LOGI(TAG, "json_string = %s", json_string);
        shadow_report_res = 0;

        res = demo_update_shadow(shadow_handle, json_string, 0);
        if (res < 0) {
            LOGE(TAG, "demo_delete_shadow_report failed, res = -0x%04x\r\n", -res);
        }

        cJSON_free(json_string);
        return res;
    }

    cJSON_Delete(root);
    return -1;
}

/// @brief 将设备影子上的参数写入flash中
/// @param payload
/// @return 0-写入flash中 其他-未写入
static int8_t shadow_recv_payload_write_flash(const char *payload)
{
    if (payload == NULL)
        return -1;

    cJSON *root = cJSON_Parse(payload);

    if (root == NULL)
        return -2;

    /* 若主动获取设备影子时，返回的JSON字符串中包含status字段 */
    cJSON *obj_status = cJSON_GetObjectItem(root, "status");

    if (obj_status != NULL) {
        LOGI(TAG, "status=%s", obj_status->valuestring);

        if (!strstr(obj_status->valuestring, "success")) {
            cJSON_Delete(root);
            return -3;
        }
    }

    cJSON *obj_state    = cJSON_GetObjectItem(root, "state");
    cJSON *obj_metadata = cJSON_GetObjectItem(root, "metadata");

    if (obj_state == NULL) {
        LOGE(TAG, "state is null");
        cJSON_Delete(root);
        return -4;
    }

    if (obj_metadata == NULL) {
        LOGE(TAG, "metadata is null");
        cJSON_Delete(root);
        return -5;
    }

    cJSON *obj_state_desired     = cJSON_GetObjectItem(obj_state, "desired");
    cJSON *obj_state_reported    = cJSON_GetObjectItem(obj_state, "reported");
    cJSON *obj_metadata_desired  = cJSON_GetObjectItem(obj_metadata, "desired");
    cJSON *obj_metadata_reported = cJSON_GetObjectItem(obj_metadata, "reported");

    // 1. 云平台上有历史的reported数据
    if (obj_state_desired != NULL && obj_metadata_desired != NULL && obj_state_reported != NULL && obj_metadata_reported != NULL) {
        bool flag = false;

        // 1. gateway_config
        cJSON *obj_metadata_desired_gateway_config  = cJSON_GetObjectItem(obj_metadata_desired, "gateway_config");
        cJSON *obj_metadata_reported_gateway_config = cJSON_GetObjectItem(obj_metadata_reported, "gateway_config");

        if (obj_metadata_desired_gateway_config != NULL && obj_metadata_reported_gateway_config != NULL) {
            cJSON *obj_metadata_desired_gateway_config_timestamp  = cJSON_GetObjectItem(obj_metadata_desired_gateway_config, "timestamp");
            cJSON *obj_metadata_reported_gateway_config_timestamp = cJSON_GetObjectItem(obj_metadata_reported_gateway_config, "timestamp");

            // 1. 网关配置信息
            if (obj_metadata_desired_gateway_config_timestamp != NULL && obj_metadata_reported_gateway_config_timestamp != NULL) {
                // 应用端的时间戳比设备端报告的时间戳大，表示是新的参数
                if (obj_metadata_desired_gateway_config_timestamp->valuedouble > obj_metadata_reported_gateway_config_timestamp->valuedouble) {
                    cJSON *obj_gateway_config = cJSON_GetObjectItem(obj_state_desired, "gateway_config");
                    if (obj_gateway_config != NULL) {
                        LOGI(TAG, "gateway_config = %s.", obj_gateway_config->valuestring);

                        // 写FLASH
                        int8_t res = write_gateway_config_text(obj_gateway_config->valuestring);

                        if (res == 0) {
                            flag = true;
                            LOGI(TAG, "write gateway config OK");
                        } else {
                            LOGE(TAG, "write gateway config ERROR(%d)", res);
                        }
                    }
                }
            }
        }
        // // 4. 重启ESP32使配置生效
        // if (flag)
        // {
        //     // 重启
        //     LOGE(TAG, "receive parameter from shadow, restart");
        //     esp_restart(); // 此时4G通讯正常，只重启ESP32模块
        // }

        if (flag) {
            LOGW(TAG, "There are new parameters");
            g_flag_shadow_new_param = 1;
            cJSON_Delete(root);
            return 0;
        } else {
            LOGW(TAG, "No new parameters");
            cJSON_Delete(root);
            return -20;
        }
    }
    // 2. 云平台上无历史reported数据
    else if (obj_state_desired != NULL && obj_metadata_desired != NULL) {
        bool flag = false;

        // 1. gateway_config
        cJSON *obj_gateway_config = cJSON_GetObjectItem(obj_state_desired, "gateway_config");
        if (obj_gateway_config != NULL) {
            LOGI(TAG, "gateway_config = %s.", obj_gateway_config->valuestring);

            // 写FLASH
            int8_t res = write_gateway_config_text(obj_gateway_config->valuestring);

            if (res == 0) {
                flag = true;
                LOGI(TAG, "write gateway config OK");
            } else {
                LOGE(TAG, "write gateway config ERROR(%d)", res);
            }
        }

        if (flag) {
            LOGW(TAG, "There are new parameters");
            g_flag_shadow_new_param = 1;

            cJSON_Delete(root);
            return 0;
        } else {
            LOGW(TAG, "No new parameters");
            cJSON_Delete(root);
            return -30;
        }
    }

    cJSON_Delete(root);
    return -100;
}

void update_shadow_task(void *pvParameters)
{
    int8_t res;
    for (uint8_t i = 0; i < 3; i++) {
        vTaskDelay(pdMS_TO_TICKS(2000)); // 给足够的时间让CAN显示屏可以读取到g_flag_shadow_new_param字段

        shadow_report_res = 0;
        res               = update_shadow(g_shadow_handle);

        if (res >= 0) {
            vTaskDelay(pdMS_TO_TICKS(5000));

            if (shadow_report_res) {
                break;
            }
        }
    }

    g_flag_shadow_new_param = 0;

    if (update_shadow_restart_flag) {
        LOGE(TAG, "receive parameter from shadow, restart");
        mcu_restart();
    }

    vTaskDelete(NULL);
}

/* 数据处理回调, 当SDK从网络上收到shadow消息时被调用 */
void demo_shadow_recv_handler(void *handle, const aiot_shadow_recv_t *recv, void *userdata)
{
    // LOGD(TAG, "demo_shadow_recv_handler, type = %d, productKey = %s, deviceName = %s\r\n", recv->type, recv->product_key, recv->device_name);

    LOGI(TAG, "type = %d, productKey = %s, deviceName = %s", recv->type, recv->product_key, recv->device_name);

    switch (recv->type) {
        /* 当设备发送AIOT_SHADOWMSG_UPDATE, AIOT_SHADOWMSG_CLEAN_DESIRED或者AIOT_SHADOWMSG_DELETE_REPORTED消息后, 会收到此应答消息 */
        /* 当设备发送AIOT_SHADOWMSG_GET时，若设备影子内容为空，则由此处返回错误 */
        case AIOT_SHADOWRECV_GENERIC_REPLY: {
            const aiot_shadow_recv_generic_reply_t *generic_reply = &recv->data.generic_reply;

#if (1)
            LOGI(TAG, "payload = \"%.*s\", status = %s, timestamp = %ld\r\n",
                 generic_reply->payload_len,
                 generic_reply->payload,
                 generic_reply->status,
                 (unsigned long)generic_reply->timestamp);
#endif

            if (strstr(generic_reply->status, "success")) {
                shadow_report_res = 1;
            } else if (get_shadow_flag && strstr(generic_reply->status, "error")) /* 获取的设备影子内容为空 */
            {
                get_shadow_flag = 0;

                // 上报云平台、不重启（防止通过串口等途径修改了参数，每次重启通知云平台）
                update_shadow_restart_flag = 0;

                if (xTaskCreate(update_shadow_task, "USP_TASK", 1024, NULL, 3, NULL) != pdPASS) {
                    LOGE(TAG, "create update_shadow_taskfailed !!!");
                }
            }
        } break;
        /* 当设备在线时, 若用户应用程序调用云端API主动更新设备影子, 设备便会收到此消息 */
        case AIOT_SHADOWRECV_CONTROL: {
            const aiot_shadow_recv_control_t *control = &recv->data.control;

            LOGW(TAG, "payload_len=%d", control->payload_len);
            LOGW(TAG, "version=%d", (unsigned long)control->version);

#if (0)
            LOGD(TAG, "payload = \"%.*s\", version = %ld\r\n",
                 control->payload_len,
                 control->payload,
                 (unsigned long)control->version);
#endif

            int8_t res = shadow_recv_payload_write_flash(control->payload);

            if (res == 0) // 写入flash成功
            {
                // 上报云平台并重启（使参数生效）
                update_shadow_restart_flag = 1;

                if (xTaskCreate(update_shadow_task, "USP_TASK", 1024, NULL, tskIDLE_PRIORITY, NULL) != pdPASS) {
                    LOGE(TAG, "create update_shadow_taskfailed !!!");
                }
            } else {
                LOGE(TAG, "write_flash err=%d", res);
            }
        } break;
        /* 当设备发送AIOT_SHADOWMSG_GET消息主动获取设备影子时, 会收到此消息 */
        case AIOT_SHADOWRECV_GET_REPLY: {
            get_shadow_flag = 0;

            const aiot_shadow_recv_get_reply_t *get_reply = &recv->data.get_reply;

#if (0)
            LOGD(TAG, "payload = \"%.*s\", version = %ld\r\n",
                 get_reply->payload_len,
                 get_reply->payload,
                 (unsigned long)get_reply->version);
#endif

            int8_t res = shadow_recv_payload_write_flash(get_reply->payload);

            if (res == 0) // 写入flash成功
            {
                // 上报云平台并重启（使参数生效）
                update_shadow_restart_flag = 1;
            } else {
                // 上报云平台、不重启（防止通过串口等途径修改了参数，每次重启通知云平台）
                update_shadow_restart_flag = 0;
            }

            if (xTaskCreate(update_shadow_task, "USP_TASK", 1024, NULL, tskIDLE_PRIORITY, NULL) != pdPASS) {
                LOGE(TAG, "create update_shadow_taskfailed !!!");
            }
        }
        default:
            break;
    }
}
