#include "main.h"
#include "boot.h"
#include "log/log.h"

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

#define ENABLE_INT()  __set_PRIMASK(0) /* 使能全局中断 */
#define DISABLE_INT() __set_PRIMASK(1) /* 禁止全局中断 */

void (*SysMemBootJump)(void);        /* 声明一个函数指针 */
__IO uint32_t BootAddr = 0x1FFF0000; /* 系统BootLoader地址 */

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
    uint32_t i = 0;

    /* 关闭全局中断 */
    DISABLE_INT();

    /* 关闭滴答定时器，复位到默认值 */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 设置所有时钟到默认状态， 使用 HSI 时钟 */
    HAL_RCC_DeInit();

    /* 关闭所有中断，清除所有中断挂起标志 */
    for (i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* 使能全局中断 */
    ENABLE_INT();

    /* 跳转到系统 BootLoader，首地址是 MSP，地址+4 是复位中断服务程序地址 */
    SysMemBootJump = (void (*)(void))(*((uint32_t *)(BootAddr + 4)));

    /* 设置主堆栈指针 */
    __set_MSP(*(uint32_t *)BootAddr);

    /* 跳转到系统 BootLoader */
    SysMemBootJump();

    /* 跳转成功的话，不会执行到这里 */
    LOGE(TAG, "jump to bootloader error");
    Error_Handler();
}
