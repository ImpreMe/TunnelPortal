#include "includes.h"


static TIM_HandleTypeDef htim6;

static void MX_TIM6_Init(void);

void check_init(void)
{
    MX_TIM6_Init();
}

static void MX_TIM6_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();


    /*Configure GPIO pin : PA1 */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
  
    
    __HAL_RCC_TIM6_CLK_ENABLE();
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 72 - 1;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 1000 - 1;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        assert(0);
    }
    HAL_NVIC_SetPriority(TIM6_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM6_IRQn);    
}

//捕获状态
//[7]:0,没有成功的捕获;1,成功捕获到一次.
//[6]:0,还没捕获到上升沿;1,已经捕获到下降沿了.
//[5:0]:捕获上升沿后溢出的次数(对于16位定时器来说,1ms计数器加1,溢出时间:65秒)
volatile uint32_t  Capture_Status=0; //输入捕获状态
static uint16_t Car_num = 0;

//定时器5中断服务函数
void TIM6_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim6);//定时器共用处理函数
}


void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

//车检器脉冲为一低电平脉冲，采集脉冲宽度来计算车流量
//-----------|    |-----------
//           |    |
//           |____|
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_1)
    {
        if((Capture_Status & 0x80000000) != 0)//已经成功获取，还没处理，直接返回
            return;
        
        GPIO_PinState status = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
        if(status == GPIO_PIN_RESET) //下降沿
        {
            Capture_Status = 0;
            Capture_Status |= 0x40000000; //标记捕获到了下降沿
            __HAL_TIM_DISABLE(&htim6);
            __HAL_TIM_SET_COUNTER(&htim6,0);     
            HAL_TIM_Base_Start_IT(&htim6);
        }
        else if((Capture_Status & 0x40000000) && (status == GPIO_PIN_SET)) //捕获到一个上升沿
        {
            Capture_Status |= 0x80000000; //标记成功捕获到一次低电平脉宽
        }
    }
}


uint32_t temp = 0;
void check_poll(uint16_t interval)
{
    uint32_t inter = interval > 0 ? interval * 60 * 1000 : 5 * 1000;
    static uint16_t temp_car_num = 0;
    static uint32_t last_get = 0;
    if(Capture_Status & 0x80000000) //成功捕获到了一次低电平
    {
        
        temp = Capture_Status & 0x3fffffff;
        Capture_Status=0; //开启下一次捕获
        temp_car_num += temp / 600;
    }
    if((xTaskGetTickCount() - last_get) > inter)
    {
        last_get = xTaskGetTickCount();    
        //计算车流量
        Car_num = temp_car_num;
        //Debug_Msg_Module_Printf(TEMP,"car is %d \n",Car_num);
        temp_car_num = 0;
    }
}

uint16_t get_car_num(void)
{
    return Car_num;
}