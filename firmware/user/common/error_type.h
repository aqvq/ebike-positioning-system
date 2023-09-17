
#ifndef __ERROR_TYPE_H
#define __ERROR_TYPE_H

#include <stdint.h>

// 是否使用string_table来获取相应错误码的字符串
// 如果错误码定义过多可能会消耗些许内存来存储字符串
#define ERROR_USING_STRING_TABLE (1)

// 【统一在此处添加新的错误代码】
// - 描述：该模块会自动分配错误码和相应的字符串值
// - 用法：error(错误常量)
#define error_list e(OK),                             \
                   e(STORAGE_INTERFACE_NULL),         \
                   e(STORAGE_MEMCPY_ERROR),           \
                   e(STORAGE_INIT_ERROR),             \
                   e(STORAGE_INTERFACE_INIT_NULL),    \
                   e(READ_GATEWAY_CONFIG_ERROR),      \
                   e(WRITE_GATEWAY_CONFIG_ERROR),     \
                   e(CJSON_PARSE_ERROR),              \
                   e(READ_GATEWAY_CONFIG_TEXT_ERROR), \
                   e(GATEWAY_CONFIG_TEXT_OVERFLOW),   \
                   e(W25QXX_SET_TYPE_ERROR),          \
                   e(W25QXX_SET_INTERFACE_ERROR),     \
                   e(W25QXX_INIT_ERROR),              \
                   e(W25QXX_SET_ADDRESS_MODE_ERROR),  \
                   e(W25QXX_WRITE_ERROR),             \
                   e(W25QXX_READ_ERROR),              \
                   e(ERROR_NULL_POINTER),             \
                   e(NOT_IMPLEMENTED),                \
                   e(ERROR_INVALID_PARAMETER),        \
                   e(ERROR_CREATE_TASK),              \
                   e(FLASH_ERASE_ERROR),              \
                   e(FLASH_WRITE_ERROR),              \
                   e(STORAGE_NO_DATA),                \
                   e(BLE_UNKNOWN_DATA),               \
                   e(ERROR_CJSON_PARSE),              \
                   e(EEPROM_CHECK_ERROR),             \
                   e(FLASH_SWITCH_BANK_ERROR),        \
                   e(DATA_MODIFIED),                  \
                   e(ERROR_IAP_NOT_INIT)

// 定义NULL
#ifndef NULL
#define NULL (0)
#endif // !NULL

// 错误代码枚举变量
typedef enum {
#define e(x) x
    error_list
#undef e
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
