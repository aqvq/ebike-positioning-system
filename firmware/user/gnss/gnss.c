/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:34:46
 * @FilePath: \firmware\user\gnss\gnss.c
 * @Description: gnss功能实现
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "aiot_at_api.h"
#include "bsp/at/at.h"
#include "aiot_state_api.h"
#include "log/log.h"
#include "utils/util.h"
#include "aiot_mqtt_api.h"
#include "gnss.h"
#include "cJSON.h"
#include "minmea.h"
#include "data/general_message.h"
#include "protocol/iot/iot_helper.h"
#include "main/app_main.h"
#include "bsp/at/at.h"
#include "data/gnss_settings.h"

#define TAG "GNSS"

//---------------------------------------宏定义-------------------------------------------------------------

// 上传字符串数据，解析交由服务器处理
#define GPS_DATA (1)
// 上传结构体数据，在设备端解析
#define GPS_NMEA_DATA (2)
// 上传的GPS数据类型
#define GPS_DATA_TYPE GPS_DATA

//-------------------------------------全局变量-------------------------------------------------------------

// 定位字符串，被"bsp/at/at_gnss"引用
char gnss_string[128];
// 全局变量，记录gnss nema数据
static gnss_nmea_data_t _gnss_nmea_data;
// OTA升级标志，定义在main.c中
extern uint8_t g_app_upgrade_flag;

//----------------------------------- 订阅相关的参数 -------------------------------------------------------

// 订阅队列的上限
#define MAX_SUBSCRIBER_COUNT (10)
// 订阅数量
static uint8_t subscriber_count;
// 订阅用于存储数据的队列句柄
static QueueHandle_t subscribers[MAX_SUBSCRIBER_COUNT];

//---------------------------------- 局部变量 --------------------------------------------------------------

// gnss定位516错误计数
uint32_t gnss_error_516_num = 0;
// gnss定位50错误计数
uint32_t gnss_error_50_num = 0;

//---------------------------------- 函数定义 --------------------------------------------------------------

/// @brief 订阅gnss数据，订阅者将会不断收到gnss数据
/// @param subscriber_queue 订阅者接收数据的队列
/// @return int8_t
int8_t subscribe_gnss_data(QueueHandle_t subscriber_queue)
{
    if (subscriber_count >= MAX_SUBSCRIBER_COUNT) {
        return -1;
    }
    subscribers[subscriber_count] = subscriber_queue;
    subscriber_count++;
    return 0;
}

/// @brief 将接收到的gnss nema字符串数据转换为结构体数据，暂未使用
/// @param line 字符串数据
/// @return int8_t
static int8_t translate_gnss_data(const char *line)
{
    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_GSA: {
            if (minmea_parse_gsa(&_gnss_nmea_data.gsa_frame, line)) {
                return 0;
            } else {
                LOGE(TAG, "$xxGSA sentence is not parsed");
                return -1;
            }
        } break;
        case MINMEA_SENTENCE_GGA: {
            if (minmea_parse_gga(&_gnss_nmea_data.gga_frame, line)) {
                return 0;
            } else {
                LOGE(TAG, "$xxGGA sentence is not parsed");
                return -2;
            }
        } break;
        case MINMEA_SENTENCE_RMC: {
            if (minmea_parse_rmc(&_gnss_nmea_data.rmc_frame, line)) {
                return 0;
            } else {
                LOGE(TAG, "$xxRMC sentence is not parsed");
                return -3;
            }
        } break;
        case MINMEA_SENTENCE_VTG: {
            if (minmea_parse_vtg(&_gnss_nmea_data.vtg_frame, line)) {
                return 0;
            } else {
                LOGE(TAG, "$xxVTG sentence is not parsed");
                return -4;
            }
        } break;
        default:
            LOGE(TAG, "unknow minmea sentence id");
            return -5;
            break;
    }

    // switch (minmea_sentence_id(line, false))
    // {
    // case MINMEA_SENTENCE_RMC:
    // {
    //     struct minmea_sentence_rmc frame;
    //     if (minmea_parse_rmc(&frame, line))
    //     {
    //         printf("$xxRMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\n",
    //                frame.latitude.value, frame.latitude.scale,
    //                frame.longitude.value, frame.longitude.scale,
    //                frame.speed.value, frame.speed.scale);
    //         printf("$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\n",
    //                minmea_rescale(&frame.latitude, 1000),
    //                minmea_rescale(&frame.longitude, 1000),
    //                minmea_rescale(&frame.speed, 1000));
    //         printf("$xxRMC floating point degree coordinates and speed: (%f,%f) %f\n",
    //                minmea_tocoord(&frame.latitude),
    //                minmea_tocoord(&frame.longitude),
    //                minmea_tofloat(&frame.speed));
    //     }
    //     else
    //     {
    //         printf("$xxRMC sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_GGA:
    // {
    //     struct minmea_sentence_gga frame;
    //     if (minmea_parse_gga(&frame, line))
    //     {
    //         printf("$xxGGA: fix quality: %d\n", frame.fix_quality);
    //     }
    //     else
    //     {
    //         printf("$xxGGA sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_GST:
    // {
    //     struct minmea_sentence_gst frame;
    //     if (minmea_parse_gst(&frame, line))
    //     {
    //         printf("$xxGST: raw latitude,longitude and altitude error deviation: (%d/%d,%d/%d,%d/%d)\n",
    //                frame.latitude_error_deviation.value, frame.latitude_error_deviation.scale,
    //                frame.longitude_error_deviation.value, frame.longitude_error_deviation.scale,
    //                frame.altitude_error_deviation.value, frame.altitude_error_deviation.scale);
    //         printf("$xxGST fixed point latitude,longitude and altitude error deviation"
    //                " scaled to one decimal place: (%d,%d,%d)\n",
    //                minmea_rescale(&frame.latitude_error_deviation, 10),
    //                minmea_rescale(&frame.longitude_error_deviation, 10),
    //                minmea_rescale(&frame.altitude_error_deviation, 10));
    //         printf("$xxGST floating point degree latitude, longitude and altitude error deviation: (%f,%f,%f)",
    //                minmea_tofloat(&frame.latitude_error_deviation),
    //                minmea_tofloat(&frame.longitude_error_deviation),
    //                minmea_tofloat(&frame.altitude_error_deviation));
    //     }
    //     else
    //     {
    //         printf("$xxGST sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_GSV:
    // {
    //     struct minmea_sentence_gsv frame;
    //     if (minmea_parse_gsv(&frame, line))
    //     {
    //         printf("$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
    //         printf("$xxGSV: satellites in view: %d\n", frame.total_sats);
    //         for (int i = 0; i < 4; i++)
    //             printf("$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
    //                    frame.sats[i].nr,
    //                    frame.sats[i].elevation,
    //                    frame.sats[i].azimuth,
    //                    frame.sats[i].snr);
    //     }
    //     else
    //     {
    //         printf("$xxGSV sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_VTG:
    // {
    //     struct minmea_sentence_vtg frame;
    //     if (minmea_parse_vtg(&frame, line))
    //     {
    //         printf("$xxVTG: true track degrees = %f\n",
    //                minmea_tofloat(&frame.true_track_degrees));
    //         printf("        magnetic track degrees = %f\n",
    //                minmea_tofloat(&frame.magnetic_track_degrees));
    //         printf("        speed knots = %f\n",
    //                minmea_tofloat(&frame.speed_knots));
    //         printf("        speed kph = %f\n",
    //                minmea_tofloat(&frame.speed_kph));
    //     }
    //     else
    //     {
    //         printf("$xxVTG sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_GSA:
    // {
    //     struct minmea_sentence_gsa frame;
    //     if (minmea_parse_gsa(&frame, line))
    //     {
    //         printf("$xxGSA: mode = %d\n", frame.mode);
    //         printf("        type = %d\n", frame.fix_type);
    //         printf("        pdop = %f\n", minmea_tofloat(&frame.pdop));
    //         printf("        hdop = %f\n", minmea_tofloat(&frame.hdop));
    //         printf("        vdop = %f\n", minmea_tofloat(&frame.vdop));
    //     }
    //     else
    //     {
    //         printf("$xxGSA sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_SENTENCE_ZDA:
    // {
    //     struct minmea_sentence_zda frame;
    //     if (minmea_parse_zda(&frame, line))
    //     {
    //         printf("$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d\n",
    //                frame.time.hours,
    //                frame.time.minutes,
    //                frame.time.seconds,
    //                frame.date.day,
    //                frame.date.month,
    //                frame.date.year,
    //                frame.hour_offset,
    //                frame.minute_offset);
    //     }
    //     else
    //     {
    //         printf("$xxZDA sentence is not parsed\n");
    //     }
    // }
    // break;

    // case MINMEA_INVALID:
    // {
    //     printf("$xxxxx sentence is not valid\n");
    // }
    // break;

    // default:
    // {
    //     printf("$xxxxx sentence is not parsed\n");
    // }
    // break;
    // }
}

/// @brief gnss nema回调函数
/// @param rsp 接收到的字符串数据
/// @return at_rsp_result_t
at_rsp_result_t gnss_nmea_rsp_handler(char *rsp)
{
    // TODO: 函数实现过于繁琐，现阶段未使用，故暂不修改
    if (!strstr(rsp, "ERROR")) {
        char *str1   = strstr(rsp, "+QGPSGNMEA");
        uint8_t flag = 0;

        if (str1) {
            char *str2 = strstr(str1, "$");

            if (str2) {
                char *str3 = strtok(str2, "\r\n");

                if (str3) {
                    flag = 1;
                    // LOGI(TAG, "gnss info:[%s] len=%d", str3, strlen(str3));
                    if (translate_gnss_data(str3) == 0) {
                        return AT_RSP_SUCCESS;
                    }
                }
            }
        }

        if (!flag) {
            LOGE(TAG, "unknow gps info:[%s] len=%d", rsp, strlen(rsp));
        }
    } else {
        LOGE(TAG, "rsp=%s", rsp);
    }

    return AT_RSP_FAILED;
}

#if 0
float spkm[12] = {0.0};
float altitude[12] = {0.0};

/* publish gnss */
int app_ble_4g_mqtt_pub_gnss(void *handle, char *gnss_string)
{
    char pub_topic[128];
    char *pub_payload;

    memset(pub_topic, 0, sizeof(pub_topic));

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "gps", gnss_string);

#if 0
    // printf("gps info:%s, length of gps: %d\n", gnss_string, strlen(gnss_string));
    uint8_t flag = 0;
    uint8_t pos[10] = {0};
    GPS_info_t gps;
    for (uint8_t i = 0; i < strlen(gnss_string); i++)
    {
        if (gnss_string[i] == ',')
        {
            pos[flag] = i;
            flag++;
        }
    }
    // for (uint8_t i = 0; i < 10; i++)
    // {
    //     printf("pos[%d]: %d\n", i, pos[i]);
    // }
    gps.UTC_time[10] = '\0';
    gps.latitude[10] = '\0';
    gps.longitude[11] = '\0';
    gps.COG[6] = '\0';
    gps.date[6] = '\0';
    char tmpstr[10] = {0};

    memcpy(gps.UTC_time, gnss_string+(pos[0]-10), 10);
    memcpy(gps.latitude, gnss_string+pos[0]+1, (pos[1]-pos[0]-1));
    memcpy(gps.longitude, gnss_string+pos[1]+1, (pos[2]-pos[1]-1));

    memcpy(tmpstr, gnss_string+pos[2]+1, (pos[3]-pos[2]-1));
    gps.HDOP = atof(tmpstr);

    memset(tmpstr, 0, sizeof(char)*10);
    memcpy(tmpstr, gnss_string+pos[3]+1, (pos[4]-pos[3]-1));
    gps.altitude = atof(tmpstr);

    memset(tmpstr, 0, sizeof(char)*10);
    memcpy(tmpstr, gnss_string+pos[4]+1, (pos[5]-pos[4]-1));
    gps.fix = atoi(tmpstr);

    memcpy(gps.COG, gnss_string+pos[5]+1, (pos[6]-pos[5]-1));

    memset(tmpstr, 0, sizeof(char)*10);
    memcpy(tmpstr, gnss_string+pos[6]+1, (pos[7]-pos[6]-1));
    gps.spkm = atof(tmpstr);

    memset(tmpstr, 0, sizeof(char)*10);
    memcpy(tmpstr, gnss_string+pos[7]+1, (pos[8]-pos[7]-1));
    gps.spkn = atof(tmpstr);

    memcpy(gps.date, gnss_string+pos[8]+1, (pos[9]-pos[8]-1));

    memset(tmpstr, 0, sizeof(char)*10);
    memcpy(tmpstr, gnss_string+pos[9]+1, 2);
    gps.nsat = atoi(tmpstr);

    printf("UTC_time:%s, latitude:%s, longitude:%s, HDOP:%.1f, altitude:%.1f, fix:%d, C0G:%s, spkm:%.1f, spkn:%.1f, date:%s, nsat:%d\n",
    gps.UTC_time, gps.latitude, gps.longitude, gps.HDOP, gps.altitude, gps.fix, gps.COG, gps.spkm, gps.spkn, gps.date, gps.nsat);

    for (uint8_t i = 0; i < 11; i++)
    {
        spkm[12-i-1] = spkm[12-i-2];
        altitude[12-i-1] = altitude[12-i-2];
    }

    spkm[0] = gps.spkm;
    altitude[0] = gps.altitude;

    for (uint8_t i = 0; i < 12; i++)
    {
        LOGE(TAG, "i:%d, gps.spkm:%.1f", i, spkm[i]);
    }

    uint8_t stopFlag = 0;
    for (uint8_t i = 0; i < 7; i++)
    {
        if (spkm[i] < 3.6)
        {
            stopFlag++;
        }
    }

    //最近7报数据中，若有4包或4包以上的速度小于3.6km/h，则认为车辆停止运行
    if (stopFlag >= 4)
    {
        LOGE(TAG, "The vehicle is stopping.");
    }
    else
    {
        LOGE(TAG, "The vehicle is running.");
    }

    uint8_t altitudeFlag = 0;
    for (uint8_t i = 0; i < 5; i++)
    {
        if (altitude[i] > 0.0)
        {
            altitudeFlag++;
        }
    }

    //最近5包数据中，若有4包或4包以上的高度不为0，则当前高度为0的数据包不发送到4g
    if (altitudeFlag >= 4 && gps.altitude < 0.001)
    {
        LOGE(TAG, "The current altitude is 0, but not send.");
    }
    else
    {
        LOGE(TAG, "Send.");
    }
#endif

    pub_payload = cJSON_Print(root);
    sprintf(pub_topic, "/%s/%s/%s/%s", PRODUCT_KEY, get_device_name(), "user", "gnss");

    if (NULL == handle)
    {
        LOGE(TAG, "mqtt handle is NULL");
        cJSON_Delete(root);
        cJSON_free(pub_payload);

        return -1;
    }

    int32_t res = aiot_mqtt_pub(handle, pub_topic, (uint8_t *)pub_payload, (uint32_t)strlen(pub_payload), 0);

    cJSON_Delete(root);
    cJSON_free(pub_payload);

    if (res < 0)
    {
        LOGE(TAG, "aiot_mqtt_pub failed, res: -0x%04X", -res);
        return -1;
    }

    return 0;
}
#endif

#if GPS_DATA_TYPE == GPS_DATA
/// @brief 向订阅的队列推送消息
static void general_message_to_queue(void)
{
    uint8_t i;

    for (i = 0; i < subscriber_count; i++) // 向订阅的对象发送BLE原始数据
    {
        // 若队列快要溢出，出动释放最新放入队列的元素。
        if (remove_queue_first_item(subscribers[i]) != 0) {
            LOGE(TAG, "remove_queue_first_item failed");
        }

        // 创建通用消息
        general_message_t *msg = create_general_message(GNSS_DATA, gnss_string, strlen(gnss_string));
        if (pdPASS != xQueueSend(subscribers[i], (void *)&msg, 10)) {
            free_general_message(msg); // 发送失败，释放资源
            LOGE(TAG, "send data to queue failed");
        }
    }
}
#else
/// @brief 向订阅的队列推送消息
static void general_message_to_queue(void)
{
    uint8_t i;

    for (i = 0; i < subscriber_count; i++) // 向订阅的对象发送BLE原始数据
    {
        // 若队列快要溢出，出动释放最新放入队列的元素。
        if (remove_queue_first_item(subscribers[i]) != 0) {
            LOGE(TAG, "remove_queue_first_item failed");
        }

        // 创建通用消息
        general_message_t *msg = create_general_message(GNSS_NMEA_DATA, &_gnss_nmea_data, sizeof(gnss_nmea_data_t));
        if (pdPASS != xQueueSend(subscribers[i], (void *)&msg, 10)) {
            free_general_message(msg); // 发送失败，释放资源
            LOGE(TAG, "send data to queue failed");
        }
    }
}
#endif

#if GPS_DATA_TYPE == GPS_DATA

// gnss数据结构
typedef struct gnss_data {
    char UTC[20];
    char latitude[20];
    char longitude[20];
    float HDOP;
    float altitude;
    int fix;
    char COG[10];
    float spkm;
    float spkn;
    char date[10];
    char nsat[10];
} gnss_data_t;

/// @brief 解析gnss字符串
/// @param data gnss字符串数据
/// @return int8_t
static int8_t parse_gnss_string(gnss_data_t *data)
{
    if (!data) {
        return -1;
    }

    // 参考示例：
    // +QGPSLOC: 074839.000,3145.8432N,11716.0268E,1.1,76.3,3,000.00,0.0,0.0,121222,20
    // +QGPSLOC: <UTC>,<latitude>,<longitude>,<HDOP>,<altitude>,<fix>,<COG>,<spkm>,<spkn>,<date>,<nsat>
    // <UTC> 字符串类型。UTC时间。格式：hhmmss.sss（引自GPGGA语句）。
    // <latitude> 字符串类型。纬度。如果<mode>为0：格式：ddmm.mmmmN/S（引自GPGGA语句）dd 度。范围：00~89mm.mmmm 分。范围：00.0000~59.9999 N/S 北纬/南纬
    // <longitude> 字符串类型。经度。如果<mode>为 0：格式：dddmm.mmmmE/W（引自 GPGGA 语句）ddd 度。范围：000~179mm.mmmm 分。范围：00.0000~59.9999E/W 东经/西经
    // <HDOP> 水平精度因子。范围：0.5~99.9（引自GPGGA语句）。
    // <altitude> 天线的海拔高度。精确到小数点后一位。单位：米（引自GPGGA语句）。
    // <fix> 整型。GNSS定位模式（引自GAGSA/GPGSA语句）。2 2D定位 3 3D定位
    // <COG> 字符串类型。以正北方为对地航向。格式：ddd.mm（引自GPVTG语句）。ddd 度。范围：000~359mm 分。范围：00~59
    // <spkm> 对地速度。精确到小数点后一位。单位：千米/时（引自GPVTG语句）。
    // <spkn> 对地速度。精确到小数点后一位。单位：节（引自GPVTG语句）。
    // <date> UTC日期。格式：ddmmyy（引自GPRMC语句）。
    // <nsat> 卫星数量。固定两位数，前导位数不足则补0（引自GPGGA语句）。

    char src[128];
    strcpy(src, gnss_string);

    char *line = NULL;
    line       = strstr(src, "+QGPSLOC");
    char *fmt  = "+QGPSLOC: %[^,]%*[,]%[^,]%*[,]%[^,]%*[,]%f,%f,%d,%[^,]%*[,]%f,%f,%[^,]%*[,]%[^,]%*[,]";

    if (line != NULL) {
        line = strtok(line, "\r\n");

        if (line) {
            if (sscanf(line, fmt, data->UTC, data->latitude, data->longitude, &data->HDOP, &data->altitude, &data->fix, data->COG, &data->spkm, &data->spkn, data->date, data->nsat)) {
                // LOGI(TAG, "%s ------------ %d", line, strlen(line));

                // printf("UTC=%s\r\n", data->UTC);
                // printf("latitude=%s\r\n", data->latitude);
                // printf("longitude=%s\r\n", data->longitude);
                // printf("HDOP=%f\r\n", data->HDOP);
                // printf("altitude=%f\r\n", data->altitude);
                // printf("fix=%d\r\n", data->fix);
                // printf("COG=%s\r\n", data->COG);
                // printf("spkm=%f\r\n", data->spkm);
                // printf("spkn=%f\r\n", data->spkn);
                // printf("date=%s\r\n", data->date);
                // printf("nsat=%s\r\n", data->nsat);

                return 0;
            }
        }
    }

    return -2;
}
#endif

/* GPS定位时间间隔,单位:ms */
#define GNSS_INTERVAL_TIME (3000)
/* 允许错误50持续的最大时长,单位:ms */
#define GNSS_ERROR_50_CONTINUE_TIME_MAX (3 * 60 * 1000)
/* 连续采集GPS速度个数 */
#define SPEED_NUM (100)
/* 速度平均值 */
float g_gnss_speed_mean = 0;

/*******************************************************************
 * 向物联网平台发送GPS数据的策略
 * 1. 模块初始化后或者定位失败后，前N个点的数据发送给物联网平台
 * 2. 若连续N个点的速度值均低于阈值T，则第N+1个点的数据不发送给物联网平台
 * 3. 每隔M个点，必须给物联网平台发送一次数据
 *******************************************************************/
#define SEND_IOT_GPS_N (10)
#define SEND_IOT_GPS_M (200)
#define SEND_IOT_GPS_T (0)

/// @brief gnss任务
/// @param pvParameters 未使用可忽略
void gnss_task(void *pvParameters)
{
#if GPS_DATA_TYPE == GPS_DATA
    uint16_t count = 0; // 累计获得GPS个数

    float gnss_speeds[SPEED_NUM];
    uint16_t gnss_speed_index = 0;

    uint8_t send_iot_num         = 0;
    uint8_t send_iot_under_T_num = 0; // 速度连续低于SEND_IOT_GPS_T的个数

#endif

    for (;;) {
        // 当执行升级任务时退避
        if (g_app_upgrade_flag) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(GNSS_INTERVAL_TIME));

#if GPS_DATA_TYPE == GPS_DATA
        // 老的接口
        if (ec800m_at_gnss_location() != STATE_SUCCESS) {
            LOGD(TAG, "ec800m_at_gnss_location failed");
            continue;
        }

        // +QGPSLOC: 074839.000,3145.8432N,11716.0268E,1.1,76.3,3,000.00,0.0,0.0,121222,20
        // +QGPSLOC: 094614.000,3145.8370N,11716.0300E,1.4,91.0,3,000.00,0.0,0.0,200223,20
        if (gnss_string[0] != '\0' && !strstr(gnss_string, "ERROR")) {
            gnss_data_t data;
            data.spkm = 1000;

            if (parse_gnss_string(&data) == 0) {
                // 1. 计算平均速度的逻辑，用于定时器判断是否定时重启功能
                gnss_speeds[gnss_speed_index] = data.spkm;
                gnss_speed_index++;

                if (gnss_speed_index >= SPEED_NUM) {
                    g_gnss_speed_mean = mean(gnss_speeds, SPEED_NUM);
                    gnss_speed_index  = 0;

                    LOGI(TAG, "gnss_speed_mean=%.1fkm/h", g_gnss_speed_mean);
                }
            } else {
                gnss_speed_index  = 0; // 一次解析失败，则index置零
                g_gnss_speed_mean = 0;
            }

            // 2. 计数
            if (data.spkm <= SEND_IOT_GPS_T) {
                if (send_iot_under_T_num <= SEND_IOT_GPS_N) {
                    send_iot_under_T_num++;
                }
            } else {
                send_iot_under_T_num = 0;
            }

            if (send_iot_num <= SEND_IOT_GPS_N) {
                send_iot_num++;
            }

            if (send_iot_num <= SEND_IOT_GPS_N ||         // 前SEND_IOT_GPS_N个值必须发送
                send_iot_under_T_num <= SEND_IOT_GPS_N || // 连续低速不发送数据
                count % SEND_IOT_GPS_M == 0)              // 连续SEND_IOT_GPS_M个值必须发送一次
            {
                // 向订阅的任务推送数据
                general_message_to_queue();
                // LOGD(TAG, "gnss data: %s", gnss_string);
                // LOGI(TAG, "send gps %d", count);
            }

            if (count % 15 == 0) // 每隔一段时间打印一次GPS日志
            {
                LOGI(TAG, "info:%s \r\nlen:%d", gnss_string, strlen(gnss_string));
            }

            gnss_error_50_num  = 0;
            gnss_error_516_num = 0;
            count++;
        } else {
            gnss_speed_index  = 0;
            g_gnss_speed_mean = 0;

            send_iot_num         = 0;
            send_iot_under_T_num = 0;

            if (strstr(gnss_string, "516")) {
                gnss_error_516_num++;
            } else if (strstr(gnss_string, "50")) {
                gnss_error_50_num++;
            }

            char info[200] = {0};
            sprintf(info, "gnss_string=%s,E516=%u,E50=%u", gnss_string, gnss_error_516_num, gnss_error_50_num);

            LOGE(TAG, "%s", info);
            iot_post_log(IOT_LOG_LEVEL_ERROR, "GNSS", 100, info);

            //----------------------------------------------
            // 发现：错误代码50一般出现在打开GPS后前2次定位时，然后出现错误代码一般是516。
            // 若GPS正常工作后，连续出现错误码50，则GPS模块将不能恢复，直到下次定时重启时。
            // 因此：此处发现一段时间内都是GPS定位失败，则直接重启整个系统。
            //----------------------------------------------
            if ((gnss_error_50_num * GNSS_INTERVAL_TIME) >= GNSS_ERROR_50_CONTINUE_TIME_MAX) {
                iot_post_log(IOT_LOG_LEVEL_ERROR, "GNSS", 100, "Restart all (error 50).");
                vTaskDelay(pdMS_TO_TICKS(1000));
                ec800m_poweroff_and_mcu_restart();
            }
        }
#else
        // 回调函数中，全部数据获取并且数据被正确解析
        if (ec800m_at_gnss_nmea_query() == 0) {
            // 向订阅的任务推送数据
            general_message_to_queue();
        }
#endif
    }

    // 一般不会运行到这里
    ec800m_at_gnss_close();
    vTaskDelete(NULL);
}

/// @brief gnss初始化
void gnss_init(void)
{
    // while (at_handle.is_init != 1)
    //     vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t retry = GNSS_MAX_RETRY;

    // 读取gnss配置信息
    gnss_settings_t config = {0};
    error_t err            = read_gnss_settings(&config);
    if (err != OK) {
        LOGE(TAG, "read gnss settings error");
        return;
    } else {
        LOGI(TAG, "read gnss settings success");
        LOGD(TAG, "gnss_upload_strategy: %d", config.gnss_upload_strategy);
        LOGD(TAG, "gnss_offline_strategy: %d", config.gnss_offline_strategy);
        LOGD(TAG, "gnss_ins_strategy: %d", config.gnss_ins_strategy);
        LOGD(TAG, "agps: %d", (config.gnss_mode & GNSS_AGPS_MSK) >> GNSS_AGPS_SHIFT);
        LOGD(TAG, "apflash: %d", (config.gnss_mode & GNSS_APFLASH_MSK) >> GNSS_APFLASH_SHIFT);
        LOGD(TAG, "autogps: %d", (config.gnss_mode & GNSS_AUTOGPS_MSK) >> GNSS_AUTOGPS_SHIFT);
        LOGD(TAG, "gnss_mode: %d", (config.gnss_mode & GNSS_MODE_CONFIG_MSK) >> GNSS_MODE_CONFIG_SHIFT);
    }

    do {
        static uint8_t gnss_state;
        int32_t res = STATE_SUCCESS;

        // 1. 设置AGPS 重启生效
        static uint8_t agps_state = 0;
        res                       = ec800m_at_gnss_agps_state(&agps_state);
        // 如果当前状态不符合设定的状态
        if (res == STATE_SUCCESS && ((config.gnss_mode & GNSS_AGPS_MSK) >> GNSS_AGPS_SHIFT) != agps_state) {
            if (agps_state == 0) {
                // AGPS处于关闭状态，打开AGPS
                res = ec800m_at_gnss_enable_agps();
                if (res < 0) {
                    LOGE(TAG, "open agps failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "open agps success.");
                }
            } else {
                // AGPS处于打开状态，关闭AGPS
                res = ec800m_at_gnss_disable_agps();
                if (res < 0) {
                    LOGE(TAG, "close agps failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "close agps success.");
                }
            }
        }

        // 2. 设置AUTOGPS 重启生效
        static uint8_t autogps_state = 0;
        res                          = ec800m_at_gnss_autogps_state(&autogps_state);
        // 如果当前状态不符合设定的状态
        if (res == STATE_SUCCESS && autogps_state != ((config.gnss_mode & GNSS_AUTOGPS_MSK) >> GNSS_AUTOGPS_SHIFT)) {
            if (autogps_state == 0) {
                // 打开autogps，重启生效
                res = ec800m_at_gnss_enable_autogps();
                if (res < 0) {
                    LOGE(TAG, "open autogps failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "open autogps success");
                }
            } else {
                // 关闭autogps，重启生效
                res = ec800m_at_gnss_disable_autogps();
                if (res < 0) {
                    LOGE(TAG, "close autogps failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "close autogps success");
                }
            }
        }

        // 3. 设置APFLASH 立即生效
        static uint8_t apflash_state = 0;
        res                          = ec800m_at_gnss_apflash_state(&apflash_state);
        // 如果当前状态不符合设定的状态
        if (res == STATE_SUCCESS && apflash_state != ((config.gnss_mode & GNSS_APFLASH_MSK) >> GNSS_APFLASH_SHIFT)) {
            if (apflash_state == 0) {
                // 打开APFLASH，立即生效
                res = ec800m_at_gnss_enable_apflash();
                if (res < 0) {
                    LOGE(TAG, "open apflash failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "open apflash success");
                }
            } else {
                // 关闭APFLASH，立即生效
                res = ec800m_at_gnss_disable_apflash();
                if (res < 0) {
                    LOGE(TAG, "close apflash failed, error code: %d", res);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    continue;
                } else {
                    LOGI(TAG, "close apflash success");
                }
            }
        }

        // 4. 配置GNSS模式 立即生效
        uint8_t gnss_config_mode = ((config.gnss_mode & GNSS_MODE_CONFIG_MSK) >> GNSS_MODE_CONFIG_SHIFT);
        res                      = ec800m_at_gnss_config(gnss_config_mode);
        if (res < 0) {
            LOGE(TAG, "config gnss failed, error code: %d", res);
            continue;
        } else {
            LOGI(TAG, "config gnss success (state: %d)", gnss_config_mode);
        }

        // 5. 查询GNSS是否打开
        res = ec800m_at_gnss_state(&gnss_state);
        if (res == STATE_SUCCESS && gnss_state == 1) {
            LOGW(TAG, "gnss is open.");
        } else {
            // 打开GNSS
            res = ec800m_at_gnss_open();
            if (res < 0) {
                LOGE(TAG, "open gnss failed, error code: %d", res);
                vTaskDelay(pdMS_TO_TICKS(3000));
                continue;
            } else {
                LOGI(TAG, "open gnss success");
            }
        }

        // 6. 创建任务
        TaskHandle_t xHandle;
        if (xTaskCreate(gnss_task, GNSS_TASK_NAME, GNSS_TASK_STACK_SIZE, NULL, GNSS_TASK_PRIORITY, &xHandle) != pdPASS) {
            LOGE(TAG, "create gnss task failed");
            break;
        } else {
            LOGI(TAG, "create gnss task success");
            break;
        }

    } while (retry--);

    vTaskDelete(NULL);
}
