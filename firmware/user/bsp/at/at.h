#ifndef __EC800M_AT_API_H
#define __EC800M_AT_API_H

#include <stdio.h>
#include <string.h>
#include "aiot_at_api.h"
#include "error_type.h"
#include "log/log.h"
#include "utils/linked_list.h"
#include "aiot_state_api.h"
#include "utils/macros.h"

extern core_at_handle_t at_handle;

// at device
error_t ec800m_init();
int32_t at_hal_init(void);
int32_t ec800m_at_poweroff();
int32_t ec800m_at_imei(char *imei);
int32_t ec800m_state();

// at tcp
int32_t ec800m_at_tcp_close();
int32_t ec800m_at_context_state(uint8_t *id, uint8_t *state);
int32_t ec800m_at_tcp_state(uint8_t *id, uint8_t *state);

// at gnss
int32_t ec800m_at_gnss_open();
int32_t ec800m_at_gnss_state(uint8_t *state);
int32_t ec800m_at_gnss_location();
int32_t ec800m_at_gnss_close();
int32_t ec800m_at_gnss_config(uint8_t mode);

// at gnss nmea
int32_t ec800m_at_gnss_nmea_enable();
int32_t ec800m_at_gnss_nmea_disable();
int32_t ec800m_at_gnss_nmea_query();

// at gnss agps
int32_t ec800m_at_gnss_enable_agps();
int32_t ec800m_at_gnss_disable_agps();
int32_t ec800m_at_gnss_agps_state(uint8_t *state);

// at gnss apflash
int32_t ec800m_at_gnss_enable_apflash();
int32_t ec800m_at_gnss_disable_apflash();
int32_t ec800m_at_gnss_apflash_state(uint8_t *state);

// at gnss autogps
int32_t ec800m_at_gnss_enable_autogps();
int32_t ec800m_at_gnss_disable_autogps();
int32_t ec800m_at_gnss_autogps_state(uint8_t *state);

#endif
