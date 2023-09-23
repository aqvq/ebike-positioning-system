通过`PVD`实现掉电检测。首先在CUBE配置界面`NVIC`中勾选`PVD`。
然后在`main.c`中定义PVD初始化函数：

```c
void PVD_Init(void)
{
    PWR_PVDTypeDef PvdStruct;

    HAL_PWR_EnablePVD();                        /* 使能PVD */

    PvdStruct.PVDLevel = PWR_PVDLEVEL_6;
    PvdStruct.Mode = PWR_PVD_MODE_IT_RISING;
    HAL_PWR_ConfigPVD(&PvdStruct);

    HAL_NVIC_SetPriority(PVD_VDDIO2_IRQn, 0, 0);       /* 配置PVD中断优先�? */
    HAL_NVIC_EnableIRQ(PVD_VDDIO2_IRQn);               /* 使能PVD中断 */

    pvd_flag=1;
}

```

注意，在上电或者掉电瞬间，电压会有波动，需要设置标志变量，确保紧急任务只在掉电瞬间执行一次:

```c
/* USER CODE BEGIN PV */
uint8_t pvd_flag=0;
/* USER CODE END PV */
```

在`main`函数初始化阶段调用`PVD_Init`。

在`stm32xx_it.c`中找到`PVD Handler`，添加内容如下：

```c
/**
  * @brief This function handles PVD through EXTI line 16, PVM (monit. VDDIO2) through EXTI line 34.
  */
void PVD_VDDIO2_IRQHandler(void)
{

  /* USER CODE BEGIN PVD_VDDIO2_IRQn 0 */
    if(__HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ) && pvd_flag)
    {
        // 这里执行紧急任务
	    HAL_UART_Transmit(&huart5, urgmsg, 128, 0xFFFF);

      	// 标志复位
	    pvd_flag = 0;
    }
  /* USER CODE END PVD_VDDIO2_IRQn 0 */
  HAL_PWREx_PVD_PVM_IRQHandler();
  /* USER CODE BEGIN PVD_VDDIO2_IRQn 1 */

  /* USER CODE END PVD_VDDIO2_IRQn 1 */
}
```

在`stm32xx_it.c`顶部引用外部变量`pvd_flag`：

```c
extern uint8_t pvd_flag;
```

经测试代码工作正常。