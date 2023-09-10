#include "main.h"
#include "boot.h"

// boot mode configuration
error_t boot_configure_boot_mode(void)
{
    FLASH_OBProgramInitTypeDef optionsbytesstruct;
    error_t check_flag = OK;

    HAL_FLASHEx_OBGetConfig(&optionsbytesstruct);
    uint32_t userconfig = optionsbytesstruct.USERConfig;

    // set nBOOT_SEL
    if ((userconfig & FLASH_OPTR_nBOOT_SEL_Msk) != OB_BOOT0_FROM_OB) {
        userconfig |= OB_BOOT0_FROM_OB;
        check_flag = DATA_MODIFIED;
    }

    // set nBOOT1
    if ((userconfig & FLASH_OPTR_nBOOT1_Msk) != OB_BOOT1_SYSTEM) {
        userconfig |= OB_BOOT1_SYSTEM;
        check_flag = DATA_MODIFIED;
    }

    // reset nBOOT0
    if ((userconfig & FLASH_OPTR_nBOOT0_Msk) != OB_nBOOT0_RESET) {
        userconfig &= ~OB_nBOOT0_SET;
        check_flag = DATA_MODIFIED;
    }

    // write configuration
    if (check_flag == DATA_MODIFIED) {
        optionsbytesstruct.USERConfig = userconfig;
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        HAL_FLASHEx_OBProgram(&optionsbytesstruct);
        HAL_FLASH_OB_Launch();
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
    return check_flag;
}

// enable dual bank
error_t boot_enable_dual_bank(void)
{
    FLASH_OBProgramInitTypeDef optionsbytesstruct;
    error_t check_flag = OK;

    HAL_FLASHEx_OBGetConfig(&optionsbytesstruct);
    uint32_t userconfig = optionsbytesstruct.USERConfig;

    // set dual bank
    if ((userconfig & FLASH_OPTR_DUAL_BANK_Msk) != OB_USER_DUALBANK_ENABLE) {
        userconfig |= OB_USER_DUALBANK_ENABLE;
        check_flag = DATA_MODIFIED;
    }

    // write configuration
    if (check_flag == DATA_MODIFIED) {
        optionsbytesstruct.USERConfig = userconfig;
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        HAL_FLASHEx_OBProgram(&optionsbytesstruct);
        HAL_FLASH_OB_Launch();
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
    return check_flag;
}

// ob swap bank
error_t boot_swap_bank(void)
{
    FLASH_OBProgramInitTypeDef optionsbytesstruct;
    error_t check_flag = OK;

    HAL_FLASHEx_OBGetConfig(&optionsbytesstruct);
    uint32_t userconfig = optionsbytesstruct.USERConfig;

    // flip swap bank bit
    userconfig ^= FLASH_OPTR_nSWAP_BANK_Msk;
    check_flag = DATA_MODIFIED;

    // write configuration
    if (check_flag == DATA_MODIFIED) {
        optionsbytesstruct.USERConfig = userconfig;
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        HAL_FLASHEx_OBProgram(&optionsbytesstruct);
        HAL_FLASH_OB_Launch();
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
    return check_flag;
}

uint8_t boot_get_current_bank(void)
{
    FLASH_OBProgramInitTypeDef optionsbytesstruct;
    HAL_FLASHEx_OBGetConfig(&optionsbytesstruct);
    uint32_t userconfig = optionsbytesstruct.USERConfig;
    return ((userconfig & FLASH_OPTR_nSWAP_BANK_Msk) == OB_USER_DUALBANK_SWAP_ENABLE);
}
