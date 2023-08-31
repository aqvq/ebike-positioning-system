
#ifndef __APP_INFO_H
#define __APP_INFO_H

#include <stdint.h>
#include "error_type.h"

#define APP_INFO_ENABLED_FLAG (0xCD)

typedef struct {
    int8_t id;          // app编号,合法值为1或2
    uint32_t addr;      // app地址
    uint32_t timestamp; // app下载时间
    uint32_t size;      // app大小
    // version字段: "0.0.1"，是有三位的版本号,分别是对应的version里面的：major, minor, patch
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_patch;
    uint8_t note[16];     // app信息
    uint8_t enabled;      // 是否有效
    uint8_t reserved[47]; // 保留
} app_info_t;

typedef struct {
    app_info_t app_current;
    app_info_t app_previous;
} app_partition_t;

// 将app info格式化为JSON字符串
error_t partition_info_print(const app_partition_t *config, char *output, uint16_t *output_len);

// 将JSON字符串转换为app info
error_t partition_info_parse(const char *message, app_partition_t *config);

// 读配置信息，以字符串返回
error_t read_partition_info_text(char *out_value);

// 写配置信息，用字符串作为参数
error_t write_partition_info_text(const char *value);

// 读配置信息
error_t read_partition_info(app_partition_t *config);
error_t read_app_current(app_info_t *config);
error_t read_app_previous(app_info_t *config);

// 写配置信息
error_t write_partition_info(app_partition_t *config);
error_t write_app_current(app_info_t *config);
error_t write_app_previous(app_info_t *config);

error_t swap_app_info(app_partition_t *config);

#endif
