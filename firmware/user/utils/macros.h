/*
 * @Date: 2023-08-31 15:54:25
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:21:44
 * @FilePath: \firmware\user\utils\macros.h
 */

#ifndef __MACROS_H
#define __MACROS_H

#define offsetof(type, member)          ((uint32_t) & (((type *)0)->member))
#define member_size(type, member)       (sizeof(((type *)0)->member))
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})
#define roundup(x, y) (                  \
    {                                    \
        typeof(y) __y = y;               \
        (((x) + (__y - 1)) / __y) * __y; \
    })

#endif // !__MACROS_H