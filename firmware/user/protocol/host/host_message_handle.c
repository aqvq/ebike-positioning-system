
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocol/host/host_message_handle.h"
#include "cJSON.h"
#include "log/log.h"

#include "minmea.h"
#include "gnss/gnss.h"
#include "utils/util.h"

#define TAG "HOST"

static int8_t general_message_to_string(general_message_t *general_message, char *output, uint8_t *output_len);
static void to_json_string(cJSON *root, char *output, uint8_t *output_len);

static cJSON *create_cjson_root_with_gnss_data(void *data);
static cJSON *create_cjson_root_with_gnss_nmea_data(void *data);

#define MAX_JSON_LEN 1024

int8_t handle_host_message(general_message_t *general_message)
{
    if (general_message == NULL || general_message->data == NULL)
        return -1;

    // 输出的JSON字符串
    char json_string[MAX_JSON_LEN] = {0};
    uint8_t json_string_len        = 0;
    int8_t res;

    res = general_message_to_string(general_message, json_string, &json_string_len);

    if (res == 0) {
        if (strlen(json_string) > 0) {
            // 打印字符串到串口
            LOG("%s", json_string);
            res = 0;
        } else {
            res = -100;
        }
    }

    return res;
}

static int8_t general_message_to_string(general_message_t *general_message, char *output, uint8_t *output_len)
{
    if (general_message == NULL || general_message->data == NULL)
        return -1;

    cJSON *root;

    switch (general_message->type) {
        case GNSS_DATA:
            root = create_cjson_root_with_gnss_data(general_message->data);
            break;
        case GNSS_NMEA_DATA:
            root = create_cjson_root_with_gnss_nmea_data(general_message->data);
            break;
        default:
            return -2;
            break;
    }

    to_json_string(root, output, output_len);

    cJSON_Delete(root);
    return 0;
}

static void to_json_string(cJSON *root, char *output, uint8_t *output_len)
{
    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    *output_len = (uint8_t)json_string_len;
    memcpy((void *)output, (void *)json_string, json_string_len);

    vPortFree(json_string);
}

//----------------------------- GNSS NMEA DATA ------------------------------------------
#if 0
static cJSON *create_minmea_float_object(const struct minmea_float *f)
{
    cJSON *obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(obj, "value", f->value);
    cJSON_AddNumberToObject(obj, "scale", f->scale);

    return obj;
}
#endif

static cJSON *create_minmea_float_object(const struct minmea_float *f)
{
    if (f->scale <= 0) {
        return cJSON_CreateNull();
    } else {
        return cJSON_CreateNumber((double)f->value / (double)f->scale);
    }
}

static cJSON *create_gnss_nmea_gsa_object(struct minmea_sentence_gsa *gsa_frame)
{
    cJSON *obj_gsa = cJSON_CreateObject();

    // mode
    char mode[1];
    sprintf(mode, "%c", gsa_frame->mode);
    cJSON_AddStringToObject(obj_gsa, "mode", mode);

    // fix_type
    cJSON_AddNumberToObject(obj_gsa, "fix_type", gsa_frame->fix_type);

    // sats
    cJSON *array_gsa_sats = cJSON_CreateArray();
    int i;
    for (i = 0; i < 12; i++) {
        cJSON_AddItemToArray(array_gsa_sats, cJSON_CreateNumber(gsa_frame->sats[i]));
    }
    cJSON_AddItemToObject(obj_gsa, "sats", array_gsa_sats);

    // pdop
    cJSON_AddItemToObject(obj_gsa, "pdop", create_minmea_float_object(&gsa_frame->pdop));

    // hdop
    cJSON_AddItemToObject(obj_gsa, "hdop", create_minmea_float_object(&gsa_frame->hdop));

    // vdop
    cJSON_AddItemToObject(obj_gsa, "vdop", create_minmea_float_object(&gsa_frame->vdop));

    return obj_gsa;
}

static cJSON *create_gnss_nmea_gga_object(struct minmea_sentence_gga *gga_frame)
{
    cJSON *obj_gga = cJSON_CreateObject();

    // time
    char time[10];
    sprintf(time, "%d:%d:%d.%d", gga_frame->time.hours,
            gga_frame->time.minutes,
            gga_frame->time.seconds,
            gga_frame->time.microseconds);
    cJSON_AddStringToObject(obj_gga, "time", time);

    // latitude
    cJSON_AddItemToObject(obj_gga, "latitude", create_minmea_float_object(&gga_frame->latitude));

    // longitude
    cJSON_AddItemToObject(obj_gga, "longitude", create_minmea_float_object(&gga_frame->longitude));

    // fix_quality
    cJSON_AddNumberToObject(obj_gga, "fix_quality", gga_frame->fix_quality);

    // satellites_tracked
    cJSON_AddNumberToObject(obj_gga, "satellites_tracked", gga_frame->satellites_tracked);

    // hdop
    cJSON_AddItemToObject(obj_gga, "hdop", create_minmea_float_object(&gga_frame->hdop));

    // altitude
    cJSON_AddItemToObject(obj_gga, "altitude", create_minmea_float_object(&gga_frame->altitude));

    // altitude_units
    char altitude_units[1];
    sprintf(altitude_units, "%c", gga_frame->altitude_units);
    cJSON_AddStringToObject(obj_gga, "altitude_units", altitude_units);

    // height
    cJSON_AddItemToObject(obj_gga, "height", create_minmea_float_object(&gga_frame->height));

    // height_units
    char height_units[1];
    sprintf(height_units, "%c", gga_frame->height_units);
    cJSON_AddStringToObject(obj_gga, "height_units", height_units);

    // dgps_age
    cJSON_AddItemToObject(obj_gga, "dgps_age", create_minmea_float_object(&gga_frame->dgps_age));

    return obj_gga;
}

static cJSON *create_gnss_nmea_rmc_object(struct minmea_sentence_rmc *rmc_frame)
{
    cJSON *obj_rmc = cJSON_CreateObject();

    // time
    char time[10];
    sprintf(time, "%d:%d:%d.%d", rmc_frame->time.hours,
            rmc_frame->time.minutes,
            rmc_frame->time.seconds,
            rmc_frame->time.microseconds);
    cJSON_AddStringToObject(obj_rmc, "time", time);

    // valid
    if (rmc_frame->valid) {
        cJSON_AddTrueToObject(obj_rmc, "valid");
    } else {
        cJSON_AddFalseToObject(obj_rmc, "valid");
    }

    // latitude
    cJSON_AddItemToObject(obj_rmc, "latitude", create_minmea_float_object(&rmc_frame->latitude));

    // longitude
    cJSON_AddItemToObject(obj_rmc, "longitude", create_minmea_float_object(&rmc_frame->longitude));

    // speed
    cJSON_AddItemToObject(obj_rmc, "speed", create_minmea_float_object(&rmc_frame->speed));

    // course
    cJSON_AddItemToObject(obj_rmc, "course", create_minmea_float_object(&rmc_frame->course));

    // date
    char date[10];
    sprintf(date, "%d-%d-%d",
            rmc_frame->date.year,
            rmc_frame->date.month,
            rmc_frame->date.day);
    cJSON_AddStringToObject(obj_rmc, "date", date);

    // variation
    cJSON_AddItemToObject(obj_rmc, "variation", create_minmea_float_object(&rmc_frame->variation));

    return obj_rmc;
}

static cJSON *create_gnss_nmea_vtg_object(struct minmea_sentence_vtg *vtg_frame)
{
    cJSON *obj_vtg = cJSON_CreateObject();

    // true_track_degrees
    cJSON_AddItemToObject(obj_vtg, "true_track_degrees", create_minmea_float_object(&vtg_frame->true_track_degrees));

    // magnetic_track_degrees
    cJSON_AddItemToObject(obj_vtg, "magnetic_track_degrees", create_minmea_float_object(&vtg_frame->magnetic_track_degrees));

    // speed_knots
    cJSON_AddItemToObject(obj_vtg, "speed_knots", create_minmea_float_object(&vtg_frame->speed_knots));

    // speed_kph
    cJSON_AddItemToObject(obj_vtg, "speed_kph", create_minmea_float_object(&vtg_frame->speed_kph));

    // faa_mode
    char faa_mode[1];
    sprintf(faa_mode, "%c", vtg_frame->faa_mode);
    cJSON_AddStringToObject(obj_vtg, "faa_mode", faa_mode);

    return obj_vtg;
}

static cJSON *create_cjson_root_with_gnss_data(void *data)
{
    char *gps_data = (char *)data;
    cJSON *root    = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "data_type", "gnss_data");
    cJSON_AddStringToObject(root, "data", gps_data);

    return root;
}

static cJSON *create_cjson_root_with_gnss_nmea_data(void *data)
{
    gnss_nmea_data_t *nmea_data = (gnss_nmea_data_t *)data;
    cJSON *root                 = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "data_type", "gnss_nmea_data");
    cJSON_AddItemToObject(root, "gsa", create_gnss_nmea_gsa_object(&nmea_data->gsa_frame));
    cJSON_AddItemToObject(root, "gga", create_gnss_nmea_gga_object(&nmea_data->gga_frame));
    cJSON_AddItemToObject(root, "rmc", create_gnss_nmea_rmc_object(&nmea_data->rmc_frame));
    cJSON_AddItemToObject(root, "vtg", create_gnss_nmea_vtg_object(&nmea_data->vtg_frame));

    return root;
}

