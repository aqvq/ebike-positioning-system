/**
 * @file aiot_at_api.h
 * @brief AT模块头文件, 提供对接AT模组的能力
 * @date 2019-12-27
 *
 * @copyright Copyright (C) 2015-2018 Alibaba Group Holding Limited
 *
 */

#ifndef _AIOT_AT_API_H_
#define _AIOT_AT_API_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include "core_list.h"
#include "aiot_sysdep_api.h"


#define MODULE_NAME_AT                      "AT"
/**
 * @brief -0x1000~-0x10FF表达SDK在AT模块内的状态码
 */
#define STATE_AT_BASE                       (-0x1000)
#define STATE_AT_ALREADY_INITED             (-0x1003)
#define STATE_AT_NOT_INITED                 (-0x1004)
#define STATE_AT_UART_TX_FUNC_MISSING       (-0x1005)
#define STATE_AT_GET_RSP_FAILED             (-0x1006)
#define STATE_AT_RINGBUF_OVERFLOW           (-0x1007)
#define STATE_AT_RX_TIMEOUT                 (-0x1008)
#define STATE_AT_NO_AVAILABLE_LINK          (-0x1009)
#define STATE_AT_TX_TIMEOUT                 (-0x100A)
#define STATE_AT_TX_ERROR                   (-0x100B)
#define STATE_AT_RINGBUF_NO_DATA            (-0x100C)
#define STATE_AT_UART_TX_FUNC_NULL          (-0x100D)
#define STATE_AT_NO_DATA_SLOT               (-0x100F)
#define STATE_AT_LINK_IS_DISCONN            (-0x1010)
#define STATE_AT_UART_TX_FAILED             (-0x1011)

/**
 * @brief AT组件配置参数
 */
#define AIOT_AT_CMD_LEN_MAXIMUM             (128)       /* AT命令最大长度 */
#define AIOT_AT_RSP_LEN_MAXIMUM             (128)       /* AT应答最大长度 */
#define AIOT_AT_TX_TIMEOUT_DEFAULT          (10000)      /* UART默认发送超时时间 */
#define AIOT_AT_RX_TIMEOUT_DEFAULT          (10000)      /* UART默认接受等待超时时间 */
#define AIOT_AT_DATA_RB_SIZE_DEFAULT        (4096)       /* 内部网络数据接收缓冲区大小,用户可根据接收数据流量的大小调整 */
#define AIOT_AT_RSP_RB_SIZE_DEFAULT         (4096)       /* 内部应答报文接受缓冲区大小,用户可根据接收数据流量的大小调整 */
#define AIOT_AT_CMD_RETRY_TIME              (3)         /* AT命令发送重试最大次数 */
#define AIOT_AT_RINGBUF_RETRY_INTERVAL      (5)     /* RINGBUF数据长度检查间隔时间 */
#define AIOT_AT_SOCKET_NUM                  (5)         /* 支持的数据链路数量 */
#define AIOT_AT_MAX_PACKAGE_SIZE            (1460)      /* 一帧报文最大的数据长度*/
#define AT_SOCKET_ID_START                  (1)

/**
 * @brief AT组件配置选项
 *
 */
typedef enum {
    AIOT_ATOPT_UART_TX_FUNC,
    AIOT_ATOPT_USER_DATA,
    AIOT_ATOPT_DEVICE,
    AIOT_ATOPT_MAX,
} aiot_at_option_t;

/**
 * @brief 串口发送回调函数定义
 *
 * @param[in] p_data
 * @param[in] len
 * @param[in] timeout
 *
 * @return int32_t
 * @retval length, 实际发送的字节数
 * @retval 0, 发送超时
 * @retval -1, 发送失败
 */
typedef int32_t (*aiot_at_uart_tx_func_t)(const uint8_t *p_data, uint16_t len, uint32_t timeout);


typedef enum rsp_result_t {
    AT_RSP_SUCCESS,              /* AT命令执行成功 */
    AT_RSP_WAITING,              /* 继续等待命令回执*/
    AT_RSP_FAILED,               /* AT命令执行失败，可能是超时也可能是返回错误*/
} at_rsp_result_t;

/**
 * @brief 接受到应答数据后的用户处理回调函数原型定义
 *
 * @param[in] rsp       AT命令的应答数据
 *
 * @return rsp_result_t
 */
typedef at_rsp_result_t (*at_rsp_handler_t)(char *rsp);
/**
 * @brief 接受到应答数据后的用户处理回调函数原型定义
 *
 * @param[in] line       AT命令的一行数据
 *
 * @return int32_t
 * @retval >= 0,
 * @retval < 0,
 */
typedef void (*at_urc_handler_t)(char *line);
/**
 * @brief AT命令请求响应结构体定义
 */
typedef struct {
    /* 要发送的AT命令格式，当AT命令需要输入参数时使用 */
    char *fmt;
    /* 要发送的AT命令，当设置的AT命令不需要输入参数时使用 */
    char *cmd;
    /* AT命令的数据长度，不需要用户赋值 */
    uint32_t cmd_len;
    /* 期望的应答数据，默认处理匹配到该字符串认为命令执行成功 */
    char *rsp;
    /* 得到应答的超时时间，达到超时时间为执行失败， 默认10000，即10秒*/
    uint32_t timeout_ms;
    /* 接受到应答数据后的用户自定义处理回调函数 */
    at_rsp_handler_t handler;
} core_at_cmd_item_t;

/**
 * @brief AT主动上报命令
 */
typedef struct {
    /* 主动上报数据匹配头 */
    char *prefix;
    /* 接受到应答数据后的用户处理回调函数 */
    at_urc_handler_t handle;
} core_at_urc_item_t;

/**
 * @brief AT主动上报接收的数据，可识别头
 */
typedef struct {
    /* 匹配头标识符，匹配到即表示后面有数据上报 */
    char *prefix;
    /* 匹配包体，应该包含socket_id和data_len, 后面接数据 */
    char *fmt;
} core_at_recv_data_prefix;


/**
 * @brief AT设备结构化数据
 */
typedef struct {
    /* 模组初始化命令列表， 有默认值 */
    core_at_cmd_item_t *module_init_cmd;
    uint32_t           module_init_cmd_size;

    /* 网络初始化命令列表 */
    core_at_cmd_item_t *ip_init_cmd;
    uint32_t           ip_init_cmd_size;

    /* 打开socket网络列表 */
    core_at_cmd_item_t *open_cmd;
    uint32_t           open_cmd_size;

    /* 发送数据命令列表 */
    core_at_cmd_item_t *send_cmd;
    uint32_t           send_cmd_size;

    /* 模组主动上报数据标识符，识别到数据上报，会进入数据接收流程 */
    core_at_recv_data_prefix *recv;

    /* 关闭socket命令列表 */
    core_at_cmd_item_t *close_cmd;
    uint32_t           close_cmd_size;

    /* 错误标识符，控制命令返回数据中，识别到错误标识符会返回执行失败 */
    char               *error_prefix;

    /* 订阅模组主动上报处理,可选 */
    core_at_urc_item_t *urc_register;
    uint32_t            urc_register_size;

    /* 开启ssl命令列表 ，可选*/
    core_at_cmd_item_t *ssl_cmd;
    uint32_t           ssl_cmd_size;
} at_device_t;


/**
 * @brief ringbuf结构体定义
 */
typedef struct {
    uint8_t *buf;
    uint8_t *head;
    uint8_t *tail;
    uint8_t *end;
    uint32_t size;
    void *mutex;
} core_ringbuf_t;

/**
 * @brief 模组数据链路状态
 */
typedef enum {
    CORE_AT_LINK_DISABLE    = 0,
    CORE_AT_LINK_DISCONN    = 1,
    CORE_AT_LINK_CONN       = 2,
} core_at_link_status_t;

/**
 * @brief 数据链路上下文结构体
 */
typedef struct {
    core_at_link_status_t link_status;
    core_ringbuf_t data_rb;
} link_descript_t;

typedef struct {
    uint32_t curr_link_id;
    uint32_t data_len;
    uint32_t remain_len;
} core_at_read_t;

/*
* 模组IP状态定义
*/
typedef enum {
    IP_DEINIT,
    IP_CSQ_ERROR,
    IP_CARD_ERROR,
    IP_ERR_OTHER,
    IP_SUCCESS,
} core_ip_status_t;

/**
 * @brief AT组件上下文数据结构体
 */
typedef struct {
    /* 是否初始化 */
    uint8_t is_init;
    /* 模组网络状态 */
    core_ip_status_t        ip_status;
    /* 模组socket连接数据 */
    link_descript_t fd[AIOT_AT_SOCKET_NUM];
    /* AT命令发送超时 */
    uint32_t tx_timeout;
    /* ip数据接收处理上下文 */
    core_at_read_t reader;
    /* 当前处理的命令 */
    const core_at_cmd_item_t *cmd_content;
    /* AT命令返回数据缓存<--接收到一行写入 */
    core_ringbuf_t rsp_rb;
    /* 缓存一行数据 */
    char rsp_buf[AIOT_AT_RSP_LEN_MAXIMUM];
    /* 缓存一行数据偏移 */
    uint32_t rsp_buf_offset;
    /* 串口发送接口，由外部设置*/
    aiot_at_uart_tx_func_t uart_tx_func;
    /* AT指令发送锁*/
    void *cmd_mutex;
    /* 网络数据发送锁*/
    void *send_mutex;
    void *user_data;
    /* AT设备 */
    at_device_t *device;
} core_at_handle_t;

/**
 * @brief 初始化AT组件
 *
 * @return int32_t
 * @retval =0 初始化成功
 * @retval <0 初始化失败
 */
int32_t aiot_at_init(void);

/**
 * @brief 配置AT组件, 主要为配置串口发送回调函数
 *
 * @param opt 配置项
 * @param data 配置数据
 *
 * @return int32_t
 * @retval =0 配置成功
 * @retval <0 配置失败
 */
int32_t aiot_at_setopt(aiot_at_option_t opt, void *data);

/**
 * @brief 模组启动, 内部发送模组相关的AT命令已启动模组, 启动完成后, 模组将获取到IP地址
 *
 * @return int32_t
 * @retval =0 启动成功
 * @retval <0 启动失败
 */
int32_t aiot_at_bootstrap(void);

/**
 * @brief 为对应的链路ID创建ringbuf资源
 *
 * @param socket_id 链路ID
 *
 * @return int32_t
 * @retval =0 操作成功
 * @retval <0 操作失败
 */
int32_t aiot_at_nwk_open(uint8_t *socket_id);

/**
 * @brief 内部会调用模组相关的AT命令, 通过串口发送TCP建连命令。可对接到core_sysdep_network_establish网络接口
 *
 * @param socket_id 链路ID
 * @param host 主机名
 * @param port 端口号
 * @param timeout 建连超时时间
 *
 * @return int32_t
 * @retval >=0 建连成功
 * @retval <0 建连失败
 */
int32_t aiot_at_nwk_connect(uint8_t socket_id, const char *host, uint16_t port, uint32_t timeout);

/**
 * @brief 内部会调用模组相关的AT命令, 通过串口发送TCP数据。可对接到core_sysdep_network_send网络接口
 *
 * @param socket_id 链路ID
 * @param buffer 指向外部数据缓冲区的指针
 * @param len 数据长度
 * @param timeout 发送超时时间
 *
 * @return int32_t
 * @retval >=0 已发送的数据长度
 * @retval <0 发送失败
 */
int32_t aiot_at_nwk_send(uint8_t socket_id, const uint8_t *buffer, uint32_t len, uint32_t timeout);

/**
 * @brief 内部会从对应的数据ringbu中读取网络数据, 可对接到core_sysdep_network_recv网络接口
 *
 * @param socket_id 链路ID
 * @param buffer 指向外部数据缓冲区的指针
 * @param len 外部数据缓冲区大小
 * @param timeout_ms 接收超时时间
 *
 * @return int32_t
 * @retval >=0 读取到的数据长度
 * @retval <0 读取失败
 */
int32_t aiot_at_nwk_recv(uint8_t socket_id, uint8_t *buffer, uint32_t len, uint32_t timeout_ms);

/**
 * @brief 内部会调用模组相关的AT命令, 关闭连接链路。可对接到core_sysdep_network_deinit网络接口
 *
 * @param socket_id 链路ID
 *
 * @return int32_t
 * @retval =0 操作成功
 * @retval <0 操作失败
 */
int32_t aiot_at_nwk_close(uint8_t socket_id);

/**
 * @brief 为AT组件提供串口数据
 *
 * @param[in] data 接收的数据
 * @param[in] size 接收的数据长度
 * @return int32_t
 * @retval >=0 操作成功,返回已经消费掉的数据
 * @retval <0 操作失败
 */
int32_t aiot_at_hal_recv_handle(uint8_t *data, uint32_t size);
/**
 * @brief 反初始化AT组件
 *
 * @return int32_t
 */
int32_t aiot_at_deinit(void);

int32_t core_at_ip_status(core_ip_status_t status);
int32_t core_at_socket_status(uint32_t id, core_at_link_status_t status);
#if defined(__cplusplus)
}
#endif


#endif
