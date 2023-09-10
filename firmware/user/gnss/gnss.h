
#ifndef _GNSS_H_
#define _GNSS_H_

#include "aiot_at_api.h"
#include "bsp/at/ec800m_at_api.h"
#include "minmea.h"
#include "FreeRTOS.h"
#include "queue.h"

#define AGPS_ENABLE (0)

/// @brief GNSS初始化
/// @param
void gnss_init(void);

/// @brief 使用AT指令查询GPS数据时，响应的回调函数；在【quectel_ec200s_tcp.c】文件中调用
/// @param rsp
/// @return
at_rsp_result_t gnss_nmea_rsp_handler(char *rsp);

/// @brief 订阅GNSS NEMA格式数据
/// @param subscriber_queue
/// @return
int8_t subscribe_gnss_nmea_data(QueueHandle_t subscriber_queue);

#define GNSS_TASK_NAME       "gnss_task"
#define GNSS_TASK_STACK_SIZE (2048)
#define GNSS_TASK_PRIORITY   (1)

// GNSS NEMA格式数据
typedef struct gnss_nmea_data {
    struct minmea_sentence_gsa gsa_frame;
    struct minmea_sentence_gga gga_frame;
    struct minmea_sentence_rmc rmc_frame;
    struct minmea_sentence_vtg vtg_frame;
} gnss_nmea_data_t;

void gnss_init(void);

#endif // _GNSS_H_
