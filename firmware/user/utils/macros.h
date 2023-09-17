/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:21:44
 * @FilePath: \firmware\user\utils\macros.h
 */

#ifndef __MACROS_H
#define __MACROS_H

#include "log/log.h"

#define offsetof(type, member)          ((uint32_t) & (((type *)0)->member))
#define member_size(type, member)       (sizeof(((type *)0)->member))
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

#define roundup(x, y)    ((((x) + ((y)-1)) / (y)) * (y))
#define array_size(x)    (sizeof(x) / sizeof((x)[0]))

#define assert(x)                                         \
    do {                                                  \
        if (!(x)) LOGE(TAG, "Assertion Failure: %s", #x); \
    } while (0)

#endif // !__MACROS_H
