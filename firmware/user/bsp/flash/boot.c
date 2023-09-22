/*
 * @Author: 橘崽崽啊 2505940811@qq.com
 * @Date: 2023-09-21 12:21:15
 * @LastEditors: 橘崽崽啊 2505940811@qq.com
 * @LastEditTime: 2023-09-22 15:31:40
 * @FilePath: \firmware\user\bsp\flash\boot.c
 * @Description: 针对stm32 option byte读写的驱动代码
 * 
 * Copyright (c) 2023 by 橘崽崽啊 2505940811@qq.com, All Rights Reserved. 
 */
#include "main.h"
#include "boot.h"
#include "log/log.h"
#include "bsp/mcu/mcu.h"

#define TAG "BOOT"

error_t boot_configure_boot_from_bootloader(void)
// boot mode configuration
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

// boot mode configuration
error_t boot_configure_boot_from_flash(void)
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

    // set nBOOT0
    if ((userconfig & FLASH_OPTR_nBOOT0_Msk) != OB_nBOOT0_SET) {
        userconfig |= OB_nBOOT0_SET;
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

/************************************************************
 * @brief boot_jump_to_bootloader
 * 跳转到系统 BootLoader。
 * 地址是0x1FFF 000 ~ 0x1FFF 77FF。
 * 系统存储区是用户不能访问的区域。
 * 它在芯片出厂时已经固化了启动程序。
 * 它负责实现串口、USB以及CAN等isp烧录功能。
 ************************************************************/
void boot_jump_to_bootloader(void)
{
    LOGD(TAG, "boot from bootloader");
    boot_configure_boot_from_bootloader();
}
