/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 17:26:36
 * @FilePath: \firmware\user\data\gnss_settings.c
 * @Description: GNSS配置信息数据结构实现
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "gnss_settings.h"
#include "log/log.h"
#include "common/error_type.h"
#include "storage/storage.h"
#include "cJSON.h"
#include <string.h>

error_t read_gnss_settings(gnss_settings_t *config)
{
    error_t err = OK;
    err         = storage_read(gnss_settings, config);
    return err;
}

error_t write_gnss_settings(gnss_settings_t *config)
{
    error_t err = OK;
    err         = storage_write(gnss_settings, config);
    return err;
}

error_t gnss_settings_parse(char *input, gnss_settings_t *config)
{
    error_t err = OK;
    cJSON *root = cJSON_Parse(input);
    cJSON *obj_gnss_upload_strategy;
    cJSON *obj_gnss_offline_strategy;
    cJSON *obj_gnss_ins_strategy;
    cJSON *obj_gnss_mode;

    if (root != NULL) {
        obj_gnss_upload_strategy  = cJSON_GetObjectItem(root, "gnss_upload_strategy");
        obj_gnss_offline_strategy = cJSON_GetObjectItem(root, "gnss_offline_strategy");
        obj_gnss_ins_strategy     = cJSON_GetObjectItem(root, "gnss_ins_strategy");
        obj_gnss_mode             = cJSON_GetObjectItem(root, "gnss_mode");
        if (obj_gnss_upload_strategy != NULL) {
            config->gnss_upload_strategy = obj_gnss_upload_strategy->valueint;
        }
        if (obj_gnss_offline_strategy != NULL) {
            config->gnss_offline_strategy = obj_gnss_offline_strategy->valueint;
        }
        if (obj_gnss_ins_strategy != NULL) {
            config->gnss_ins_strategy = obj_gnss_ins_strategy->valueint;
        }
        if (obj_gnss_mode != NULL) {
            config->gnss_mode = obj_gnss_mode->valueint;
        }
    } else {
        return ERROR_NULL_POINTER;
    }
    cJSON_Delete(root);
    return err;
}

error_t gnss_settings_print(gnss_settings_t *config, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "gnss_upload_strategy", config->gnss_upload_strategy);
    cJSON_AddNumberToObject(root, "gnss_offline_strategy", config->gnss_offline_strategy);
    cJSON_AddNumberToObject(root, "gnss_ins_strategy", config->gnss_ins_strategy);
    cJSON_AddNumberToObject(root, "gnss_mode", config->gnss_mode);

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);
    // output[json_string_len] = '\0';

    if (output_len) {
        *output_len = (uint16_t)json_string_len;
    }
    strcpy(output, json_string);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

error_t write_gnss_settings_text(const char *text)
{
    error_t err            = OK;
    gnss_settings_t config = {0};

    if (!text) {
        return ERROR_NULL_POINTER;
    }
    if (*text == '\0') {
        // 清空存储数据
        err = write_gnss_settings(&config);
        return err;
    }

    err = gnss_settings_parse(text, &config);
    if (err == OK) {
        err = write_gnss_settings(&config);
    }
    return err;
}

error_t read_gnss_settings_text(char *text)
{
    error_t err            = OK;
    gnss_settings_t config = {0};
    err                    = read_gnss_settings(&config);
    if (err == OK) {
        err = gnss_settings_print(&config, text, NULL);
    }
    return err;
}
