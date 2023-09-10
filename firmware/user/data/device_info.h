#ifndef __DEVICE_INFO_H
#define __DEVICE_INFO_H

#include "error_type.h"

/* 白名单模式下用于保存deviceSecret的结构体定义 */
typedef struct
{
    char device_name[20];
    char device_secret[64];
} devinfo_wl_t;

// not used
/* 免白名单模式下用于保存mqtt建连信息clientid, username和password的结构体定义 */
typedef struct
{
    char conn_clientid[128];
    char conn_username[128];
    char conn_password[64];
} devinfo_nwl_t;

error_t read_device_info(devinfo_wl_t *devinfo_wl);
error_t write_device_info(devinfo_wl_t *devinfo_wl);
error_t device_info_parse(char *input, devinfo_wl_t *device);
error_t device_info_print(devinfo_wl_t *config, char *output, uint16_t *output_len);
error_t write_device_info_text(const char *text);
error_t read_device_info_text(char *text);

#endif // !__DEVICE_INFO_H
