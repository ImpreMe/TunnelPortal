#include "includes.h"


void vTaskLoraCode( void * pvParameters );
void vTaskLightCode( void * pvParameters );

SemaphoreHandle_t xConfig_mutex;
SemaphoreHandle_t xReset_seam;




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
    TaskHandle_t xHandleLora;
    err = xTaskCreate( vTaskLoraCode,"lora",256,NULL,4,&xHandleLora);
    assert(err == pdPASS);
    
    TaskHandle_t xHandleComm;
    err = xTaskCreate( vTaskCommCode,"comm",256,NULL,2,&xHandleComm);
    assert(err == pdPASS);

    TaskHandle_t xHandleLight;
    err = xTaskCreate( vTaskLightCode,"light",256,NULL,3,&xHandleLight);
    assert(err == pdPASS);
    
    xConfig_mutex = xSemaphoreCreateMutex();
    assert(xConfig_mutex != NULL);
    
    xReset_seam = xSemaphoreCreateBinary();
    assert(xReset_seam != NULL);
    
    vTaskStartScheduler();
    return 0;
}


void vTaskLoraCode( void * pvParameters )
{
    (void)pvParameters;
    
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Get_mode_from_num(uint16_t num ,Config_t config, uint8_t array[])
{
    if(num < config.threshold1)
    {
        array[0] = 2;
        array[1] = 0;
        array[2] = 0;
    }
    else if(num < config.threshold2)
    {
        array[0] = 0;
        array[1] = 3;
        array[2] = 0;
    }
    else 
    {
        array[0] = 0;
        array[1] = 4;
        array[2] = 0;
    }  
}

void Get_value_from_light(uint32_t light_value ,Config_t config,uint8_t lamp_value[])
{
    (void)config;
    lamp_value[0] = 100;
    lamp_value[1] = 100;
    lamp_value[2] = 100;
}


void vTaskLightCode( void * pvParameters )
{
    (void)pvParameters;
    //Set_Lighteness(WHITE , 200);
    Config_Init();
    
    static uint32_t last_control = 0;
    uint32_t light ;    //当前环境光照强度
    uint16_t num ;      //当前环境车流量数据
    Config_t temp_config;
    
    uint8_t light_mode_auto[3] = {2,0,0}; //自动条件下用来计算临时模式的数组
    uint8_t light_value_auto[3] = {50,50,50}; //自动条件下用来计算临时模式的数组
    
    uint8_t iCount[3] = {0};
    
    uint8_t mode[3] = {0xFF , 0xFF , 0xFF}; //灯光工作模式（统一入口）
    uint8_t value[3] = {50,50,50};          //灯光亮度（统一入口）
    while(1)
    {
        if(Get_Config(&temp_config))
        {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        light = Get_Light(); //获取当前环境亮度
        num   = get_car_num(); //获取当前车流量统计信息      
        /* 根据车流量计算闪烁频率*/
        if((xTaskGetTickCount() - last_control) > temp_config.control_period * 60 * 1000)
        {
            last_control = xTaskGetTickCount();
            Get_mode_from_num(num,temp_config,light_mode_auto);
        }              
        /* 根据光照计算亮度值*/
        Get_value_from_light(light ,temp_config,light_value_auto);
        
        if(((xTaskGetTickCount() - temp_config.manual_tick) > temp_config.manual_duration * 60 * 1000 ) && (temp_config.control_mode == 1))
        {
            temp_config.control_mode = 0;
            Set_Config(temp_config);
        }

        for(int i = 0 ; i < 3 ; i++)
        {
            if(temp_config.control_mode == 1) //手动模式
            {
                mode[i] = temp_config.lamp_mode[i];
                value[i] = temp_config.lamp_value[i];
            }
            else
            {
                mode[i] = light_mode_auto[i];
                value[i] = light_value_auto[i];
            }
        }            

        for(int i = 0 ; i < 3 ; i++)
        {
            switch (mode[i])
            {
                case 0:
                {
                    Set_Lighteness((LightType_t)i , 0);
                }
                break;
                case 1:
                {
                    Set_Lighteness((LightType_t)i , value[i] * 10);
                }
                break;
                case 2:
                {
                    iCount[i]++;
                    if(iCount[i] >= 10)
                    {
                        iCount[i] = 0;
                        if(Get_Lighteness((LightType_t)i))
                            Set_Lighteness((LightType_t)i , 0);
                        else
                            Set_Lighteness((LightType_t)i , value[i] * 10);
                    }
                }
                break;
                case 3:
                {
                    iCount[i]++;
                    if(iCount[i] >= 7)
                    {
                        iCount[i] = 0;
                        if(Get_Lighteness((LightType_t)i))
                            Set_Lighteness((LightType_t)i , 0);
                        else
                            Set_Lighteness((LightType_t)i , value[i] * 10);
                    }                        
                }
                break;
                case 4:
                {
                    iCount[i]++;
                    if(iCount[i] >= 5)
                    {
                        iCount[i] = 0;
                        if(Get_Lighteness((LightType_t)i))
                            Set_Lighteness((LightType_t)i , 0);
                        else
                            Set_Lighteness((LightType_t)i , value[i] * 10);
                    }                        
                }
                break;
                default : break;
            }
        }                       
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
