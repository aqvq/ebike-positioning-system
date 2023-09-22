## README

本文件夹实现IoT通信相关协议。包括连接IoT平台、断开连接IoT平台、发送数据至IoT平台、向Iot平台发送日志等。

`iot_helper`是`app_main`连接阿里云IoT的最直接的模块，之后会调用`aliyun/aliyun_protocol`模块，进行后续的初始化、连接、发送等操作。

`iot_interface`抽象了连接IoT平台（不限于阿里云IoT）的函数，以便于后续迁移其他云平台。
