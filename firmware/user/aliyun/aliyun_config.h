
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

#define ALIYUN_VERSION_V1 (1) /* 老的阿里云平台 */
#define ALIYUN_VERSION_V2 (2) /* 新的阿里云平台 */

#define ALIYUN_VERSION    ALIYUN_VERSION_V1

#if (ALIYUN_VERSION == ALIYUN_VERSION_V1)
#define MQTT_HOST      "iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define MQTT_PORT      443
#define PRODUCT_KEY    "a1LBtd7UqnS"
#define PRODUCT_SECRET "fZbJPOMNU0tZVuDN"
#elif (ALIYUN_VERSION == ALIYUN_VERSION_V2)
#define MQTT_HOST      "iot-060a0d0l.mqtt.iothub.aliyuncs.com"
#define MQTT_PORT      1883
#define PRODUCT_KEY    "i69elwfQwzS"
#define PRODUCT_SECRET "ifz6s7RmZuk8Sq8e"
#endif

#endif // _ALIYUN_CONFIG_H_
