## README

本文件夹代码实现AT驱动层，包括

- at_basic 基础at指令
- at_device 设备相关的控制命令
- at_gnss_agps GNSS AGPS相关命令
- at_gnss_apflash GNSS APFLASH相关命令
- at_gnss_autogps GNSS AUTOGPS相关命令
- at_gnss_nema GNSS NEMA相关命令
- at_gnss GNSS相关命令
- at_tcp TCP相关命令
- at.h 所有at命令都要在此处声明函数，供外部统一调用，外部模块调用at指令仅需要包含此文件即可
- hal_adapter AT串口驱动层适配接口
- os_freertos_impl AT系统驱动层适配接口

> at命令实现代码大多大同小异，参考一个文件，例如`at_gnss_agps`内的函数实现过程即可
> 更多at指令以及at指令详细内容参考`EC800M AT命令手册`
