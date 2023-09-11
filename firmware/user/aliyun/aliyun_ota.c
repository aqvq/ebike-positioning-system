/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:09:36
 * @FilePath: \firmware\user\protocol\aliyun\aliyun_ota.c
 */

#include "aiot_mqtt_api.h"
#include "aiot_at_api.h"
#include "bsp/at/at.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "aiot_ota_api.h"
#include "aiot_mqtt_download_api.h"
#include "core_mqtt.h"
#include "log/log.h"
#include "aliyun_ota.h"
#include "mqtt_download_private.h"
#include "upgrade/iap.h"
#include "storage/storage.h"
#include "upgrade/iap.h"
#include "data/partition_info.h"
#include "error_type.h"
#include "utils/macros.h"
#include "bsp/mcu/mcu.h"
#include "bsp/at/at.h"
#include "utils/time.h"
#include "bsp/mcu/mcu.h"
#include "bsp/flash/boot.h"

static const char *TAG = "ALIYUN_OTA";

//--------------------------全局变量-----------------------------------
// 关闭EC200模块并重启ESP32, 定义在main中
extern void ec800m_poweroff_and_mcu_restart(void);

extern int g_major_version; /* 当前程序major版本 */
extern int g_minor_version; /* 当前程序minor版本 */
extern int g_patch_version; /* 当前程序patch版本 */

/* OTA升级标志，定义在main.c中 */
extern uint8_t g_app_upgrade_flag;
extern void *mqtt_handle;
// mqtt_download实例，不为NULL表示当前有OTA任务
void *g_dl_handle        = NULL;
uint8_t ota_success_flag = 0;

static void *ota_handle               = NULL;
static uint16_t ota_version_first_no  = 0;
static uint16_t ota_version_second_no = 0;
static uint16_t ota_version_third_no  = 0;
static uint32_t ota_app_size          = 0;

static void ota_mcu_restart(void)
{
    ec800m_poweroff_and_mcu_restart();
}

static void aliyun_ota_task(void *pvParameters);
static void report_version(char *version);

/* 下载收包回调, 用户调用 aiot_download_recv() 后, SDK收到数据会进入这个函数, 把下载到的数据交给用户 */
/* TODO: 一般来说, 设备升级时, 会在这个回调中, 把下载到的数据写到Flash上 */
void user_download_recv_handler(void *handle, const aiot_mqtt_download_recv_t *packet, void *userdata)
{
    LOGD(TAG, "ota_download_recv_handler");
    uint32_t data_buffer_len = 0;
    error_t err              = OK;

    /* 目前只支持 packet->type 为 AIOT_DLRECV_HTTPBODY 的情况 */
    if (!packet || AIOT_MDRECV_DATA_RESP != packet->type) {
        return;
    }

    // 写数据
    data_buffer_len = packet->data.data_resp.data_size;

    err = iap_write(packet->data.data_resp.data, data_buffer_len);

    if (err != OK) {
        iap_deinit();
        LOGE(TAG, "write to flash failed[offset=%d], size=%d", packet->data.data_resp.offset, data_buffer_len);
        /* 升级失败了，要重启嘛？重启前 mqtt要不要断开链接？*/
        ota_mcu_restart();
    } else {
        LOGI(TAG, "download %03d%% done, +%d bytes", packet->data.data_resp.percent, data_buffer_len);
    }
}

/* 用户通过 aiot_ota_setopt() 注册的OTA消息处理回调, 如果SDK收到了OTA相关的MQTT消息, 会自动识别, 调用这个回调函数 */
static void user_ota_recv_handler(void *ota_handle, aiot_ota_recv_t *ota_msg, void *userdata)
{
    // LOGD(TAG, "user_ota_recv_handler");
    uint32_t request_size = 0;
    switch (ota_msg->type) {
        case AIOT_OTARECV_FOTA: {
            // LOGD(TAG, "AIOT_OTARECV_FOTA");
            if (NULL == ota_msg->task_desc || ota_msg->task_desc->protocol_type != AIOT_OTA_PROTOCOL_MQTT) {
                break;
            }

            if (g_dl_handle != NULL) {
                LOGE(TAG, "already have task to download return...");
                /* 代表有下载任务，直接返回即可 */
                break;
                // aiot_mqtt_download_deinit(&g_dl_handle);
            }

            error_t err;

            // OTA版本号相关
            sscanf(ota_msg->task_desc->version, "%d.%d.%d", &ota_version_first_no, &ota_version_second_no, &ota_version_third_no);
            ota_app_size = ota_msg->task_desc->size_total;
            LOGW(TAG, "==== OTA target firmware version: %d.%d.%d, size: %u Bytes ====\r\n", ota_version_first_no, ota_version_second_no, ota_version_third_no, ota_msg->task_desc->size_total);

#if 0
        // OTA版本小于当前运行的版本，则不执行OTA升级
        if ((ota_version_first_no < cur_version_first_no) ||
            (ota_version_first_no == cur_version_first_no && ota_version_second_no < cur_version_second_no) ||
            (ota_version_first_no == cur_version_first_no && ota_version_second_no == cur_version_second_no && ota_version_third_no <= cur_version_third_no))
        {
            LOGW(TAG, "the version is unreasonable");
            iap_deinit();
            break;
        }
#endif
            // 开始OTA
            LOGD(TAG, "begin ota");
            app_info_t config = {0};
            read_app_current(&config);
            assert(storage_check(&config) == OK);
            err = iap_init((config.id == 1 ? 2 : 1));
            if (err != OK) {
                LOGE(TAG, "esp_ota_begin failed (%s)", error_string(err));
                iap_deinit();
                break;
            }

            void *md_handler = aiot_mqtt_download_init();

            if (NULL == md_handler) {
                LOGE(TAG, "aiot_mqtt_download_init failed");
                iap_deinit();
                break;
            }

            if (aiot_mqtt_download_setopt(md_handler, AIOT_MDOPT_TASK_DESC, ota_msg->task_desc) < STATE_SUCCESS) {
                LOGE(TAG, "AIOT_MDOPT_TASK_DESC failed");
                // 结束下载会话
                aiot_mqtt_download_deinit(&g_dl_handle);
                g_dl_handle = NULL;
                // 取消OTA升级
                iap_deinit();
                break;
            }

            /* 设置下载一包的大小，对于资源受限设备可以调整该值大小 */
            request_size = 1024 * 10; // 不知道为何，此处不设置request_size值，request_size会等于0.
            if (aiot_mqtt_download_setopt(md_handler, AIOT_MDOPT_DATA_REQUEST_SIZE, &request_size) < STATE_SUCCESS) {
                LOGE(TAG, "AIOT_MDOPT_DATA_REQUEST_SIZE failed");
                // 结束下载会话
                aiot_mqtt_download_deinit(&g_dl_handle);
                g_dl_handle = NULL;
                // 取消OTA升级
                iap_deinit();
                break;
            }

            /* 部分场景下，用户如果只需要下载文件的一部分，即下载指定range的文件，可以设置文件起始位置、终止位置。
             * 若设置range区间下载，单包报文的数据有CRC校验，但SDK将不进行完整文件MD5校验，
             * 默认下载全部文件，单包报文的数据有CRC校验，并且SDK会对整个文件进行md5校验 */
            // uint32_t range_start = 10, range_end = 50 * 1024 + 10;
            // aiot_mqtt_download_setopt(md_handler, AIOT_MDOPT_RANGE_START, &range_start);
            // aiot_mqtt_download_setopt(md_handler, AIOT_MDOPT_RANGE_END, &range_end);

            /* 这个设置的回调函数会由任务 "tMqttDemoRecv" 进行调用 */
            if (aiot_mqtt_download_setopt(md_handler, AIOT_MDOPT_RECV_HANDLE, user_download_recv_handler) < STATE_SUCCESS) {
                LOGE(TAG, "AIOT_MDOPT_RECV_HANDLE failed");
                // 结束下载会话
                aiot_mqtt_download_deinit(&g_dl_handle);
                g_dl_handle = NULL;
                // 取消OTA升级
                iap_deinit();
                break;
            }

            g_dl_handle = md_handler;

            // 创建任务
            if (xTaskCreate(aliyun_ota_task, "aliyun_ota_task", 5120, NULL, 3, NULL) != pdPASS) {
                LOGE(TAG, "create aliyun_ota_task failed, ota cancel");

                // 结束下载会话
                aiot_mqtt_download_deinit(&g_dl_handle);
                g_dl_handle = NULL;
                // 取消OTA升级
                iap_deinit();
            }
            break;
        }
        default:
            break;
    }
}

static void aliyun_ota_task(void *pvParameters)
{
    /* 创建信号量，用于写入flash和下载的同步 */
    int res;

    /* g_dl_handle != NULL的时候 代表有下载任务 */
    if (NULL != g_dl_handle) {
        g_app_upgrade_flag = 1;

        /* 等待下载结果 */
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(10));
            aiot_mqtt_process(mqtt_handle);
            aiot_mqtt_recv(mqtt_handle);
            if (g_dl_handle != NULL) {
                res = aiot_mqtt_download_process(g_dl_handle);

                if (STATE_MQTT_DOWNLOAD_SUCCESS == res) {
                    /* 升级成功，这里重启并且上报新的版本号 */
                    LOGI(TAG, "mqtt download ota success");
                    break;
                } else if (STATE_MQTT_DOWNLOAD_FAILED_RECVERROR == res ||
                           STATE_MQTT_DOWNLOAD_FAILED_TIMEOUT == res ||
                           STATE_MQTT_DOWNLOAD_FAILED_MISMATCH == res) {
                    LOGE(TAG, "mqtt download ota failed, res=%d", res);
                    break;
                }
            }
        }
        LOGD(TAG, "download process ok???");
        // LOGW(TAG, "total_size=%d, write_size=%d.", g_stOtaDownloadInfo.total_size, g_stOtaDownloadInfo.write_size);

        /* 判断文件的所有段是否已经下载完成了, 并且已经写入到flash中了 */
        if (/*g_stOtaDownloadInfo.total_size <= g_stOtaDownloadInfo.write_size && */
            STATE_MQTT_DOWNLOAD_SUCCESS == res) {
            LOGI(TAG, "file download ok!");
            aiot_mqtt_download_deinit(&g_dl_handle);
            g_dl_handle = NULL;

            error_t err;
            err = iap_deinit();
            if (OK != err) {
                LOGE(TAG, "esp_ota_end failed (%s)!", error_string(err));
            } else {
                /* 设置下次启动分区 */
                app_info_t app_info = {0};
                read_app_current(&app_info);
                assert(storage_check(&app_info) == OK);
                app_info.enabled       = APP_INFO_ENABLED_FLAG;
                app_info.version_major = ota_version_first_no;
                app_info.version_minor = ota_version_second_no;
                app_info.version_patch = ota_version_third_no;
                app_info.size          = ota_app_size;
                app_info.timestamp     = get_timestamp();
                if (app_info.id == 1) {
                    app_info.id   = 2;
                    app_info.addr = APP2_FLASH_ADDRESS;
                } else if (app_info.id == 2) {
                    app_info.id   = 1;
                    app_info.addr = APP1_FLASH_ADDRESS;
                } else {
                    LOGE(TAG, "app_info.id error!");
                }
                strcpy(app_info.note, "aliyun ota");
                err = iap_update_partition(&app_info);
                if (err != OK) {
                    LOGE(TAG, "iap_set_boot_partition failed!");
                } else {
                    /* 上报升级后的版本号 */
                    char version[20] = {0};
                    sprintf(version, "%d.%d.%d", ota_version_first_no, ota_version_second_no, ota_version_third_no);
                    report_version(version);
                    ota_success_flag = 1;
                    LOGI(TAG, "Prepare to restart system!");
                }
            }
        } else // 可能是取消升级
        {
            iap_deinit();
            LOGW(TAG, "iap_deinit!!!");
        }
        if (ota_success_flag == 1) {
            ota_success_flag = 0;
            boot_swap_bank();
        }
        ota_mcu_restart();
    }

    /* 升级正常或异常结束 */
    aiot_mqtt_download_deinit(&g_dl_handle);
    g_dl_handle        = NULL;
    g_app_upgrade_flag = 0;

    vTaskDelete(NULL);
}

int32_t aliyun_ota_init(void *mqtt_handle)
{
    LOGD(TAG, "aliyun_ota_init");
    if (!mqtt_handle) {
        return -1;
    }

    /* 与MQTT例程不同的是, 这里需要增加创建OTA会话实例的语句 */
    ota_handle = aiot_ota_init();
    if (NULL == ota_handle) {
        LOGE(TAG, "aiot_ota_init failed");
        return -2;
    }

    /* 用以下语句, 把OTA会话和MQTT会话关联起来 */
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_MQTT_HANDLE, mqtt_handle);

    /* 用以下语句, 设置OTA会话的数据接收回调, SDK收到OTA相关推送时, 会进入这个回调函数 */
    aiot_ota_setopt(ota_handle, AIOT_OTAOPT_RECV_HANDLER, user_ota_recv_handler);

    /* 上报当前设备的版本号 */
    char cur_version[20] = {0};
    sprintf(cur_version, "%d.%d.%d", g_major_version, g_minor_version, g_patch_version);
    report_version(cur_version);

    return 0;
}

/* 向物联网平台报告版本号 */
static void report_version(char *version)
{
    int32_t res = aiot_ota_report_version(ota_handle, version);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "report version failed, code is -0x%04X\r\n", -res);
    } else {
        LOGI(TAG, "report version(%s) success.", version);
    }
}

int32_t aliyun_ota_deinit(void)
{
    if (NULL == ota_handle) {
        return -1;
    }

    int32_t res = aiot_ota_deinit(&ota_handle);
    if (res < STATE_SUCCESS) {
        LOGE(TAG, "aiot_ota_deinit failed: -0x%04X\r\n", -res);
        return -2;
    }

    return 0;
}
