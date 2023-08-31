/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "hg24c64.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FLASH_ADDRESS          (0x08000000)
#define FLASH_END_ADDRESS      (0x08100000)         // 1024KB
#define APP_BOOTLOADER_ADDRESS (0x08000000)         // 16KB
#define APP1_FLASH_ADDRESS     (0x08004000)         // 496KB
#define APP2_FLASH_ADDRESS     (0x08080000)         // 512KB
#define APP1_FLASH_END_ADDRESS (APP2_FLASH_ADDRESS) // 不包�??????
#define APP2_FLASH_END_ADDRESS (FLASH_END_ADDRESS)  // 不包�??????
#define APP_INFO_FLASH_SECTOR  FLASH_SECTOR_1
#define APP_INFO_ENABLED_FLAG  (0xCD)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define offsetof(type, member)          ((uint32_t) & (((type *)0)->member))
#define member_size(type, member)       (sizeof(((type *)0)->member))
#define container_of(ptr, type, member) ({             \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); \
})

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
typedef struct {
    int8_t id;          // app编号,合法值为1�??2
    uint32_t addr;      // app地址
    uint32_t timestamp; // app下载时间
    uint32_t size;      // app大小
    // version字段: "0.0.1"，是有三位的版本�??,分别是对应的version里面的：major, minor, patch
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_patch;
    uint8_t note[16];     // app信息
    uint8_t enabled;      // 是否有效
    uint8_t reserved[47]; // 保留
} app_info_t;

typedef struct {
    app_info_t app_current;
    app_info_t app_previous;
} app_partition_t;

app_partition_t g_app_partition;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void jump_to_app(void)
{

    int i = 0;
    // 检查栈顶地址是否合法
    if (((*(uint32_t *)g_app_partition.app_current.addr) & 0x2FFE0000) == 0x20020000) {
        /* 关闭全局中断 */
        __set_PRIMASK(1);

        /** 关闭外设 */
        HAL_UART_DeInit(&huart3);
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);
        HAL_RCC_DeInit();
        HAL_DeInit();

        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;

        for (i = 0; i < 8; i++) {
            NVIC->ICER[i] = 0xFFFFFFFF;
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }

        /* 使能全局中断 */
        __set_PRIMASK(0);

        /* 在RTOS工程，这条语句很重要，设置为特权级模式，使用MSP指针 */
        __set_CONTROL(0);

        void (*app_start)() = *(__IO uint32_t *)(g_app_partition.app_current.addr + 4);

        // 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
        __set_MSP(*(__IO uint32_t *)(g_app_partition.app_current.addr));
        uint32_t offset = g_app_partition.app_current.addr - FLASH_ADDRESS;
        SCB->VTOR       = FLASH_BASE | offset;
        app_start();
    }
    /* 跳转成功的话，不会执行到这里 */
    printf("\r\nlaunch app error.");
    Error_Handler();
}

void print_welcome()
{
    printf("\r\n*************************************************************");
    printf("\r\n*                                                           *");
    printf("\r\n*                     THIS IS BOOTLOADER                    *");
    printf("\r\n*                  SOFTWARE VERSION: 1.0.0                  *");
    printf("\r\n*                  Created By Shang Wentao                  *");
    printf("\r\n*                                                           *");
    printf("\r\n*************************************************************");
    printf("\r\n");
}

void print_time(uint32_t timestamp, char *dest, size_t len)
{
    long days   = 24 * 60 * 60;
    long months = days * 30;
    long years  = months * 12;
    long y, m, d, HH, MM, SS;
    y  = timestamp / years + 1970;
    m  = timestamp % years / months + 1;
    d  = timestamp % years % months / days + 1;
    HH = timestamp % years % months % days / 3600;
    MM = timestamp % years % months % days % 3600 / 60;
    SS = timestamp % years % months % days % 3600 % 60;
    snprintf(dest, len, "%04d/%02d/%02d %02d:%02d:%02d", y, m, d, HH, MM, SS);
}

void print_app_info(app_info_t *app_info)
{
    char date_time[32] = {0};
    printf("\r\npartition id:  %1d", app_info->id);
    printf("\r\napp version:   v%d.%d.%d", app_info->version_major, app_info->version_minor, app_info->version_patch);
    printf("\r\napp size:      %dKB", app_info->size >> 10);
    printf("\r\nboot address:  0x%08X", app_info->addr);
    printf("\r\napp note:      \"%s\"", app_info->note);
    print_time(app_info->timestamp, date_time, sizeof(date_time));
    printf("\r\nupdate time:   %s", date_time);
}
#ifdef DEBUG
void debug_app_info(app_partition_t *storage_data)
{
    memset((void *)storage_data, 0, sizeof(app_partition_t));
    storage_data->app_previous.id            = 2;
    storage_data->app_previous.version_minor = 0;
    storage_data->app_previous.version_major = 0;
    storage_data->app_previous.version_patch = 0;
    storage_data->app_previous.enabled       = 0;
    strcpy(storage_data->app_current.note, "");
    storage_data->app_previous.timestamp = 0;
    storage_data->app_previous.addr      = APP2_FLASH_ADDRESS;
    storage_data->app_previous.size      = 0;

    storage_data->app_current.id            = 1;
    storage_data->app_current.version_minor = 0;
    storage_data->app_current.version_major = 0;
    storage_data->app_current.version_patch = 0;
    storage_data->app_current.enabled       = APP_INFO_ENABLED_FLAG;
    strcpy(storage_data->app_current.note, "init version");
    storage_data->app_current.timestamp = HAL_GetTick();
    storage_data->app_current.addr      = APP1_FLASH_ADDRESS;
    storage_data->app_current.size      = APP1_FLASH_END_ADDRESS - APP1_FLASH_ADDRESS;

    Storage_Write_Buffer(0x00, (uint8_t *)&g_app_partition, sizeof(g_app_partition));
}
#endif
void reset_app_info(app_partition_t *storage_data)
{
    memset((void *)storage_data, 0, sizeof(app_partition_t));
    storage_data->app_previous.id            = 2;
    storage_data->app_previous.version_minor = 0;
    storage_data->app_previous.version_major = 0;
    storage_data->app_previous.version_patch = 0;
    storage_data->app_previous.enabled       = 0;
    strcpy(storage_data->app_current.note, "");
    storage_data->app_previous.timestamp = 0;
    storage_data->app_previous.addr      = APP2_FLASH_ADDRESS;
    storage_data->app_previous.size      = 0;

    storage_data->app_current.id            = 1;
    storage_data->app_current.version_minor = 0;
    storage_data->app_current.version_major = 0;
    storage_data->app_current.version_patch = 0;
    storage_data->app_current.enabled       = APP_INFO_ENABLED_FLAG;
    strcpy(storage_data->app_current.note, "init version");
    storage_data->app_current.timestamp = HAL_GetTick();
    storage_data->app_current.addr      = APP1_FLASH_ADDRESS;
    storage_data->app_current.size      = APP1_FLASH_END_ADDRESS - APP1_FLASH_ADDRESS;

    Storage_Write_Buffer(0x00, (uint8_t *)&g_app_partition, sizeof(g_app_partition));
}

void app_rollback()
{
    app_info_t cur = {0};
    app_info_t pre = {0};
    Storage_Read_Buffer(offsetof(app_partition_t, app_current), &cur, sizeof(cur));
    Storage_Read_Buffer(offsetof(app_partition_t, app_previous), &pre, sizeof(pre));
    cur.enabled = 0; // 禁用当前app
    Storage_Write_Buffer(offsetof(app_partition_t, app_current), &pre, sizeof(pre));
    Storage_Write_Buffer(offsetof(app_partition_t, app_previous), &cur, sizeof(cur));
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART3_UART_Init();
    /* USER CODE BEGIN 2 */
    print_welcome();
    printf("\r\ninit device.");
    if (!Storage_Ready()) {
        printf("\r\ninit storage device failed.");
        Error_Handler();
    }
#ifdef DEBUG
    debug_app_info(&g_app_partition);
#endif
read_app_info:
    printf("\r\nread app info.");
    Storage_Read_Buffer(0x00, (uint8_t *)&g_app_partition, sizeof(g_app_partition));

    if (g_app_partition.app_current.enabled == APP_INFO_ENABLED_FLAG) {
        // 打印g_app_partition.app_current成员信息
        if (g_app_partition.app_previous.enabled == APP_INFO_ENABLED_FLAG) {
            printf("\r\n[previous app info]");
            print_app_info(&g_app_partition.app_previous);
            printf("\r\n");
        }
        printf("\r\n[current app info]");
        print_app_info(&g_app_partition.app_current);
        printf("\r\n");
    } else if (g_app_partition.app_previous.enabled == APP_INFO_ENABLED_FLAG) {
        // app1 disabled, enable app2
        printf("\r\nfound valid previous app, rollback.");
        app_rollback();
        goto read_app_info; // 重新读取app信息
    } else {
        printf("\r\napp not initialized.");
        printf("\r\nreset app info.");
        // 重置g_app_partition内容并写入存储器
        reset_app_info(&g_app_partition);
        printf("\r\ndone.");
        goto read_app_info;
    }
    printf("\r\nlaunch app.");
    jump_to_app();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = 4;
    RCC_OscInitStruct.PLL.PLLN       = 168;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void)
{

    /* USER CODE BEGIN USART3_Init 0 */

    /* USER CODE END USART3_Init 0 */

    /* USER CODE BEGIN USART3_Init 1 */

    /* USER CODE END USART3_Init 1 */
    huart3.Instance          = USART3;
    huart3.Init.BaudRate     = 921600;
    huart3.Init.WordLength   = UART_WORDLENGTH_8B;
    huart3.Init.StopBits     = UART_STOPBITS_1;
    huart3.Init.Parity       = UART_PARITY_NONE;
    huart3.Init.Mode         = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART3_Init 2 */

    /* USER CODE END USART3_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);

    /*Configure GPIO pin : PE15 */
    GPIO_InitStruct.Pin   = GPIO_PIN_15;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /*Configure GPIO pins : PB10 PB11 */
    GPIO_InitStruct.Pin   = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
#ifdef __GNUC__
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, 0xFFFF);
    return len;
}
#else
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF); // 输出指向串口USART3
    return ch;
}
#endif
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
