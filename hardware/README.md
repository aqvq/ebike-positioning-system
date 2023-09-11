<!--
 * @Date: 2023-08-31 15:27:37
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 15:51:14
 * @FilePath: \ebike-positioning\hardware\README.md
-->
# README

本项目使用立创EDA制作。

created by Shang Wentao

## 硬件选型

- STM32G0B0RET6: OTA, 动态注册, APP分区（Flash需求量大，RAM需求量大）
- TYPEC供电5V: 4G模块使用
- LM1117转3.3V: MCU使用
- AT24C64: 存储设备数据
- EC800M: GPS, BDS, 4G
- SIM卡槽: 4G模块联网（连接阿里云物联网平台）
- 电源指示灯，4G指示灯，网络指示灯，GPS指示灯
- 预留SWD烧录接口，UART调试串口
