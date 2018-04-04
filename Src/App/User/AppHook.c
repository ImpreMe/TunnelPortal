#include "FreeRTOS.h"
#include "task.h"

#include "stm32f1xx_hal.h"


void vApplicationTickHook( void )
{
    HAL_IncTick();
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
    (void)xTask;
    (void)pcTaskName;
}




