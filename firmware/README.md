## 项目介绍

created by Shang Wentao

本项目实现电动自行车固件定位监测系统的固件部分。其主要功能包括但不限于：GPS/BDS定位、动态注册、设备影子、NTP、OTA等。

## 项目结构

```txt
/libraries 依赖库
    /cJSON json解析与打印
    /FreeRTOS rtos
    /LinkSDK 阿里云物联网平台sdk
    /minmea gnss nema解析库
    /STM32G0B0RETx STM32 HAL库
/user 项目应用
    /aliyun 阿里云各种功能与实现
    /bsp 设备驱动实现
        /at at驱动
        /eeprom 存储驱动
        /flash stm32 flash和option bytes读写操作
        /mcu stm32控制驱动
    /common 配置信息头文件
    /data 数据结构抽象
    /gnss gnss功能实现
    /log 日志打印功能实现
    /main 项目应用入口
    /protocol 项目协议实现
        /iot 阿里云协议实现
        /host 上位机协议
        /uart 串口配置协议
    /storage 存储模块
    /upgrade 程序升级模块
    /utils 实用函数
```
