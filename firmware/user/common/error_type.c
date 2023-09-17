
#include "common/error_type.h"

#if ERROR_USING_STRING_TABLE
// 错误码的字符串常量表
const char *error_string_table[] = {
#define e(x) #x
    error_list
#undef e
};
#endif
