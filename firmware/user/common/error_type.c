/*
 * @Date: 2023-08-31 15:54:24
 * @LastEditors: ShangWentao shangwentao
 * @LastEditTime: 2023-08-31 17:06:48
 * @FilePath: \firmware\user\common\error_type.c
 */

#include "error_type.h"

#if ERROR_USING_STRING_TABLE
// 错误码的字符串常量表
const char *error_string_table[] = {
#define e(x) #x
    error_list
#undef e
};
#endif
