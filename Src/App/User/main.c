#include "includes.h"


void vTaskDemoCode( void * pvParameters );
void vTaskLightCode( void * pvParameters );

int main (void)
{
    HAL_Init();
    BSP_SystemClkCfg();
    BSP_Init();
    Comm_Init();
    EEPROM_Init();
    Light_Init();
    check_init();
    DIDO_Init();
    
    BaseType_t err;
    TaskHandle_t xHandleDemo;
    err = xTaskCreate( vTaskDemoCode,"demo",256,NULL,2,&xHandleDemo);
    assert(err == pdPASS);
    
    TaskHandle_t xHandleComm;
    err = xTaskCreate( vTaskCommCode,"comm",256,NULL,3,&xHandleComm);
    assert(err == pdPASS);

    TaskHandle_t xHandleLight;
    err = xTaskCreate( vTaskLightCode,"light",256,NULL,4,&xHandleLight);
    assert(err == pdPASS);
    
    vTaskStartScheduler();
    return 0;
}


void vTaskDemoCode( void * pvParameters )
{
    (void)pvParameters;
    //uint8_t buf[10] = {0x01,0x12,0x23,0x34,0x45,0x56,0x67,0x78,0x89,0x90};
    
    while(1)
    {
        //Comm_Send(buf , sizeof(buf));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vTaskLightCode( void * pvParameters )
{
    (void)pvParameters;
    Set_Lighteness(WHITE , 200);
    uint16_t num = 0;
    
    uint16_t interval = 500;
    static uint32_t last_control = 0;
    uint32_t light = 0;
    while(1)
    {
        light = Get_Light();
        /* 根据车流量计算闪烁屏幕*/
        if((xTaskGetTickCount() - last_control) > CONTROL_INTERVAL * 60 * 1000)
        {
            last_control = xTaskGetTickCount();
            num = get_car_num();
            if(num < 5)
                interval = 500;
            else if(num < 15)
                interval = 375;   
            else 
                interval = 250;   
        }

        if(interval == 0) //长亮,隧道关闭，红灯长亮
        {
            Set_Lighteness(YELLOW , 0);
            Set_Lighteness(WHITE , 0);
            if(light < 100)
                Set_Lighteness(RED , 100);
            else if(light < 300)
                Set_Lighteness(RED , 300);
            else if(light < 800)
                Set_Lighteness(RED , 600);
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else if(interval == 500) //车流量小,白灯1Hz
        {
            Set_Lighteness(RED , 0);
            Set_Lighteness(YELLOW , 0);
            if(Get_Lighteness(WHITE))
                Set_Lighteness(WHITE , 0);
            else
            {
                if(light < 100)
                    Set_Lighteness(WHITE , 100);
                else if(light < 300)
                    Set_Lighteness(WHITE , 300);
                else
                    Set_Lighteness(WHITE , 600);
            }
            vTaskDelay(pdMS_TO_TICKS(interval));
        }
        else if(interval == 375)//车流量中,黄灯1.5Hz
        {
            Set_Lighteness(RED , 0);
            Set_Lighteness(WHITE , 0);
            if(Get_Lighteness(YELLOW))
                Set_Lighteness(YELLOW , 0);
            else
            {
                if(light < 100)
                    Set_Lighteness(YELLOW , 100);
                else if(light < 300)
                    Set_Lighteness(YELLOW , 300);
                else
                    Set_Lighteness(YELLOW , 600);
            }
            vTaskDelay(pdMS_TO_TICKS(interval));            
        }
        else if(interval == 250)//车流量大,黄灯2Hz
        {
            Set_Lighteness(RED , 0);
            Set_Lighteness(WHITE , 0);
            if(Get_Lighteness(YELLOW))
                Set_Lighteness(YELLOW , 0);
            else
            {
                if(light < 100)
                    Set_Lighteness(YELLOW , 100);
                else if(light < 300)
                    Set_Lighteness(YELLOW , 300);
                else
                    Set_Lighteness(YELLOW , 600);
            }
            vTaskDelay(pdMS_TO_TICKS(interval));            
        }
            
    }
}