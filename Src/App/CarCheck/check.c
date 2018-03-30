#include "stm32f1xx_hal.h"
#include "check.h"
#include "assert.h"
TIM_HandleTypeDef htim5;



void check_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : PB4 */
    GPIO_InitStruct.Pin = CHECK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CHECK_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);  
    
    __HAL_RCC_TIM5_CLK_ENABLE();
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 84000 - 1; //定时器5再APB1总线上，时钟频率84MHz,分频都时钟为1K
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 10000 - 1;  //溢出时间 10S
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        assert(0);
    }
    
    __HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE);
    
    HAL_NVIC_SetPriority(TIM5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM5_IRQn);
    //HAL_TIM_Base_Start_IT(&htim5);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(CHECK_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == CHECK_PIN)  //触发了中断，判断是上升沿还是下降沿
    {
        GPIO_PinState  pinstatus = HAL_GPIO_ReadPin(CHECK_PORT, CHECK_PIN);
        if(pinstatus == GPIO_PIN_RESET)    //下降沿中断
        {
            //在下降沿中断中，改为上升沿中断，并开启定时器计时
            HAL_TIM_Base_Start_IT(&htim5);            
        }
        else if(pinstatus == GPIO_PIN_SET)
        {
            //上升沿，关定时器,设置为下降沿中断,
        }

        
    }
}