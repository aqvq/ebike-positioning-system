/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 18:45:44
 * @FilePath: \firmware\user\protocol\host\host_protocol.h
 * @Description: 实现host协议功能头文件
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef _HOST_PROTOCOL_H_
#define _HOST_PROTOCOL_H_

// host任务名
#define HOST_PROTOCOL_TASK_NAME "HOST_RPINT_TASK"
// host消息队列长度
#define HOST_MESSAGE_QUEUE_SIZE (100) // (1024 * 2)
// host任务栈大小
#define HOST_PROTOCOL_TASK_STACK_SIZE (1024 * 5)
// host任务优先级
#define HOST_PROTOCOL_TASK_PRIORITY (3)

/**
 * @brief 与上位机交互的task
 *
 * @param pvParameters
 */
void host_protocol_task(void *pvParameters);

#endif // _HOST_PROTOCOL_H_