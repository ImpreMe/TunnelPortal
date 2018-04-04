#include "dido.h"
#include <assert.h>


static ADC_HandleTypeDef hadc1;
static DMA_HandleTypeDef hdma_adc1;

static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);

uint16_t rx_adc[TOTAL_CHANNEL] = {0};

void DIDO_Init(void)
{
    MX_GPIO_Init();
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

    __HAL_LINKDMA(hadc,DMA_Handle,hdma_adc1);
  }

}

static void MX_GPIO_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    //__HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CH1_CTR_PORT, CH1_CTR_PIN, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CH2_CTR_PORT, CH2_CTR_PIN, GPIO_PIN_RESET);
    
    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CH3_CTR_PORT, CH3_CTR_PIN, GPIO_PIN_RESET);

    /*Configure GPIO pins : PA8 */
    GPIO_InitStruct.Pin = CH1_CTR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CH1_CTR_PORT, &GPIO_InitStruct);

    /*Configure GPIO pins : PA0 */
    GPIO_InitStruct.Pin = CH2_CTR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CH2_CTR_PORT, &GPIO_InitStruct);
    
    /*Configure GPIO pin : PC6 */
    GPIO_InitStruct.Pin = CH3_CTR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CH3_CTR_PORT, &GPIO_InitStruct);

}

//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
//{
//    if(hadc->Instance == ADC1)
//    {
//        
//    }
//}