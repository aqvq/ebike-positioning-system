/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 16:42:45
 * @FilePath: \firmware\user\storage\storage.h
 * @Description: 存储模块功能实现头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */
#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>
#include "common/error_type.h"
#include "aliyun/aliyun_dynreg.h"
#include "data/device_info.h"
#include "log/log.h"
#include "data/gnss_settings.h"
#include "utils/macros.h"

// 存储设备初始化标志
#define STORAGE_DEVICE_INIT_FLAG (0x8E)

// 存储设备数据类型版本，当版本变化时，说明历史数据已损坏无法正确读取
// 这时需要将历史数据全部擦除。极少情况会遇到，一般都会考虑数据兼容性
#define STORAGE_METADATA_VERSION (13)

// 存储类型定义
typedef struct {
    char storage_device_name[16];
    uint32_t data_version;
    uint32_t data_size;
    uint8_t init;
    uint8_t reserved[16];
} storage_metadata_t;

/**
 * @brief 该数据结构实现数据存储管理，在结构体末尾添加需要存储的数据，并通过storage_read/write即可
 */
typedef struct {
    // 设备元数据
    storage_metadata_t metadata;
    // 阿里云登录参数
    devinfo_wl_t device_info;
    // 卫星定位参数
    gnss_settings_t gnss_settings;
} storage_data_t;

// 存储模块读取数据函数接口定义
typedef error_t (*storage_read_data_func_t)(uint32_t, uint8_t *, uint32_t);
// 存储模块写入数据函数接口定义
typedef error_t (*storage_write_data_func_t)(uint32_t, uint8_t *, uint32_t);
// 存储模块初始化函数接口定义
typedef error_t (*storage_init_func_t)(void);

// 存储模块接口定义
typedef struct
{
    uint32_t storage_base_address;
    storage_read_data_func_t storage_read_data_func;
    storage_write_data_func_t storage_write_data_func;
    storage_init_func_t storage_init_func;
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
 * @brief 检查是否是有效值（非0xFF），未使用
 *
 */
#define storage_check(storage_data_member_ptr) ({                                   \
    (((((uint8_t *)(storage_data_member_ptr))[0]) != 0xFF) ? OK : STORAGE_NO_DATA); \
})

// 存储模块接口结构体
extern storage_interface_t g_storage_interface;

/// @brief 注册存储模块接口，初始化存储设备
error_t storage_install_interface(storage_interface_t *interface);

/// @brief 缺省读函数，使用memcpy函数读取数据，适用于将配置信息存放于ram中。
/// @param addr 存储器地址
/// @param data 读取数据存放地址
/// @param len 读取数据大小
/// @return 错误码
error_t storage_default_read(uint32_t addr, uint8_t *data, uint32_t len);

/// @brief 缺省写函数，使用memcpy函数写入数据，适用于将配置信息存放于ram中。
/// @param addr 存储器地址
/// @param data 待写入数据存放地址
/// @param len 待写入数据大小
/// @return 错误码
error_t storage_default_write(uint32_t addr, uint8_t *data, uint32_t len);

#endif // !__STORAGE_H
