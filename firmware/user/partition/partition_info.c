
#include <stdint.h>
#include <string.h>
#include "log/log.h"
#include "utils/util.h"
#include "cJSON.h"
#include "storage/storage.h"
#include "partition_info.h"
#include "FreeRTOS.h"

static const char *TAG = "APPINFO";

error_t partition_info_print(const app_partition_t *config, char *output, uint16_t *output_len)
{
    cJSON *root        = cJSON_CreateObject();
    cJSON *app_current = cJSON_AddObjectToObject(root, "app_current");
    cJSON_AddNumberToObject(app_current, "id", config->app_current.id);
    cJSON_AddNumberToObject(app_current, "addr", config->app_current.addr);
    cJSON_AddNumberToObject(app_current, "timestamp", config->app_current.timestamp);
    cJSON_AddNumberToObject(app_current, "size", config->app_current.size);
    cJSON_AddNumberToObject(app_current, "version_major", config->app_current.version_major);
    cJSON_AddNumberToObject(app_current, "version_minor", config->app_current.version_minor);
    cJSON_AddNumberToObject(app_current, "version_patch", config->app_current.version_patch);
    cJSON_AddStringToObject(app_current, "note", config->app_current.note);
    cJSON_AddNumberToObject(app_current, "enabled", config->app_current.enabled);

    cJSON *app_previous = cJSON_AddObjectToObject(root, "app_previous");
    cJSON_AddNumberToObject(app_previous, "id", config->app_previous.id);
    cJSON_AddNumberToObject(app_previous, "addr", config->app_previous.addr);
    cJSON_AddNumberToObject(app_previous, "timestamp", config->app_previous.timestamp);
    cJSON_AddNumberToObject(app_previous, "size", config->app_previous.size);
    cJSON_AddNumberToObject(app_previous, "version_major", config->app_previous.version_major);
    cJSON_AddNumberToObject(app_previous, "version_minor", config->app_previous.version_minor);
    cJSON_AddNumberToObject(app_previous, "version_patch", config->app_previous.version_patch);
    cJSON_AddStringToObject(app_previous, "note", config->app_previous.note);
    cJSON_AddNumberToObject(app_previous, "enabled", config->app_previous.enabled);

    char *json_string       = cJSON_PrintUnformatted(root);
    size_t json_string_len  = strlen(json_string);
    output[json_string_len] = '\0';

    if (output_len) {
        *output_len = (uint16_t)json_string_len;
    }
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return OK;
}

error_t partition_info_parse(const char *message, app_partition_t *config)
{
    cJSON *root = cJSON_Parse(message);
    cJSON *app_current;
    cJSON *app_previous;
    cJSON *obj_id;
    cJSON *obj_addr;
    cJSON *obj_timestamp;
    cJSON *obj_size;
    cJSON *obj_version_major;
    cJSON *obj_version_minor;
    cJSON *obj_version_patch;
    cJSON *obj_note;
    cJSON *obj_enabled;

    if (root == NULL) {
        return ERROR_CJSON_PARSE;
    }

    app_current                       = cJSON_GetObjectItem(root, "app_current");
    obj_id                            = cJSON_GetObjectItem(app_current, "id");
    obj_addr                          = cJSON_GetObjectItem(app_current, "addr");
    obj_timestamp                     = cJSON_GetObjectItem(app_current, "timestamp");
    obj_size                          = cJSON_GetObjectItem(app_current, "size");
    obj_version_major                 = cJSON_GetObjectItem(app_current, "version_major");
    obj_version_minor                 = cJSON_GetObjectItem(app_current, "version_minor");
    obj_version_patch                 = cJSON_GetObjectItem(app_current, "version_patch");
    obj_note                          = cJSON_GetObjectItem(app_current, "note");
    obj_enabled                       = cJSON_GetObjectItem(app_current, "enabled");
    config->app_current.id            = obj_id->valueint;
    config->app_current.addr          = obj_addr->valueint;
    config->app_current.timestamp     = obj_timestamp->valueint;
    config->app_current.size          = obj_size->valueint;
    config->app_current.version_major = obj_version_major->valueint;
    config->app_current.version_minor = obj_version_minor->valueint;
    config->app_current.version_patch = obj_version_patch->valueint;
    strcpy(config->app_current.note, obj_note->valuestring);
    config->app_current.enabled = obj_enabled->valueint;

    app_previous                       = cJSON_GetObjectItem(root, "app_previous");
    obj_id                             = cJSON_GetObjectItem(app_previous, "id");
    obj_addr                           = cJSON_GetObjectItem(app_previous, "addr");
    obj_timestamp                      = cJSON_GetObjectItem(app_previous, "timestamp");
    obj_size                           = cJSON_GetObjectItem(app_previous, "size");
    obj_version_major                  = cJSON_GetObjectItem(app_previous, "version_major");
    obj_version_minor                  = cJSON_GetObjectItem(app_previous, "version_minor");
    obj_version_patch                  = cJSON_GetObjectItem(app_previous, "version_patch");
    obj_note                           = cJSON_GetObjectItem(app_previous, "note");
    obj_enabled                        = cJSON_GetObjectItem(app_previous, "enabled");
    config->app_previous.id            = obj_id->valueint;
    config->app_previous.addr          = obj_addr->valueint;
    config->app_previous.timestamp     = obj_timestamp->valueint;
    config->app_previous.size          = obj_size->valueint;
    config->app_previous.version_major = obj_version_major->valueint;
    config->app_previous.version_minor = obj_version_minor->valueint;
    config->app_previous.version_patch = obj_version_patch->valueint;
    strcpy(config->app_previous.note, obj_note->valuestring);
    config->app_previous.enabled = obj_enabled->valueint;

    cJSON_Delete(root);
    return OK;
}

// @brief 读app信息
/// @param out_value
/// @param length
/// @return
error_t read_partition_info_text(char *out_value)
{
    app_partition_t config = {0};
    error_t err            = OK;

    err = read_partition_info(&config);
    if (err == OK) {
        err = partition_info_print(&config, out_value, NULL);
    }
    return err;
}

/// @brief 写app信息
/// @param value
/// @return
error_t write_partition_info_text(const char *value)
{
    app_partition_t config = {0};
    error_t err;

    if (!value) {
        return ERROR_NULL_POINTER;
    }
    if (*value == '\0') {
        // 清空存储数据
        err = write_partition_info(&config);
        return err;
    }

    // 从flash中读取当前的配置信息
    err = read_partition_info(&config);
    if (err == OK) {
        // 解析JSON字符串，更新配置参数
        err = partition_info_parse(value, &config);
        if (err == OK) {
            err = write_partition_info(&config);
        }
    }
    return err;
}

// 读配置信息
error_t read_partition_info(app_partition_t *config)
{
    error_t err = storage_read(app, config);
    return err;
}

error_t read_app_current(app_info_t *config)
{
    app_partition_t partition_info = {0};
    error_t err                    = read_partition_info(&partition_info);
    memcpy(config, &partition_info.app_current, sizeof(app_info_t));
    return err;
}

error_t read_app_previous(app_info_t *config)
{
    app_partition_t partition_info = {0};
    error_t err                    = read_partition_info(&partition_info);
    memcpy(config, &partition_info.app_previous, sizeof(app_info_t));
    return err;
}

// 写配置信息
error_t write_partition_info(app_partition_t *config)
{
    error_t err = storage_write(app, config);
    return err;
}

error_t write_app_current(app_info_t *config)
{
    app_partition_t partition_info = {0};
    error_t err                    = read_partition_info(&partition_info);
    memcpy(&partition_info.app_current, config, sizeof(app_info_t));
    err |= write_partition_info(&partition_info);
    return err;
}

error_t write_app_previous(app_info_t *config)
{
    app_partition_t partition_info = {0};
    error_t err                    = read_partition_info(&partition_info);
    memcpy(&partition_info.app_previous, config, sizeof(app_info_t));
    err |= write_partition_info(&partition_info);
    return err;
}

// 交换配置信息
error_t swap_app_info(app_partition_t *config)
{
    app_info_t temp;
    memcpy(&temp, &config->app_current, sizeof(app_info_t));
    memcpy(&config->app_current, &config->app_previous, sizeof(app_info_t));
    memcpy(&config->app_previous, &temp, sizeof(app_info_t));
    return OK;
}