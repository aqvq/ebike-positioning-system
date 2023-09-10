/********************** NOTES **********************************************
To use this module, the following steps should be followed :

1- in the _OS_Config.h file (ex. FreeRTOSConfig.h) enable the following macros :
    - #define configUSE_IDLE_HOOK    1
    - #define configUSE_TICK_HOOK    1

2- in the _OS_Config.h define the following macros :
    - #define traceTASK_SWITCHED_IN()extern void StartIdleMonitor(void); \
              OStartIdleMonitor()
    - #define traceTASK_SWITCHED_OUT() extern void EndIdleMonitor(void); \
            		EndIdleMonitor()
*******************************************************************************/


/* Includes ------------------------------------------------------------------*/
//#include "cpu_utils.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

TaskHandle_t  xIdleHandle = NULL;
__IO uint32_t osCPU_Usage = 0;
uint32_t     osCPU_IdleStartTime = 0;
uint32_t     osCPU_IdleSpentTime = 0;
uint32_t     osCPU_TotalIdleTime = 0;

/* Private functions ---------------------------------------------------------*/
/**
* @briefApplication Idle Hook
* @paramNone
* @retval None
*/
void vApplicationIdleHook(void)
{
    if( xIdleHandle == NULL )
    {
        /* Store the handle to the idle task. */
        xIdleHandle = xTaskGetCurrentTaskHandle();
    }
}

/**
* @briefApplication Idle Hook
* @paramNone
* @retval None
*/
#define CALCULATION_PERIOD  1000
void vApplicationTickHook (void)
{
    static int tick = 0;

    if(tick ++ > CALCULATION_PERIOD)
    {
        tick = 0;

        if(osCPU_TotalIdleTime > 1000)
        {
            osCPU_TotalIdleTime = 1000;
        }
        osCPU_Usage = (100 - (osCPU_TotalIdleTime * 100) / CALCULATION_PERIOD);
        osCPU_TotalIdleTime = 0;
    }
}

/**
* @briefStart Idle monitor
* @paramNone
* @retval None
*/
void StartIdleMonitor (void)
{
    if( xTaskGetCurrentTaskHandle() == xIdleHandle )
    {
        osCPU_IdleStartTime = xTaskGetTickCountFromISR();
    }
}

/**
* @briefStop Idle monitor
* @paramNone
* @retval None
*/
void EndIdleMonitor (void)
{
    if( xTaskGetCurrentTaskHandle() == xIdleHandle )
    {
        /* Store the handle to the idle task. */
        osCPU_IdleSpentTime = xTaskGetTickCountFromISR() - osCPU_IdleStartTime;
        osCPU_TotalIdleTime += osCPU_IdleSpentTime;
    }
}

/**
* @briefStop Idle monitor
* @paramNone
* @retval None
*/
uint16_t osGetCPUUsage (void)
{
    return (uint16_t)osCPU_Usage;
}

