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
        GPIO_InitTypeDef GPIO_InitStruct;
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM1 GPIO Configuration    
        PA8     ------> TIM1_CH1 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);  
        
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**TIM1 GPIO Configuration    
        PA8     ------> TIM1_CH1 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0 | GPIO_PIN_8, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
    
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