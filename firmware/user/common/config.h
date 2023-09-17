
#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

typedef uint8_t bool;
#define true          1
#define false         0
#define NULL          ((void *)0)

#define MAC_ADDR_LEN  (6)

#define RAM_HEAP_SIZE (120 * 1024)
// #define CCMRAM_HEAP_SIZE     (64 * 1024)

#define RAM_START_ADDRESS    ((uint8_t *)0x20000000)
#define RAM_SIZE             (128 * 1024)
#define CCMRAM_START_ADDRESS ((uint8_t *)0x10000000)
#define CCMRAM_SIZE          (64 * 1024)

#define CCMRAM               __attribute__((section(".ccmram")))
#define RAM                  __attribute__((section(".ram")))

#define APP_VERSION_MAJOR    (1)
#define APP_VERSION_MINOR    (0)
#define APP_VERSION_BUILD    (9)

#define HOST_ENABLED         (1) // 串口发送数据给上位机
#define MQTT_ENABLED         (1) // 4G通讯功能使能
#define GNSS_ENABLED         (1) // 采集GPS数据使能
#define UART_CONFIG_ENABLED  (1) // 用于上位机配置网关参数功能

//------------------------------------------------ 系统监控参数 --------------------------------------------------------------------------
/* 所有模块重启的最小间隔时间，当GPS模块速度几乎为0时重启，单位：分钟 */
#define MIN_RESTART_ALL_INTERVAL_MINUTES (4 * 60)
/* 重启时GPS模块速度最大值，超过该值不重启，单位：KM */
#define RESTART_GNSS_SPEED_MAX (5)
/* 所有模块必须重启间隔时间（防止车辆一直在运行或者GNSS模块故障速度一直很大导致一直不能重启情况），单位：分钟 */
#define MUST_RESTART_ALL_INTERVAL_MINUTES (8 * 60)
/* BLE模块监测无传感器数据的时间，超过该时间只重启ESP32，单位：分钟 */
#define BLE_RESTART_NO_DATA_MINUTE (10)
//--------------------------------------------------- END --------------------------------------------------------------------------------

#define TASK_STACK_DEPTH (1024 * 10)

#endif // _COMMON_CONFIG_H_
