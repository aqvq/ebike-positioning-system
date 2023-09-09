#ifndef __BOOT_OB_H
#define __BOOT_OB_H

#include "error_type.h"

error_t boot_configure_boot_mode(void);
error_t boot_enable_dual_bank(void);
error_t boot_swap_bank(void);
uint8_t boot_get_current_bank(void);

#endif