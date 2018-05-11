#include "dido.h"
#include <assert.h>


static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;

static void MX_PWM_Init(void);
static void MX_ADC1_Init(void);

uint16_t rx_adc[TOTAL_CHANNEL] = {0};


static TIM_HandleTypeDef htim1;
static TIM_HandleTypeDef htim5;
static TIM_HandleTypeDef htim8;

void DIDO_Init(void)
{
    //MX_PWM_Init();
    MX_ADC1_Init();
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

    ADC_ChannelConfTypeDef sConfig;

    /**Common config 
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = TOTAL_CHANNEL;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        assert(0);
    }

    /**Configure Regular Channel 
    */
    sConfig.Channel = ADC_CHANNEL_10;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        assert(0);
    }

    /**Configure Regular Channel 
    */
    sConfig.Channel = ADC_CHANNEL_11;
    sConfig.Rank = ADC_REGULAR_RANK_2;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        assert(0);
    }

    /**Configure Regular Channel 
    */
    sConfig.Channel = ADC_CHANNEL_12;
    sConfig.Rank = ADC_REGULAR_RANK_3;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        assert(0);
    }
    HAL_ADC_Start_DMA(&hadc1,(uint32_t*)rx_adc,TOTAL_CHANNEL); 
}




void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hadc->Instance==ADC1)
  {
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    /**ADC1 GPIO Configuration    
    PC0     ------> ADC1_IN10
    PC1     ------> ADC1_IN11
    PC2     ------> ADC1_IN12 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
        assert(0);
    }
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    __HAL_LINKDMA(hadc,DMA_Handle,hdma_adc1);
  }

}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_adc1);
}

static float adc[TOTAL_CHANNEL];
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static uint16_t Average_array[AVERAGE_NUM][TOTAL_CHANNEL] = {0};
    static uint16_t average_time = 0;
    if(hadc->Instance == ADC1)
    {
        Average_array[average_time][0] = rx_adc[0];
        Average_array[average_time][1] = rx_adc[1];
        Average_array[average_time][2] = rx_adc[2];
        average_time++;
        if(average_time >= AVERAGE_NUM)
        {
            float sum = 0;
            for(int i = 0 ; i < TOTAL_CHANNEL ;i++)
            {
                for(int j = 0 ; j < AVERAGE_NUM ; j++)
                {
                    sum += Average_array[j][i];
                }
                adc[i] = sum / AVERAGE_NUM;
                sum = 0;
            }
            average_time = 0;
        }
        
    }
    
}


static float current[3] = {0};
void ADC_Poll(void)
{
    for(int i = 0 ; i < 3 ; i++)
    {
        current[i] = (2.5 -  (adc[i]/4096 * 3.3)) * 6.67;
    }
}

void Get_current(float adc_value[])
{
    for(int i = 0 ; i < sizeof(current) / sizeof(float) ; i++)
    {
        adc_value[i] = current[i];
    }
}
/**
    TIM1 : APB2总线，72MHz  >> CH1_CTR
    TIM5 : APB1总线，72MHz  >> CH2_CTR
    TIM8 : APB2总线，72MHz  >> CH3_CTR
*/
static void MX_PWM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC;  
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 72 - 1; //1M Hz 重装载值1000，所以PWM波周期为1KHz
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 1000 - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        assert(0);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        assert(0);
    }

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 72 - 1; //1M Hz 重装载值1000，所以PWM波周期为1KHz
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 1000 - 1;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.RepetitionCounter = 0;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
    {
        assert(0);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        assert(0);
    }
    
    htim8.Instance = TIM8;
    htim8.Init.Prescaler = 72 - 1; //1M Hz 重装载值1000，所以PWM波周期为1KHz
    htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim8.Init.Period = 1000 - 1;
    htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim8.Init.RepetitionCounter = 0;
    htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
    {
        assert(0);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        assert(0);
    }
    
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);//开启PWM通道1
    HAL_TIM_PWM_Start(&htim5,TIM_CHANNEL_1);//开启PWM通道1
    HAL_TIM_PWM_Start(&htim8,TIM_CHANNEL_1);//开启PWM通道1
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if(htim_pwm->Instance==TIM1)
    {
        /* Peripheral clock enable */
        __HAL_RCC_TIM1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM1 GPIO Configuration    
        PA8     ------> TIM1_CH1 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if(htim_pwm->Instance==TIM5)
    {
        /* Peripheral clock enable */
        __HAL_RCC_TIM5_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM5 GPIO Configuration    
        PA0-WKUP     ------> TIM5_CH1 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
    else if(htim_pwm->Instance==TIM8)
    {

        /* Peripheral clock enable */
        __HAL_RCC_TIM8_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**TIM8 GPIO Configuration    
        PC6     ------> TIM8_CH1 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}


void Set_Lighteness(LightType_t lighttype , uint16_t lighteness)
{
    if(lighttype == WHITE)
        TIM1->CCR1 = lighteness;

    if(lighttype == YELLOW)
        TIM5->CCR1 = lighteness;
    
    if(lighttype == RED)
        TIM8->CCR1 = lighteness;
}

uint16_t Get_Lighteness(LightType_t lighttype)
{
    uint16_t lighteness = 0;
    if(lighttype == WHITE)
        lighteness = TIM1->CCR1;

    if(lighttype == YELLOW)
        lighteness = TIM5->CCR1;
    
    if(lighttype == RED)
        lighteness = TIM8->CCR1;
    
    return lighteness;
}