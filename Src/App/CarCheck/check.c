#include "stm32f1xx_hal.h"
#include "check.h"
#include "assert.h"

TIM_HandleTypeDef htim5;

static void MX_TIM5_Init(void);

void check_init(void)
{
    MX_TIM5_Init();
}

static void MX_TIM5_Init(void)
{

    TIM_MasterConfigTypeDef sMasterConfig;
    TIM_IC_InitTypeDef sConfigIC;

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 72000 - 1;  //时钟为72Mhz，分频72，计数器时钟为1ms
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 0xffff;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_IC_Init(&htim5) != HAL_OK)
    {
        assert(0);
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
    {
        assert(0);
    }

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 0;
    if (HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
    {
        assert(0);
    }
    HAL_TIM_IC_Start_IT(&htim5,TIM_CHANNEL_2);
    __HAL_TIM_ENABLE_IT(&htim5,TIM_IT_UPDATE); //使能更新中断
    
    HAL_NVIC_SetPriority(TIM5_IRQn,2,0); //设置中断优先级，抢占2，子优先级0
    HAL_NVIC_EnableIRQ(TIM5_IRQn); //开启ITM5中断通道
}

void HAL_TIM_IC_MspInit(TIM_HandleTypeDef* htim_ic)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(htim_ic->Instance==TIM5)
  {

    /* Peripheral clock enable */
    __HAL_RCC_TIM5_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM5 GPIO Configuration    
    PA1     ------> TIM5_CH2 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }

}


//捕获状态
//[7]:0,没有成功的捕获;1,成功捕获到一次.
//[6]:0,还没捕获到上升沿;1,已经捕获到下降沿了.
//[5:0]:捕获上升沿后溢出的次数(对于16位定时器来说,1ms计数器加1,溢出时间:65秒)
uint8_t  Capture_Status=0; //输入捕获状态
uint16_t Capture_Value;//输入捕获值(TIM2/TIM5是16位)


//定时器5中断服务函数
void TIM5_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim5);//定时器共用处理函数
}

/*
//定时器更新中断（溢出）中断处理回调函数， 在HAL_TIM_IRQHandler中会被调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)//更新中断发生时执行
{
    if(htim->Instance == TIM5)
    {
        if((Capture_Status & 0x80) ==0 )//还未成功捕获
        {
            if(Capture_Status & 0x40 )//已经捕获到高电平了
            {
                if((Capture_Status & 0x3F) == 0x3F)//高电平太长了
                {
                    Capture_Status |= 0x80; //标记成功捕获了一次
                    Capture_Value = 0xffff;
                }
                else 
                    Capture_Status++;
            }
        }        
    }
    else if(htim->Instance == TIM2)  //串口在使用定时器2
    {
        
    }

}*/


//车检器脉冲为一低电平脉冲，采集脉冲宽度来计算车流量
//-----------|    |-----------
//           |    |
//           |____|
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)//捕获中断发生时执行
{
    if(htim->Instance == TIM5)
    {
        if((Capture_Status & 0x80) != 0)//已经成功获取，还没处理
            return;
        if(Capture_Status & 0x40) //捕获到一个下降沿
        {
            Capture_Status |= 0x80; //标记成功捕获到一次高电平脉宽
            Capture_Value = HAL_TIM_ReadCapturedValue(&htim5,TIM_CHANNEL_2);//获取当前的捕获值.
            TIM_RESET_CAPTUREPOLARITY(&htim5,TIM_CHANNEL_2); //清除设置
            TIM_SET_CAPTUREPOLARITY(&htim5,TIM_CHANNEL_2,TIM_ICPOLARITY_FALLING);//下降沿捕获
        }
        else //还未开始,第一次捕获下降沿
        {
            Capture_Status = 0; //清空
            Capture_Value  = 0;
            Capture_Status |= 0x40; //标记捕获到了下降沿
            __HAL_TIM_DISABLE(&htim5); //关闭定时器5
            __HAL_TIM_SET_COUNTER(&htim5,0);
            TIM_RESET_CAPTUREPOLARITY(&htim5,TIM_CHANNEL_2); //清除原来设置
            TIM_SET_CAPTUREPOLARITY(&htim5,TIM_CHANNEL_2,TIM_ICPOLARITY_RISING);//上升沿沿捕获
            __HAL_TIM_ENABLE(&htim5);//使能定时器5
        }       
    }

}

