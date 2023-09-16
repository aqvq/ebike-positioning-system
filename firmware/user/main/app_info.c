
#include "utils/time.h"
#include "data/partition_info.h"
#include "main.h"
#include "bsp/flash/boot.h"
#include "bsp/mcu/mcu.h"
#include "upgrade/iap.h"

void print_app_info(app_info_t *app_info)
{
    char date_time[32] = {0};
    printf("\r\npartition id:  %1d", app_info->id);
    printf("\r\napp version:   v%d.%d.%d", app_info->version_minor, app_info->version_major, app_info->version_patch);
    printf("\r\napp size:      %dKB", app_info->size >> 10);
    printf("\r\nboot address:  0x%08X", app_info->addr);
    printf("\r\napp note:      \"%s\"", app_info->note);
    print_time(app_info->timestamp, date_time, sizeof(date_time));
    printf("\r\nupdate time:   %s", date_time);
}

void reset_app_info(app_partition_t *storage_data)
{
    uint32_t addr = flash_get_alternate_bank_address();
    if (addr == APP2_FLASH_ADDRESS) {
        memset((void *)storage_data, 0, sizeof(app_partition_t));
        storage_data->app_previous.id            = 2;
        storage_data->app_previous.version_minor = 0;
        storage_data->app_previous.version_major = 0;
        storage_data->app_previous.version_patch = 0;
        storage_data->app_previous.enabled       = 0;
        strcpy(storage_data->app_current.note, "");
        storage_data->app_previous.timestamp = 0;
        storage_data->app_previous.addr      = APP2_FLASH_ADDRESS;
        storage_data->app_previous.size      = 0;

        storage_data->app_current.id            = 1;
        storage_data->app_current.version_minor = 0;
        storage_data->app_current.version_major = 0;
        storage_data->app_current.version_patch = 0;
        storage_data->app_current.enabled       = APP_INFO_ENABLED_FLAG;
        strcpy(storage_data->app_current.note, "init version");
        storage_data->app_current.timestamp = HAL_GetTick();
        storage_data->app_current.addr      = APP1_FLASH_ADDRESS;
        storage_data->app_current.size      = APP1_FLASH_END_ADDRESS - APP1_FLASH_ADDRESS;
    } else {
        memset((void *)storage_data, 0, sizeof(app_partition_t));
        storage_data->app_previous.id            = 1;
        storage_data->app_previous.version_minor = 0;
        storage_data->app_previous.version_major = 0;
        storage_data->app_previous.version_patch = 0;
        storage_data->app_previous.enabled       = 0;
        strcpy(storage_data->app_current.note, "");
        storage_data->app_previous.timestamp = 0;
        storage_data->app_previous.addr      = APP1_FLASH_ADDRESS;
        storage_data->app_previous.size      = 0;

        storage_data->app_current.id            = 2;
        storage_data->app_current.version_minor = 0;
        storage_data->app_current.version_major = 0;
        storage_data->app_current.version_patch = 0;
        storage_data->app_current.enabled       = APP_INFO_ENABLED_FLAG;
        strcpy(storage_data->app_current.note, "init version");
        storage_data->app_current.timestamp = HAL_GetTick();
        storage_data->app_current.addr      = APP2_FLASH_ADDRESS;
        storage_data->app_current.size      = APP2_FLASH_END_ADDRESS - APP2_FLASH_ADDRESS;
    }
    write_partition_info(storage_data);
}

void app_rollback()
{
    app_info_t cur = {0};
    app_info_t pre = {0};

    read_app_current(&cur);
    read_app_previous(&pre);
    cur.enabled = 0; // 禁用当前app
    write_app_current(&pre);
    write_app_previous(&cur);
    boot_swap_bank();
    mcu_restart();
}

void app_init()
{
    app_partition_t app_partition = {0};
    read_partition_info(&app_partition);

    if (app_partition.app_current.enabled == APP_INFO_ENABLED_FLAG) {
        // 打印app_partition.app_current成员信息
        if (app_partition.app_previous.enabled == APP_INFO_ENABLED_FLAG) {
            printf("\r\n[previous app info]");
            print_app_info(&app_partition.app_previous);
            printf("\r\n");
        }
        printf("\r\n[current app info]");
        print_app_info(&app_partition.app_current);
        printf("\r\n");
    } else if (app_partition.app_previous.enabled == APP_INFO_ENABLED_FLAG) {
        // app1 disabled, enable app2
        printf("\r\nfound valid previous app, rollback.");
        app_rollback();
    } else {
        printf("\r\napp not initialized.");
        printf("\r\nreset app info.");
        // 重置app_partition内容并写入存储器
        reset_app_info(&app_partition);
    }
    printf("\r\nlaunch app.");
    printf("\r\n");
}
