#include "cJSON.h"
#include "gateway_config.h"
#include <stdio.h>
#include <string.h>
#include "log/log.h"
#include "storage/storage.h"
#include "error_type.h"

static const char *TAG = "GATEWAY_CONFIG";

error_t read_gateway_config_text(char *text, uint16_t *text_len)
{
    if (text == NULL) {
        return ERROR_NULL_POINTER;
    }
    error_t err             = OK;
    gateway_config_t config = {0};
    err                     = read_gateway_config(&config);
    if (err == OK) {
        err = gateway_config_print(&config, text, NULL);
    }
    if (err == OK && text_len) {
        *text_len = strlen(text);
    }
    return err;
}

error_t write_gateway_config_text(const char *text)
{
    error_t err             = OK;
    gateway_config_t config = {0};

    if (!text) {
        return ERROR_NULL_POINTER;
    }
    if (*text == '\0') {
        // 清空存储数据
        err = write_gateway_config(&config);
        return err;
    }

    err = read_gateway_config(&config);
    if (err == OK) {
        err = gateway_config_parse(text, &config);
        if (err == OK) {
            err = write_gateway_config(&config);
        }
    }
    return err;
}

error_t gateway_config_print(const gateway_config_t *config, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "apn", config->apn);
    cJSON_AddStringToObject(root, "apn_username", config->apn_username);
    cJSON_AddStringToObject(root, "apn_password", config->apn_password);

    char *json_string      = cJSON_PrintUnformatted(root);
    size_t json_string_len = strlen(json_string);

    if (output_len) {
        *output_len = (uint16_t)json_string_len;
    }
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return OK;
}

error_t gateway_config_parse(const char *message, gateway_config_t *config)
{
    cJSON *root = cJSON_Parse(message);
    cJSON *obj_apn;
    cJSON *obj_apn_username;
    cJSON *obj_apn_password;

    if (root == NULL) {
        return CJSON_PARSE_ERROR;
    }

    obj_apn          = cJSON_GetObjectItem(root, "apn");
    obj_apn_username = cJSON_GetObjectItem(root, "apn_username");
    obj_apn_password = cJSON_GetObjectItem(root, "apn_password");

    if (obj_apn != NULL) {
        strcpy(config->apn, obj_apn->valuestring);
    } else {
        LOGW(TAG, "item apn not exist.");
    }

    if (obj_apn_username != NULL) {
        strcpy(config->apn_username, obj_apn_username->valuestring);
    } else {
        LOGW(TAG, "item apn_username not exist.");
    }

    if (obj_apn_password != NULL) {
        strcpy(config->apn_password, obj_apn_password->valuestring);
    } else {
        LOGW(TAG, "item apn_password not exist.");
    }

    cJSON_Delete(root);
    return OK;
}

error_t read_gateway_config(gateway_config_t *config)
{
    if (OK != storage_read(gateway_config, config)) {
        return READ_GATEWAY_CONFIG_ERROR;
    }
    return OK;
}

error_t write_gateway_config(gateway_config_t *config)
{
    int8_t res;
    res = storage_write(gateway_config, config);
    return res;
}
