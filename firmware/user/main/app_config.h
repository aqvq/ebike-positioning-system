
#ifndef _APP_CONFIG_H_
#define _APP_CONFIG_H_

#define HOST_ENABLED                (0) // 串口发送数据给上位机
#define MQTT_ENABLED                (1) // 4G通讯功能使能
#define GNSS_ENABLED                (0) // 采集GPS数据使能
#define UART_GATEWAY_CONFIG_ENABLED (0) // 用于上位机配置网关参数功能

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

#endif //_APP_CONFIG_H_
