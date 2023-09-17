
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "util.h"
#include "common/config.h"
#include "aiot_state_api.h"
#include "storage/storage.h"
#include "bsp/at/at.h"

#define TAG "UTILITY"

uint8_t is_telink_mac(const uint8_t *mac)
{
    return (0xa4 == mac[0] && 0xc1 == mac[1] && 0x38 == mac[2]) ? 1 : 0;
}

uint8_t str_start_with(const char *str, const char *start)
{
    while (*start != '\0') {
        if (*str != *start) {
            return 0;
        }
        str++;
        start++;
    }
    return 1;
}

uint8_t str_end_with(const char *str, const char *end)
{
    int len     = strlen(str);
    int end_len = strlen(end);
    if (len < end_len) {
        return 0;
    }
    while (end_len > 0) {
        if (*str != *end) {
            return 0;
        }
        str++;
        end++;
        end_len--;
    }
    return 1;
}

uint8_t is_mac_equal(const uint8_t *mac_a, const uint8_t *mac_b)
{
    for (uint8_t i = 0; i < MAC_ADDR_LEN; i++) {
        if (mac_a[i] != mac_b[i]) {
            return 0;
        }
    }
    return 1;
}

uint8_t is_str_empty(const char *str, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        if (str[i] != 0) {
            return 0;
        }
    }
    return 1;
}

uint8_t is_str_equal(const char *str_a, const char *str_b, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        if (str_a[i] != str_b[i]) {
            return 0;
        }
    }
    return 1;
}

void mac_to_str(const uint8_t *mac, const char delimiter, char *output, uint8_t *output_len)
{
    if (delimiter == 0) {
        *output_len = MAC_ADDR_LEN * 2;
        sprintf((char *)output, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        *output_len = MAC_ADDR_LEN * 2 + 5;
        sprintf((char *)output, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X", mac[0], delimiter, mac[1],
                delimiter, mac[2],
                delimiter, mac[3],
                delimiter, mac[4],
                delimiter, mac[5]);
    }
}

char *get_device_name()
{
    static char device_name[20] = {0};
    if (device_name[0] != 0) {
        return device_name;
    }

    devinfo_wl_t device = {0};
    int8_t err          = read_device_info(&device);
    if (err == OK) {
        if (device.device_name[0] == 0xFF || device.device_name[0] == 0) {
            if (ec800m_at_imei(device_name) == STATE_SUCCESS && device_name[0] != 0) {
                return device_name;
            }
        }
        strcpy(device_name, device.device_name);
        return device_name;
    } else {
        return NULL;
    }
}

uint8_t CRC8(uint8_t *ptr, uint8_t len)
{
    uint8_t crc;
    uint8_t i;
    crc = 0;
    while (len--) {
        crc ^= *ptr++;
        for (i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;
            } else
                crc <<= 1;
        }
    }
    return crc;
}

float mean(float *data, uint16_t len)
{
    if (data == NULL || len <= 0) {
        return 0;
    }

    float sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }

    return sum / len;
}

int hex_string_to_byte(const char *in, uint16_t len, uint8_t *out)
{
    if (len == 0) {
        return 0;
    }
    char *str = (char *)pvPortMalloc(len);
    memset(str, 0, len);
    memcpy(str, in, len);
    for (int i = 0; i < len; i += 2) {
        // 小写转大写
        if (str[i] >= 'a' && str[i] <= 'f')
            str[i] = str[i] & ~0x20;
        if (str[i + 1] >= 'a' && str[i] <= 'f')
            str[i + 1] = str[i + 1] & ~0x20;
        // 处理第前4位
        if (str[i] >= 'A' && str[i] <= 'F')
            out[i / 2] = (str[i] - 'A' + 10) << 4;
        else
            out[i / 2] = (str[i] & ~0x30) << 4;
        // 处理后4位, 并组合起来
        if (str[i + 1] >= 'A' && str[i + 1] <= 'F')
            out[i / 2] |= (str[i + 1] - 'A' + 10);
        else
            out[i / 2] |= (str[i + 1] & ~0x30);
    }
    vPortFree(str);
    return 0;
}
