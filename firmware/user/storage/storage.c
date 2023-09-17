
#include <string.h>
#include "storage.h"
#include "common/error_type.h"
#include "log/log.h"
#include "main/app_main.h"

#define TAG "STORAGE"

// TODO: 添加注释

storage_interface_t g_storage_interface;

error_t storage_install_interface(storage_interface_t *interface)
{
    if (interface == NULL) {
        return STORAGE_INTERFACE_NULL;
    }
    if (interface->storage_read_data_func == NULL || interface->storage_write_data_func == NULL) {
        return ERROR_NULL_POINTER;
    }
    // 成员初始化
    memcpy(&g_storage_interface, interface, sizeof(storage_interface_t));
    if (g_storage_interface.storage_init_func != NULL) {
        if (g_storage_interface.storage_init_func() != OK) {
            return STORAGE_INIT_ERROR;
        }
    }
    LOGD(TAG, "storage data size: %d", sizeof(storage_data_t));

    storage_metadata_t config = {0};
    storage_read(metadata, &config);
    if (config.init != STORAGE_DEVICE_INIT_FLAG || config.data_version != STORAGE_METADATA_VERSION) {
        LOGW(TAG, "storage init flag: %d, data version: %d", config.init, config.data_version);
        LOGW(TAG, "reset storage device");

        uint8_t init_arr[64] = {0};
        uint32_t addr_begin  = interface->storage_base_address + offsetof(storage_data_t, metadata);
        uint32_t addr_end    = interface->storage_base_address + sizeof(storage_data_t);
        LOGD(TAG, "addr_begin: %p, addr_end: %p", addr_begin, addr_end);
        for (uint32_t i = addr_begin; i < addr_end; i += 64) {
            interface->storage_write_data_func(i, init_arr, 64);
        }
        memset(&config, 0, sizeof(storage_metadata_t));
        config.init = STORAGE_DEVICE_INIT_FLAG;
        // strcpy(config.storage_device_name, "w25q16");
        strcpy(config.storage_device_name, "at24c64");
        config.data_size    = sizeof(storage_data_t);
        config.data_version = STORAGE_METADATA_VERSION;
        storage_write(metadata, &config);
        // LOGD(TAG, "restart");
        // mcu_restart();
    }

    return OK;
}

error_t storage_default_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    if (memcpy((void *)data, (void *)addr, len) != NULL) {
        return OK;
    } else {
        return STORAGE_MEMCPY_ERROR;
    }
}

error_t storage_default_write(uint32_t addr, uint8_t *data, uint32_t len)
{
    if (memcpy((void *)addr, (void *)data, len) != NULL) {
        return OK;
    } else {
        return STORAGE_MEMCPY_ERROR;
    }
}
