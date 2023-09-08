
#ifndef _HOST_PROTOCOL_H_
#define _HOST_PROTOCOL_H_

#define HOST_PROTOCOL_TASK_NAME       "HOST_RPINT_TASK"
#define HOST_MESSAGE_QUEUE_SIZE       (100) // (1024 * 2)
#define HOST_PROTOCOL_TASK_STACK_SIZE (1024 * 5)
#define HOST_PROTOCOL_TASK_PRIORITY   (3)

#define HOST_PROTOCOL_DEBUG_ENABLED   (1)

/**
 * @brief 与上位机交互的task
 *
 * @param pvParameters
 */
void host_protocol_task(void *pvParameters);

#endif // _HOST_PROTOCOL_H_