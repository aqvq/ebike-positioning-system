/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:11:42
 * @FilePath: \firmware\user\storage\storage.h
 */
#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>
#include "error_type.h"
#include "msg/gateway_config.h"
#include "protocol/aliyun/aliyun_dynreg.h"
#include "protocol/aliyun/device_info.h"
#include "log/log.h"
#include "partition/partition_info.h"
#include "utils/macros.h"

#define STORAGE_DEVICE_INIT_FLAG (0x7E)
#define STORAGE_METADATA_VERSION (10)

typedef struct {
    char storage_device_name[16];
    uint32_t data_version;
    uint32_t data_size;
    uint8_t init;
    uint8_t reserved[32];
} storage_metadata_t;

/**
 * @brief 该数据结构实现数据存储管理，在结构体末尾添加需要存储的数据，并通过storage_read/write即可
 */
typedef struct {
    app_partition_t app;                                        // 应用信息
    storage_metadata_t metadata;                                // 设备元数据
    devinfo_wl_t aliyun_devinfo_wl;                             // 阿里云白名单登录参数
    devinfo_nwl_t aliyun_devinfo_nwl;                           // 阿里云免白名单登录参数
    gateway_config_t gateway_config;                            // 网关配置参数
} storage_data_t;

typedef struct
{
    uint32_t storage_base_address;
    error_t (*storage_read_data_func)(uint32_t, uint8_t *data, uint32_t len);
    error_t (*storage_write_data_func)(uint32_t, uint8_t *data, uint32_t len);
    error_t (*storage_init_func)(void);
} storage_interface_t;

/**
 * @brief 读取存储区指定结构体
 * @param storage_data_member_t 指定要读取的结构体名称
 * @param storage_data_member_ptr 数据保存地址
 * @return 错误码。正常返回OK。
 */
#define storage_read(storage_data_member_t, storage_data_member_ptr) ({                             \
    g_storage_interface.storage_read_data_func(                                                     \
        g_storage_interface.storage_base_address + offsetof(storage_data_t, storage_data_member_t), \
        storage_data_member_ptr,                                                                    \
        member_size(storage_data_t, storage_data_member_t));                                        \
})

/**
 * @brief 存储指定结构体
 * @param storage_data_member_t 指定要读取的结构体名称
 * @param storage_data_member_ptr 数据保存地址
 * @return 错误码。正常返回OK。
 */
#define storage_write(storage_data_member_t, storage_data_member_ptr) ({                            \
    g_storage_interface.storage_write_data_func(                                                    \
        g_storage_interface.storage_base_address + offsetof(storage_data_t, storage_data_member_t), \
        storage_data_member_ptr,                                                                    \
        member_size(storage_data_t, storage_data_member_t));                                        \
})

/**
 * @brief 检查是否是有效值（非0xFF）
 *
 */
#define storage_check(storage_data_member_ptr) ({                                   \
    (((((uint8_t *)(storage_data_member_ptr))[0]) != 0xFF) ? OK : STORAGE_NO_DATA); \
})

// 存储配置信息结构体
extern storage_interface_t g_storage_interface;
// 注册配置信息
error_t storage_install_interface(storage_interface_t *interface);
error_t storage_default_read(uint32_t, uint8_t *data, uint32_t len);
error_t storage_default_write(uint32_t, uint8_t *data, uint32_t len);

#endif // !__STORAGE_H
