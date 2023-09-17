#ifndef __GNSS_SETTINGS_H
#define __GNSS_SETTINGS_H

#include "common/error_type.h"

typedef struct
{
    uint8_t gnss_upload_strategy;
    uint8_t gnss_offline_strategy;
    uint8_t gnss_ins_strategy;
    // gnss_mode:
    // bit 0 1 2 3 4 5 6 7
    // bit0: gnss agps enable/disable
    // bit1: gnss apflash enable/disable
    // bit2: gnss autogps enable/disable
    // bit345: gnss mode config @ref ec800m_at_gnss_config
    // bit67: reserved
    uint8_t gnss_mode;
}gnss_settings_t;

#define GNSS_AGPS_MSK          (1 << 0) ///< 是否开启agps
#define GNSS_APFLASH_MSK       (1 << 1) ///< 是否开启apflash
#define GNSS_AUTOGPS_MSK       (1 << 2) ///< 是否开启autogps
#define GNSS_MODE_CONFIG_MSK   (7 << 3) ///< GPS模式配置
#define GNSS_AGPS_SHIFT        (0)
#define GNSS_APFLASH_SHIFT     (1)
#define GNSS_AUTOGPS_SHIFT     (2)
#define GNSS_MODE_CONFIG_SHIFT (3)

error_t read_gnss_settings(gnss_settings_t *config);
error_t write_gnss_settings(gnss_settings_t *config);
error_t gnss_settings_parse(char *input, gnss_settings_t *config);
error_t gnss_settings_print(gnss_settings_t *config, char *output, uint16_t *output_len);
error_t write_gnss_settings_text(const char *text);
error_t read_gnss_settings_text(char *text);

#endif
