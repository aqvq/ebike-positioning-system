/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-21 22:32:42
 * @FilePath: \firmware\user\aliyun\aliyun_config.h
 * @Description: 阿里云实例配置参数：MQTT_HOST、PRODUCT_KEY、PRODUCT_SECRET
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#ifndef _ALIYUN_CONFIG_H_
#define _ALIYUN_CONFIG_H_

/*
    对于企业实例, 或者2021年07月30日之后（含当日）开通的物联网平台服务下公共实例
    mqtt_host的格式为"${YourInstanceId}.mqtt.iothub.aliyuncs.com"
    其中${YourInstanceId}: 请替换为您企业/公共实例的Id

    对于2021年07月30日之前（不含当日）开通的物联网平台服务下公共实例
    需要将mqtt_host修改为: mqtt_host = "${YourProductKey}.iot-as-mqtt.${YourRegionId}.aliyuncs.com"
    其中, ${YourProductKey}：请替换为设备所属产品的ProductKey。可登录物联网平台控制台，在对应实例的设备详情页获取。
    ${YourRegionId}：请替换为您的物联网平台设备所在地域代码, 比如cn-shanghai等
    该情况下完整mqtt_host举例: a1TTmBPIChA.iot-as-mqtt.cn-shanghai.aliyuncs.com

    详情请见: https://help.aliyun.com/document_detail/147356.html
*/
#define MQTT_HOST      "iot-06z00d470n691my.mqtt.iothub.aliyuncs.com"
#define MQTT_PORT      443
#define PRODUCT_KEY    "j1ng3QrOeu2"
#define PRODUCT_SECRET "Ri5VlQYRkEXJx1HK"

#endif // _ALIYUN_CONFIG_H_
