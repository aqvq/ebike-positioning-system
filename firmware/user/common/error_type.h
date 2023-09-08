
#ifndef __ERROR_TYPE_H
#define __ERROR_TYPE_H

#include <stdint.h>

// 是否使用string_table来获取相应错误码的字符串
// 如果错误码定义过多可能会消耗些许内存来存储字符串
#define ERROR_USING_STRING_TABLE (1)

// 【统一在此处添加新的错误代码】
// - 描述：该模块会自动分配错误码和相应的字符串值
// - 用法：error(错误常量)
#define error_list error(OK),                             \
                   error(STORAGE_INTERFACE_NULL),         \
                   error(STORAGE_MEMCPY_ERROR),           \
                   error(STORAGE_INIT_ERROR),             \
                   error(STORAGE_INTERFACE_INIT_NULL),    \
                   error(READ_GATEWAY_CONFIG_ERROR),      \
                   error(WRITE_GATEWAY_CONFIG_ERROR),     \
                   error(CJSON_PARSE_ERROR),              \
                   error(READ_GATEWAY_CONFIG_TEXT_ERROR), \
                   error(GATEWAY_CONFIG_TEXT_OVERFLOW),   \
                   error(W25QXX_SET_TYPE_ERROR),          \
                   error(W25QXX_SET_INTERFACE_ERROR),     \
                   error(W25QXX_INIT_ERROR),              \
                   error(W25QXX_SET_ADDRESS_MODE_ERROR),  \
                   error(W25QXX_WRITE_ERROR),             \
                   error(W25QXX_READ_ERROR),              \
                   error(ERROR_NULL_POINTER),             \
                   error(NOT_IMPLEMENTED),                \
                   error(ERROR_INVALID_PARAMETER),        \
                   error(ERROR_CREATE_TASK),              \
                   error(FLASH_ERASE_ERROR),              \
                   error(FLASH_WRITE_ERROR),              \
                   error(STORAGE_NO_DATA),                \
                   error(BLE_UNKNOWN_DATA),               \
                   error(ERROR_CJSON_PARSE),              \
                   error(EEPROM_CHECK_ERROR)

// 定义NULL
#ifndef NULL
#define NULL (0)
#endif // !NULL

// 错误代码枚举变量
typedef enum {
#define error(x) x
    error_list
#undef error
} error_t;

// 引用error_string_table
#if ERROR_USING_STRING_TABLE
extern const char *error_string_table[];
#endif

// 获取错误代码的字符串
static inline const char *error_string(error_t error)
{
#if ERROR_USING_STRING_TABLE
    return error_string_table[error];
#else
    return "ERROR";
#endif
}

#endif // !__ERROR_TYPE_H
