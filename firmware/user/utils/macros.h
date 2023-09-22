/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:24:28
 * @FilePath: \firmware\user\utils\macros.h
 * @Description: 实用宏函数定义
 *
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved.
 */

#ifndef __MACROS_H
#define __MACROS_H

#include "log/log.h"

/// @brief 获取结构体成员偏移量
/// @param type: 结构体类型
/// @param member: 结构体成员名
/// @return size_t 偏移量
#define offsetof(type, member) ((uint32_t) & (((type *)0)->member))

/// @brief 获取结构体成员大小
/// @param type: 结构体类型
/// @param member: 结构体成员名
/// @return size_t 占用内存大小
#define member_size(type, member) (sizeof(((type *)0)->member))

/// @brief 根据结构体成员获取结构体对象
/// @param ptr: 指针变量，指向结构体成员
/// @param type: 结构体类型
/// @param member: 结构体成员名
/// @return 指针，指向结构体对象
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

/// @brief 将x以y的倍数向上取整，常用于地址对齐
/// @param x
/// @param y
/// @return 取整后的数
#define roundup(x, y) ((((x) + ((y)-1)) / (y)) * (y))

/// @brief 获取数组元素数
/// @param x 数组名
/// @return 元素数量
#define array_size(x) (sizeof(x) / sizeof((x)[0]))

/// @brief 断言，当表达式为假时报错
/// @param x 断言表达式
#define assert(x)                                         \
    do {                                                  \
        if (!(x)) LOGE(TAG, "Assertion Failure: %s", #x); \
    } while (0)

#endif // !__MACROS_H
