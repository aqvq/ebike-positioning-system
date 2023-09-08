
#include <string.h>
#include <stdio.h>
#include "cJSON.h"
#include "aliyun_message_handle.h"
#include "log/log.h"
#include "aliyun_config.h"
#include "aiot_mqtt_api.h"
#include "utils/util.h"
#include "aliyun_config.h"
#include "utils/time.h"

#if (ALIYUN_VERSION == ALIYUN_VERSION_V2)
#include "pb.h"
#include "pb_common.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "gateway_message.pb.h"

bool write_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    char *str = *arg;
    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (uint8_t *)str, strlen(str));
}
#endif

static const char *TAG = "ALIYUN_MESSAGE_HADLE";

//--------------------------------------全局变量----------------------------------------------

#if (ALIYUN_VERSION == ALIYUN_VERSION_V1)
/* publish 溫度，壓力 加速度信息 */
static void send_gnss_data_msg(void *handle, void *data)
{
    if (NULL == handle || data == NULL) {
        return;
    }

    char *gnss_string = (char *)data;

    char pub_topic[128];
    char *pub_payload;

    memset(pub_topic, 0, sizeof(pub_topic));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "gps", gnss_string);

    pub_payload = cJSON_Print(root);
    sprintf(pub_topic, "/%s/%s/%s/%s", PRODUCT_KEY, get_device_name(), "user", "gps");

    int32_t res = aiot_mqtt_pub(handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);

    cJSON_Delete(root);
    cJSON_free(pub_payload);

    if (res < 0) {
        LOGE(TAG, "aiot_mqtt_pub failed, res: -0x%04X", -res);
    }
}

#elif (ALIYUN_VERSION == ALIYUN_VERSION_V2)

static void send_gnss_data_msg(void *handle, void *data)
{
    if (NULL == handle || data == NULL) {
        return;
    }

    mytime_t now;
    time(&now); // 获得时间戳

    pb_gps_message message   = pb_gps_message_init_zero;
    message.message_type     = MESSAGE_TYPE_GPS;
    message.utc_time         = (uint32_t)now;
    message.tid.arg          = get_device_name();
    message.tid.funcs.encode = &write_string;

    char *gnss_string = (char *)data;

    char *line = NULL;
    line       = strstr(gnss_string, "+QGPSLOC");

    const char *fmt = "+QGPSLOC: %[^,]%*[,]%f,%f,%f,%f,%d,%f,%f,%f,%[^,]%*[,]%d";

    if (line != NULL) {
        line = strtok(line, "\r\n");

        if (line) {
            // printf("%s\r\n", line);

            char time[20] = {0};
            char date[10] = {0};

            // if (sscanf(line, fmt, time, latitude, longitude, &message.hdop, &message.altitude, &message.fix, &message.cog, &message.spkm, &message.spkn, date, &message.nsat))
            if (sscanf(line, fmt, time, &message.latitude, &message.longitude, &message.hdop, &message.altitude, &message.fix, &message.cog, &message.spkm, &message.spkn, date, &message.nsat)) {
                // printf("latitude=%f\r\n", message.latitude);
                // printf("longitude=%f\r\n", message.longitude);

#if 1
                /* 利用GPS消息中的时间更新系统时间 */
                if (update_system_time_flag == 0) {
                    int year  = 0;
                    int month = 0;
                    int day;
                    int hour   = 0;
                    int minute = 0;
                    int second = 0;

                    if (sscanf(date, "%2d%2d%2d", &day, &month, &year)) {
                        year += 2000;

                        if (sscanf(time, "%2d%2d%2d", &hour, &minute, &second)) {
                            LOGW(TAG, "year=%d month=%d day=%d", year, month, day);
                            LOGW(TAG, "hour=%d minute=%d second=%d", hour, minute, second);

                            // 设置时间
                            int res = set_time(year, month, day, hour, minute, second);

                            if (res != 0) {
                                LOGE(TAG, "set time err = %d.", res);
                            } else {
                                update_system_time_flag = 1;
                            }
                        }
                    }
                }
#endif

                uint8_t buffer[100];
                pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

                bool status           = pb_encode(&stream, pb_gps_message_fields, &message);
                size_t message_length = stream.bytes_written;

                if (!status) {
                    LOGE(TAG, "encoding failed: %s", PB_GET_ERROR(&stream));
                } else {
                    char pub_topic[128];
                    memset(pub_topic, 0, sizeof(pub_topic));
                    sprintf(pub_topic, "/%s/%s/%s/%s", PRODUCT_KEY, get_device_name(), "user", "gps");

                    if (NULL != handle) {
                        int32_t res = aiot_mqtt_pub(handle, pub_topic, buffer, (uint32_t)message_length, 0);

                        if (res < 0) {
                            LOGE(TAG, "aiot_mqtt_pub failed, res: -0x%04X", -res);
                        }
                    }
                }
            }
        }
    }
}

#endif

void handle_aliyun_message(void *handle, general_message_t *general_message)
{
    if (handle == NULL || general_message == NULL || general_message->data == NULL)
        return;

    switch (general_message->type) {
        case GNSS_DATA:
            send_gnss_data_msg(handle, general_message->data);
            break;
        default:
            LOGE(TAG, "unknow data type");
            break;
    }
}
