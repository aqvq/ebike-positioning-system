如果Boot From Bank2选项激活（就支持此功能的产品而言），自举程序执行Dual Boot机

制，如图“STM32xxxx的双存储区自举实现”中所示，其中STM32xxxx是相关的STM32产

品（示例：*图* *40*）；否则，执行自举程序选择方案，如图“STM32xxxx的自举程序VY.x选

择”所示，其中STM32xxxx是相关的STM32产品（示例：*图* *21*）。****



除了上述模式之外，用户可通过从用户代码跳转到系统存储器来执行自举程序。跳转到自举

程序之前，用户必须：

• 

禁用所有外设时钟

• 

禁用所用的PLL

• 

禁用中断

• 

清空挂起的中断



![image-20230909110159666](C:/Users/shang/AppData/Roaming/Typora/typora-user-images/image-20230909110159666.png)

Flash memory address: 0x1FFF 7800

Reset value: 0xDFFF E1AA (ST production value)

![image-20230909110341410](C:/Users/shang/AppData/Roaming/Typora/typora-user-images/image-20230909110341410.png)

![image-20230909110354252](C:/Users/shang/AppData/Roaming/Typora/typora-user-images/image-20230909110354252.png)

![image-20230909110427805](C:/Users/shang/AppData/Roaming/Typora/typora-user-images/image-20230909110427805.png)

- nSWAP_BANK = 1 ==> BANK1 is mapped at 0x0800_0000
- nSWAP_BANK = 0 ==> BANK2 is mapped at 0x0800_0000



配置Boot

```c
void Flash_OB_Handle(void) {
	FLASH_OBProgramInitTypeDef optionsbytesstruct;
	bool UPDATE = false;

	HAL_FLASHEx_OBGetConfig(&optionsbytesstruct);
	uint32_t userconfig = optionsbytesstruct.USERConfig;

	if((userconfig & FLASH_OPTR_nBOOT_SEL_Msk) != OB_BOOT0_FROM_PIN) {
		userconfig &= ~FLASH_OPTR_nBOOT_SEL_Msk;
		userconfig |= OB_BOOT0_FROM_PIN;
		UPDATE = true;
	}

	if(UPDATE) {
		optionsbytesstruct.USERConfig = userconfig;
		HAL_FLASH_Unlock();
		HAL_FLASH_OB_Unlock();
		HAL_FLASHEx_OBProgram(&optionsbytesstruct);
		HAL_FLASH_OB_Launch();
		HAL_FLASH_OB_Lock();
		HAL_FLASH_Lock();
	}
}

```

