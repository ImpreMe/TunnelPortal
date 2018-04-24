#include "includes.h"

UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim2;

/*驱动层缓冲区及缓存索引*/
static uint8_t rx_buf[MAX_BUF_LEN] = {0};
static uint16_t rxbuf_index = 0;
	
void Light_Init(void)
{
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 4800;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		assert(0);
	}
}

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	if(huart->Instance==USART1)
	{
		/* USER CODE BEGIN USART1_MspInit 0 */

		/* USER CODE END USART1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_USART1_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
		/**USART1 GPIO Configuration    
		PA9     ------> USART1_TX
		PA10     ------> USART1_RX 
		*/
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
        
        __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
		HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USART1_IRQn);
			
		
		__HAL_RCC_TIM2_CLK_ENABLE();
		htim2.Instance = TIM2;
		htim2.Init.Prescaler = 7200 - 1; //定时器2再APB1总线上，时钟频率72MHz,分频都时钟为10K
		htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
		htim2.Init.Period = UART_TIME_OUT * 10 - 1;  //超时时间 200ms
		htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
		{
			assert(0);
		}
		
		__HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
		
		HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(TIM2_IRQn);
		
	}
    
}

void USART1_IRQHandler(void)
{
    uint32_t tmp_flag = 0, tmp_it_source = 0;
	tmp_flag = __HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE);
	tmp_it_source = __HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE);
    if((tmp_flag != RESET) && (tmp_it_source != RESET))
    {
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        if(rxbuf_index == 0) //第一次接受数据，则打开定时器
            HAL_TIM_Base_Start_IT(&htim2);
        rx_buf[rxbuf_index++] = (uint8_t)(huart1.Instance->DR & (uint8_t)0x00FF);
		if(rxbuf_index >= MAX_BUF_LEN) //缓冲区溢出,把之前收到的数据全部抛弃，重新获取
        {
            rxbuf_index = 0;
            HAL_TIM_Base_Stop_IT(&htim2);
        }
			
    }
}

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&htim2);
}

/*应用层缓冲区及缓存索引*/
uint8_t rx[MAX_BUF_LEN] = {0};
uint16_t rx_index = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2)
	{
		//处理数据 01 03 04 00 00 01 44 FB 90
		if(rxbuf_index != 0) //驱动层收到了数据，将数据拷贝至应用层处理
		{
			memcpy(rx,rx_buf,rxbuf_index);
            rx_index = rxbuf_index;
            //清除驱动层数据
            memset(rx_buf,0,sizeof(rx_buf));
			rxbuf_index = 0;
            
			HAL_TIM_Base_Stop_IT(&htim2);
		}
	}
    else if(htim->Instance == TIM6) //此处是车检器的定时器处理函数
    {
        if((Capture_Status & 0x80000000) == 0 )//还未成功捕获
        {
            if(Capture_Status & 0x40000000 )//已经捕获到下降沿了
            {
                if((Capture_Status & 0x3fffffff) == 0x3fffffff)//高电平太长了
                {
                    Capture_Status |= 0x80000000; //标记成功捕获了一次
                    //Capture_Value = 0xffffffff;
                }
                else 
                    Capture_Status++;
            }
        }        
    }
}

static uint16_t Modbus_CRC ( uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    for ( j = 0; j < len ; j++)
    {
        crc = crc ^*buf++;
        for ( i=0; i<8; i++)
        {
            if(crc & 0x0001)
            {
                crc = crc>>1;
                crc = crc^ 0xa001;
            }
            else
                crc = crc>>1;
        }
    }
    return (crc);
}


static uint32_t light_value;
void Light_Poll(void)
{
    static uint32_t last_get = 0;
    if((xTaskGetTickCount() - last_get) > 1 * 1000)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
        uint8_t buf[] = {0x01, 0x03, 0x00, 0x02, 0x00, 0x02, 0x65, 0xCB};
        HAL_UART_Transmit(&huart1, buf, sizeof(buf), 500);
        last_get = xTaskGetTickCount();
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
    }
    uint32_t value = 0;
    static uint8_t is_first = 0;
    /*从缓冲区取出当前的照度值*/
    if(rx_index > 2)
    {
        uint16_t crc = Modbus_CRC(rx , rx_index - 2);
        if(crc != (rx[rx_index-1] << 8 | rx[rx_index-2]))
        {
            rx_index = 0;
            return ;
        }
        value = (rx[3] << 24) |  (rx[4] << 16) |  (rx[5] << 8) | rx[6];
        if (!is_first)
        {
            is_first = 1;
            light_value = value;
        }   
    }

        
    static bool have_outlier = false;
    static uint32_t last_tick_outlier = 0;
    
    if((value > 3 * light_value) || (value < light_value / 3)) //照度值出现异常
    {
        if(!have_outlier) //之前没有发生异常
        {
            have_outlier = true;
            last_tick_outlier = xTaskGetTickCount();
        }
        else //之前已经发生过异常，切没有恢复
        {
            if((xTaskGetTickCount() - last_tick_outlier) > 5 * 1000)//超时没有处理
            {
                light_value = value;
                have_outlier = false;                
            }
        }
    }
    else
    {
        light_value = value;
        have_outlier = false;
    }
}

uint32_t Get_Light(void)
{
    return light_value;
}