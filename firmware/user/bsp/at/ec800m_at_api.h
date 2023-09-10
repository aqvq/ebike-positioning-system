#ifndef __EC800M_AT_API_H

#define __EC800M_AT_API_H

#include "aiot_at_api.h"

int32_t ec800m_at_poweroff();
int32_t ec800m_at_gnss_open();
int32_t ec800m_at_gnss_state();
int32_t ec800m_at_context_state();
int32_t ec800m_at_tcp_state();
int32_t ec800m_at_gnss_location();
int32_t ec800m_at_gnss_close();
int32_t ec800m_at_gnss_agps_open();
int32_t ec800m_at_gnss_agps_download();
int32_t ec800m_at_gnss_nema_enable();
int32_t ec800m_at_gnss_nema_disable();
int32_t ec800m_at_gnss_nema_query();
int32_t ec800m_at_tcp_close();

#endif
