/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 16:18:50
 * @FilePath: \firmware\user\common\error_type.c
 * @Description: 错误类型字符串表定义文件
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */

#include "common/error_type.h"

// 此文件不需要做任何修改

#if ERROR_USING_STRING_TABLE
// 错误码的字符串常量表
const char *error_string_table[] = {
#define e(x) #x
    error_list
#undef e
};
#endif
