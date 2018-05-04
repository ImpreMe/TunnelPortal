#include "comm.h"

static UART_HandleTypeDef huart4;

static uint16_t rx_comm_index = 0;
static uint8_t rx_comm_buf[MAX_LEN] = {0};

static uint16_t tx_comm_index = 0;
static uint8_t tx_comm_buf[MAX_LEN] = {0};

static CommStatus_t Comm_Status = STATUS_IDLE;
static uint8_t try_times = 0;


extern SemaphoreHandle_t xConfig_mutex;


static Config_t g_tConfig = {0};


void Config_Init(void)
{
    EEPROM_Read(CONFIG_ADDR, &g_tConfig, sizeof(Config_t));
    uint16_t crc = CRC16_IBM_Calc(&g_tConfig , sizeof(Config_t) - 2);
    if(crc != g_tConfig.crc16)
    {
        EEPROM_Read(BACK_CONFIG_ADDR, &g_tConfig, sizeof(Config_t));
        uint16_t crc_back = CRC16_IBM_Calc(&g_tConfig , sizeof(Config_t) - 2);  
        if(crc_back != g_tConfig.crc16)
        {
            g_tConfig.control_mode = 0;
            g_tConfig.control_period = 1;
        }
        else
            EEPROM_Write(CONFIG_ADDR, &g_tConfig, sizeof(Config_t));
    }
}

uint8_t Get_Config(Config_t *config)
{
    if( xSemaphoreTake( xConfig_mutex, 10 ) == pdPASS )
    {
        memcpy(config,&g_tConfig,sizeof(Config_t));
        xSemaphoreGive( xConfig_mutex );
        return 0;
    }
    return 1;
}

uint8_t Set_Config(Config_t config)
{
    if( xSemaphoreTake( xConfig_mutex, 100 ) == pdPASS )
    {
        memcpy(&g_tConfig,&config,sizeof(Config_t));
        xSemaphoreGive( xConfig_mutex );
        g_tConfig.crc16 = CRC16_IBM_Calc(&g_tConfig , sizeof(Config_t) - 2);  
        if(EEPROM_Write(CONFIG_ADDR, &g_tConfig, sizeof(Config_t)))
            return 1;
        if(EEPROM_Write(BACK_CONFIG_ADDR, &g_tConfig, sizeof(Config_t)))
            return 1;
        return 0;
    }
    return 1;
}

/* UART4 init function */
void Comm_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_UART4_CLK_ENABLE();
    
    huart4.Instance = UART4;
    huart4.Init.BaudRate = 9600;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart4) != HAL_OK)
    {
        assert(0);
    }

    /**UART4 GPIO Configuration    
    PC10     ------> UART4_TX
    PC11     ------> UART4_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    /* UART4 interrupt Init */
    __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
    HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(UART4_IRQn);
}



void UART4_IRQHandler(void)
{
    uint32_t tmp_flag = 0, tmp_it_source = 0;
	tmp_flag = __HAL_UART_GET_FLAG(&huart4, UART_FLAG_RXNE);
	tmp_it_source = __HAL_UART_GET_IT_SOURCE(&huart4, UART_IT_RXNE);
    if((tmp_flag != RESET) && (tmp_it_source != RESET))
    {
        rx_comm_buf[rx_comm_index++] = (uint8_t)(huart4.Instance->DR & (uint8_t)0x00FF);
        if(rx_comm_index >= sizeof(rx_comm_buf))
        {
            rx_comm_index = 0;
        }
    }
}

static void Set_CommStatus(CommStatus_t stat)
{
    Comm_Status = stat;
}

static void End_Comm(void)
{
    try_times = 0;
    Comm_Status = STATUS_IDLE;
    rx_comm_index = 0;
    memset(rx_comm_buf , 0 , sizeof(rx_comm_buf));
    tx_comm_index = 0;
    memset(tx_comm_buf , 0 , sizeof(tx_comm_buf));
}

/**
  * 发送数据
  * buf 发送缓冲区
  * len 发送数据长度
  * mod 发送模式，只能取STATUS_TX,STATUS_TX_NOREPLY
  */
void Comm_Send(const void * buf , uint16_t len , CommStatus_t mod)
{
    if((mod != STATUS_TX) && (mod != STATUS_TX_NOREPLY))
        return ;
    if(len > MAX_LEN) return ;
    memcpy(tx_comm_buf , buf , len);
    tx_comm_index = len;
    Set_CommStatus(mod); 
}

static void Comm_Poll(void)
{
    static uint32_t send_time_tick = 0;
    switch(Comm_Status)
    {
        case STATUS_IDLE :  /*串口空闲此时应该一直监听串口数据(被动数据)*/
            if(rx_comm_index != 0)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                /* 开始处理接收的数据 增加协议处理*/
                End_Comm();
            }
            break;
        case STATUS_TX :
            /* 发送数据 */
            HAL_UART_Transmit(&huart4, tx_comm_buf, tx_comm_index, 500);
            send_time_tick = xTaskGetTickCount();
            Comm_Status = STATUS_RX;
            break;
        case STATUS_TX_NOREPLY :
            /* 发送数据 */
            HAL_UART_Transmit(&huart4, tx_comm_buf, tx_comm_index, 500);
            Comm_Status = STATUS_IDLE;
            break;            
        case STATUS_RX : /*接受数据（主动数据）*/
            if(rx_comm_index != 0)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                /*开始处理接收的数据 增加协议处理*/
                End_Comm();
            }
            else if((xTaskGetTickCount() - send_time_tick) > COMM_TIME_OUT)
            {
                Comm_Status = STATUS_TIMEOUT;
            }
            break;
        case STATUS_TIMEOUT :
            if(try_times >= MAX_TRY_TIMES)
            {
                /* 通信结束 */
                End_Comm();
                break;
            }
            else
            {
                Comm_Status = STATUS_TX;
                try_times++ ;
                break;
            }
        default :
            break;
    }
}

void vTaskCommCode( void * pvParameters )
{
    (void)pvParameters;
    static uint32_t last_report = 0;

        
    while(1)
    {
        if((xTaskGetTickCount() - last_report) > 1 * 60 * 1000)
        {
            /*上报一次当前数据*/
            //Comm_Send(const void * buf , uint16_t len , STATUS_TX_NOREPLY);
        }
        Comm_Poll();
        Light_Poll();
        check_poll();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}