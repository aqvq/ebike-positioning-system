/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:35:23
 * @FilePath: \firmware\user\gnss\gnss.h
 * @Description: gnss功能实现头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef _GNSS_H_
#define _GNSS_H_

#include "aiot_at_api.h"
#include "bsp/at/at.h"
#include "minmea.h"
#include "FreeRTOS.h"
#include "queue.h"

// gnss初始化最大尝试次数
#define GNSS_MAX_RETRY (10)

/// @brief GNSS初始化
/// @param
void gnss_init(void);

/// @brief 使用AT指令查询GPS数据时，响应的回调函数；在"bsp/at/at_gnss_nema"文件中调用
/// @param rsp
/// @return
at_rsp_result_t gnss_nmea_rsp_handler(char *rsp);

/// @brief 订阅GNSS NEMA格式数据
/// @param subscriber_queue 队列句柄
/// @return
int8_t subscribe_gnss_data(QueueHandle_t subscriber_queue);

// gnss任务名
#define GNSS_TASK_NAME "gnss_task"
// gnss任务栈大小
#define GNSS_TASK_STACK_SIZE (2048)
// gnss任务优先级
#define GNSS_TASK_PRIORITY (1)

// GNSS NEMA格式数据
typedef struct gnss_nmea_data {
    struct minmea_sentence_gsa gsa_frame;
    struct minmea_sentence_gga gga_frame;
    struct minmea_sentence_rmc rmc_frame;
    struct minmea_sentence_vtg vtg_frame;
} gnss_nmea_data_t;

#endif // _GNSS_H_
