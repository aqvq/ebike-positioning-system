
#include "device_info.h"
#include "storage/storage.h"
#include "cJSON.h"

error_t read_device_info(devinfo_wl_t *devinfo_wl)
{
    error_t err = OK;
    err         = storage_read(aliyun_devinfo_wl, devinfo_wl);
    return err;
}

error_t write_device_info(devinfo_wl_t *devinfo_wl)
{
    error_t err = OK;
    err         = storage_write(aliyun_devinfo_wl, devinfo_wl);
    return err;
}

error_t device_info_parse(char *input, devinfo_wl_t *device)
{
    error_t err = OK;
    cJSON *root = cJSON_Parse(input);
    cJSON *obj_name;
    cJSON *obj_secret;

    if (root != NULL) {
        obj_name   = cJSON_GetObjectItem(root, "name");
        obj_secret = cJSON_GetObjectItem(root, "secret");
        if (obj_name != NULL && obj_secret != NULL) {
            strcpy(device->device_name, obj_name->valuestring);
            strcpy(device->device_secret, obj_secret->valuestring);
            err = OK;
        } else {
            err = ERROR_NULL_POINTER;
        }
    } else {
        return ERROR_NULL_POINTER;
    }
    cJSON_Delete(root);
    return err;
}

error_t device_info_print(devinfo_wl_t *config, char *output, uint16_t *output_len)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "name", config->device_name);
    cJSON_AddStringToObject(root, "secret", config->device_secret);

    char *json_string       = cJSON_PrintUnformatted(root);
    size_t json_string_len  = strlen(json_string);
    output[json_string_len] = '\0';

    if (output_len) {
        *output_len = (uint16_t)json_string_len;
    }
    memcpy((void *)output, (void *)json_string, json_string_len);

    cJSON_free(json_string);
    cJSON_Delete(root);
    return 0;
}

error_t write_device_info_text(const char *text)
{
    error_t err         = OK;
    devinfo_wl_t config = {0};

    if (!text) {
        return ERROR_NULL_POINTER;
    }
    if (*text == '\0') {
        // 清空存储数据
        err = write_device_info(&config);
        return err;
    }

    err = device_info_parse(text, &config);
    if (err == OK) {
        err = write_device_info(&config);
    }
    return err;
}

error_t read_device_info_text(char *text)
{
    error_t err         = OK;
    devinfo_wl_t config = {0};
    err                 = read_device_info(&config);
    if (err == OK) {
        err = device_info_print(&config, text, NULL);
    }
    return err;
}
