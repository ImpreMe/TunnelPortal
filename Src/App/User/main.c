#include "includes.h"


void vTaskDemoCode( void * pvParameters );
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
    TaskHandle_t xHandleDemo;
    err = xTaskCreate( vTaskDemoCode,"demo",256,NULL,2,&xHandleDemo);
    assert(err == pdPASS);
    
    TaskHandle_t xHandleComm;
    err = xTaskCreate( vTaskCommCode,"comm",256,NULL,3,&xHandleComm);
    assert(err == pdPASS);

    TaskHandle_t xHandleLight;
    err = xTaskCreate( vTaskLightCode,"light",256,NULL,4,&xHandleLight);
    assert(err == pdPASS);
    
    xConfig_mutex = xSemaphoreCreateMutex();
    assert(xConfig_mutex != NULL);
    
    xReset_seam = xSemaphoreCreateBinary();
    assert(xReset_seam != NULL);
    
    vTaskStartScheduler();
    return 0;
}


void vTaskDemoCode( void * pvParameters )
{
    (void)pvParameters;
    
//    uint8_t ttt[20] = {0};
//    for(int i = 0 ; i < sizeof(ttt) ; i++)
//        ttt[i] = i + 0x90;
//    
//    uint8_t rrr[20] = {0};
//    
//    EEPROM_Write(500,ttt,sizeof(ttt));
//    EEPROM_Read(500,rrr,sizeof(rrr));
    
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void Get_mode_from_num(uint16_t num , uint8_t array[])
{
    if(num < 5)
    {
        array[0] = 2;
        array[1] = 0;
        array[2] = 0;
    }
    else if(num < 15)
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

uint16_t Get_value_from_light(uint32_t light_value)
{
    return 500;
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
    
    uint8_t light_mode[3] = {1,1,1};
    
    uint8_t iCount[3] = {0};
    while(1)
    {
        if(Get_Config(&temp_config))
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        light = Get_Light(); //获取当前环境亮度
        num   = get_car_num(); //获取当前车流量统计信息      
        /* 根据车流量计算闪烁频率*/
        if((xTaskGetTickCount() - last_control) > temp_config.control_period * 60 * 1000)
        {
            last_control = xTaskGetTickCount();
            Get_mode_from_num(num,light_mode);
        }              

        if(((xTaskGetTickCount() - temp_config.manual_tick) > temp_config.manual_duration * 60 * 1000 ) && (temp_config.control_mode == 1))
        {
            temp_config.control_mode = 0;
            Set_Config(temp_config);
        }
        uint8_t mode[3] = {0xFF , 0xFF , 0xFF};

        for(int i = 0 ; i < 3 ; i++)
        {
            if(temp_config.control_mode == 1) //手动模式
                mode[i] = temp_config.lamp_mode[i];
            else
                mode[i] = light_mode[i];
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
                    Set_Lighteness((LightType_t)i , Get_value_from_light(light));
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
                            Set_Lighteness((LightType_t)i , Get_value_from_light(light));
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
                            Set_Lighteness((LightType_t)i , Get_value_from_light(light));
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
                            Set_Lighteness((LightType_t)i , Get_value_from_light(light));
                    }                        
                }
                break;
                default : break;
            }
        }                       
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

        
        
        
        
        
//        if((xTaskGetTickCount() - last_control) > CONTROL_INTERVAL * 60 * 1000)
//        {
//            last_control = xTaskGetTickCount();
//            
//            if(num < 5)
//                interval = 500;
//            else if(num < 15)
//                interval = 375;   
//            else 
//                interval = 250;
//        }
//
//        if(interval == 0) //长亮,隧道关闭，红灯长亮
//        {
//            Set_Lighteness(YELLOW , 0);
//            Set_Lighteness(WHITE , 0);
//            if(light < 100)
//                Set_Lighteness(RED , 100);
//            else if(light < 300)
//                Set_Lighteness(RED , 300);
//            else if(light < 800)
//                Set_Lighteness(RED , 600);
//            
//            vTaskDelay(pdMS_TO_TICKS(100));
//        }
//        else if(interval == 500) //车流量小,白灯1Hz
//        {
//            Set_Lighteness(RED , 0);
//            Set_Lighteness(YELLOW , 0);
//            if(Get_Lighteness(WHITE))
//                Set_Lighteness(WHITE , 0);
//            else
//            {
//                if(light < 100)
//                    Set_Lighteness(WHITE , 100);
//                else if(light < 300)
//                    Set_Lighteness(WHITE , 300);
//                else
//                    Set_Lighteness(WHITE , 600);
//            }
//            vTaskDelay(pdMS_TO_TICKS(interval));
//        }
//        else if(interval == 375)//车流量中,黄灯1.5Hz
//        {
//            Set_Lighteness(RED , 0);
//            Set_Lighteness(WHITE , 0);
//            if(Get_Lighteness(YELLOW))
//                Set_Lighteness(YELLOW , 0);
//            else
//            {
//                if(light < 100)
//                    Set_Lighteness(YELLOW , 100);
//                else if(light < 300)
//                    Set_Lighteness(YELLOW , 300);
//                else
//                    Set_Lighteness(YELLOW , 600);
//            }
//            vTaskDelay(pdMS_TO_TICKS(interval));            
//        }
//        else if(interval == 250)//车流量大,黄灯2Hz
//        {
//            Set_Lighteness(RED , 0);
//            Set_Lighteness(WHITE , 0);
//            if(Get_Lighteness(YELLOW))
//                Set_Lighteness(YELLOW , 0);
//            else
//            {
//                if(light < 100)
//                    Set_Lighteness(YELLOW , 100);
//                else if(light < 300)
//                    Set_Lighteness(YELLOW , 300);
//                else
//                    Set_Lighteness(YELLOW , 600);
//            }
//            vTaskDelay(pdMS_TO_TICKS(50));
//        }
//            
//    }
//}