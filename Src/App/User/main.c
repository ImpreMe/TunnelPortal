#include "includes.h"

void vTaskCode( void * pvParameters );

int main (void)
{
    HAL_Init();
    BSP_SystemClkCfg();
    BSP_Init();
        
    TaskHandle_t xHandle;
    BaseType_t err = xTaskCreate( vTaskCode,"demo",256,NULL,2,&xHandle);
    assert(err == pdPASS);
    vTaskStartScheduler();
    return 0;
}

void vTaskCode( void * pvParameters )
{
    (void)pvParameters;   
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

