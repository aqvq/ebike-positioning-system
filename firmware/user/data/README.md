## README

本文件夹实现项目所需的数据结构。

- device_info 阿里云接入所需的设备信息，如设备ID、设备密钥
- general_message 发送到阿里云端的通用数据结构类型
- gnss_settings gnss配置信息数据结构

这些数据结构操作流程十分相似，只需要理解其中一个即可理解本项目所用到的数据结构读写操作思路，例如`device_info`。

通常都是由以下几个函数组成：

一对结构体读写函数、一对json字符串读写函数、一对json字符串转结构体和结构体转json字符串的解析打印函数：

```c

/// @brief 读取设备信息
/// @param devinfo_wl 信息保存位置
/// @return error_t
error_t read_device_info(devinfo_wl_t *devinfo_wl);

/// @brief 写入设备信息
/// @param devinfo_wl 待写入的设备信息
/// @return error_t
error_t write_device_info(devinfo_wl_t *devinfo_wl);

/// @brief 解析设备信息结构体数据，将json字符串解析为设备信息结构体内容
/// @param input json字符串
/// @param device 待写入的设备信息结构体
/// @return error_t
error_t device_info_parse(char *input, devinfo_wl_t *device);

/// @brief 打印设备信息结构体数据，将设备信息结构体内容打印为json字符串
/// @param config 待打印的设备信息
/// @param output json字符串输出地址
/// @param output_len json字符串长度（可为NULL）
/// @return error_t
error_t device_info_print(devinfo_wl_t *config, char *output, uint16_t *output_len);

/// @brief 从json字符串中读取设备信息
/// @param text json字符串
/// @return error_t
error_t write_device_info_text(const char *text);

/// @brief 以json字符串形式读取设备信息
/// @param text 字符串缓冲区
/// @return error_t
error_t read_device_info_text(char *text);
```
