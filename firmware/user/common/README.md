## README

此文件夹存放一些包含配置信息的头文件。

例如FreeRTOS的配置文件(FreeRTOSConfig.h)、项目的一些配置信息(config.h)等。

此外还有error_type的定义，此文件用于更好的管理和显示错误信息。当需要添加新的错误类型时，只需要在error_type.h中添加即可，通过error_string可以打印相关错误码的宏定义字符串。error_type使用X-MACRO技术实现。
