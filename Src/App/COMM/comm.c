#include "comm.h"

static UART_HandleTypeDef huart4;

static uint16_t rx_comm_index = 0;
static uint8_t rx_comm_buf[MAX_LEN] = {0};

static uint16_t tx_comm_index = 0;
static uint8_t tx_comm_buf[MAX_LEN] = {0};

static CommStatus_t Comm_Status = STATUS_IDLE;
static uint8_t try_times = 0;

/*该部分均在main.c中定义*/
extern SemaphoreHandle_t xConfig_mutex;
extern SemaphoreHandle_t xReset_seam;
extern void Get_mode_from_num(uint16_t num , Config_t config,uint8_t array[]);
extern void Get_value_from_light(uint32_t light_value ,Config_t config ,uint8_t lamp_value[]);

static Config_t g_tConfig = {0};

typedef struct {
	uint16_t head;					// 帧头0xAA, 0x55
	uint16_t length;				// 数据长度: 数据帧中所有数据长度
	uint16_t cmd;					// 命令
	uint16_t srcID;				// 源ID
	uint16_t destID;				// 目的ID
	uint16_t number;				// 帧序号
	uint8_t data[308];  			// 数据: 最长不得超过308字节(包括校验位)
}*pDataFrame_t, DataFrame_t;

enum {
	// 0x0030--0x004F
	CMD_GET_ID = 0x0030,				// 0x0030 查询ID
	CMD_SET_ID,				       // 0x0031 设置ID
	CMD_GET_LIGHT,					// 0x0032 查询灯光
	CMD_SET_LIGHT,					// 0x0033 设置灯光
	CMD_GET_CONTFREQ,					// 0x0034 查询控制频率
	CMD_SET_CONTFREQ,				   // 0x0035 设置控制频率
	CMD_GET_STATUS,				   // 0x0036 查询设备运行状态
	CMD_UP_DATA,				      // 0x0037 上报数据（不用应答）
	CMD_RESET				         // 0x0038 复位
};


//// 应答命令，为通信命令或0x8000
//enum {
//	ACK_GET_ID = 0x8030,				// 0x8030 查询ID应答
//	ACK_SET_ID,						// 0x8031 设置ID应答
//	ACK_GET_LIGHT,				   // 0x8032 查询灯光应答
//	ACK_SET_LIGHT,				   // 0x8033 设置灯光应答
//	ACK_GET_CONTFREQ,			      //  0x8034 查询控制频率应答
//	ACK_SET_CONTFREQ,			      //  0x8035 设置控制频率应答
//	ACK_GET_STATUS,               //  0x8036 查询设备运行状态应答
//	ACK_RESET                    //  0x8038 复位应答
//};  


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
            memset(&g_tConfig,0,sizeof(Config_t));
            g_tConfig.control_mode = 0;    //自动控制
            g_tConfig.control_period = 1;  //一分钟控制一次
            g_tConfig.threshold1 = 5;      //闪烁频率的阈值1
            g_tConfig.threshold2 = 15;     //闪烁频率的阈值2
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

static void get_id(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 12 + 2;  //固定12字节 + 12字节的芯片ID + 2字节的deviceid
    uint32_t st_id[3] = {0};
    st_id[0] = *(uint32_t*)0x1FFFF7E8 ; 
    st_id[1] = *(uint32_t*)0x1FFFF7E8 + 4; 
    st_id[2] = *(uint32_t*)0x1FFFF7E8 + 8; 
    for(int i = 0 ; i < 3 ;i ++)
    {
        data.data[i*4] = (uint8_t)(st_id[i] >> 24);
        data.data[i*4 + 1] = (uint8_t)(st_id[i] >> 16);
        data.data[i*4 + 2] = (uint8_t)(st_id[i] >> 8);
        data.data[i*4 + 3] = (uint8_t)(st_id[i] >> 0);
    }
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        data.data[12] = (uint8_t)temp_config.deviceid;
        data.data[13] = (uint8_t)(temp_config.deviceid >> 8);
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);        
}


static void set_id(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 1;  //固定12字节 + 成功或者失败的标志
    data.data[0] = 1;
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        temp_config.deviceid = (ppkg->data[0] << 8) | ppkg->data[1];
        data.data[0] = Set_Config(temp_config);
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);        
}

static void get_light(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 6;  //固定12字节 + 三组灯光的状态值
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        uint8_t temp_mode[3];
        uint8_t temp_lamp[3];
        
        uint32_t temp_light = Get_Light();
        uint16_t car_num = get_car_num(); 
        if(temp_config.control_mode == 0)
        {
            Get_mode_from_num(car_num , temp_config,temp_mode);
            Get_value_from_light(temp_light , temp_config,temp_lamp);
        }
        for(int i = 0 ; i < 3 ; i++)
        {
            data.data[i*2] = temp_config.control_mode > 0 ? temp_config.lamp_mode[i] : temp_mode[i];
            data.data[i*2 + 1] = temp_config.control_mode > 0 ? temp_config.lamp_value[i] : temp_lamp[i];
        }
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);
}

static void set_light(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 1;  //固定12字节 + 成功或者失败的标志
    data.data[0] = 1;
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        temp_config.control_mode = 1; //切换为手动控制
        for(int i = 0 ; i < 3 ; i++)
        {
            temp_config.lamp_mode[i] = ppkg->data[i * 2];
            temp_config.lamp_value[i] = ppkg->data[i * 2 + 1];
        }
        temp_config.manual_duration = (ppkg->data[3*2]) | (ppkg->data[3*2 + 1] << 8);
        temp_config.manual_tick = xTaskGetTickCount();
        data.data[0] = Set_Config(temp_config);
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);      
}

static void get_contfreq(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 4;  //固定12字节 + 成功或者失败的标志
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        data.data[0] = (uint8_t)(temp_config.control_period >> 0);
        data.data[1] = (uint8_t)(temp_config.control_period >> 8);
        data.data[2] = temp_config.threshold1;
        data.data[3] = temp_config.threshold2;
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);      
}

static void set_contfreq(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 1;  //固定12字节 + 成功或者失败的标志
    data.data[0] = 1;
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        temp_config.control_period = (ppkg->data[0]) | (ppkg->data[1] << 8);
        temp_config.threshold1 = ppkg->data[2];
        temp_config.threshold2 = ppkg->data[3];
        data.data[0] = Set_Config(temp_config);
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);      
}

static void get_status(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 29;  //固定12字节 + 三组灯光的状态值
    Config_t temp_config ;
    if(!Get_Config(&temp_config))
    {
        uint8_t temp_mode[3];
        uint8_t temp_lamp[3];
        
        uint32_t temp_light = Get_Light();
        uint16_t car_num = get_car_num(); 
        
        data.data[0] = (uint8_t)(car_num >> 0);
        data.data[1] = (uint8_t)(car_num >> 8);
        data.data[2] = (uint8_t)(temp_light >> 0);
        data.data[3] = (uint8_t)(temp_light >> 8);
        data.data[4] = (uint8_t)(temp_light >> 16);
        data.data[5] = (uint8_t)(temp_light >> 24);
        data.data[6] = temp_config.control_mode;
        
        if(temp_config.control_mode == 0)
        {
            Get_mode_from_num(car_num ,temp_config, temp_mode);
            Get_value_from_light(temp_light , temp_config,temp_lamp);
        }        
        for(int i = 0 ; i < 3 ; i++)
        {
            data.data[7 + i*2] = temp_config.control_mode > 0 ? temp_config.lamp_mode[i] : temp_mode[i];
            data.data[7 + i*2 + 1] = temp_config.control_mode > 0 ? temp_config.lamp_value[i] : temp_lamp[i];
        }
        data.data[13] = (uint8_t)(temp_config.control_period >> 0);
        data.data[14] = (uint8_t)(temp_config.control_period >> 8);
        
        data.data[15] = temp_config.threshold1;
        data.data[16] = temp_config.threshold2;
        
        float current_value[3] = {0};
        Get_current(current_value);
        for(int i = 0 ; i < sizeof(current_value) / sizeof(float) ; i++)
        {
            data.data[17 + i*4]     = ((int)(current_value[i] * 1000));
            data.data[17 + i*4 + 1] = ((int)(current_value[i] * 1000) >> 8);
            data.data[17 + i*4 + 2] = ((int)(current_value[i] * 1000) >> 16);
            data.data[17 + i*4 + 3] = ((int)(current_value[i] * 1000) >> 24);
        }
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);
}

static void reset(pDataFrame_t ppkg)
{
    DataFrame_t data;
    data.cmd = ppkg->cmd | 0x8000;
    data.destID = ppkg->srcID;
    data.srcID = ppkg->destID;
    data.head = ppkg->head;
    data.number = ppkg->number;
    data.length = 12 + 1;  //固定12字节 + 成功或者失败的标志
    data.data[0] = 0;
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY); 
    xSemaphoreGive( xReset_seam );
}

static void Auto_report(void)
{
    static uint16_t pkg_num = 0;
    Config_t temp_config ;
    if(Get_Config(&temp_config))
        return;
    DataFrame_t data;
    data.head = 0x55AA;
    data.cmd = CMD_UP_DATA;
    data.destID = 0xFFFF;
    data.srcID = temp_config.deviceid;
    data.number = pkg_num++;
    data.length = 12 + 29;  //固定12字节 + 三组灯光的状态值
    

    uint8_t temp_mode[3];
    uint8_t temp_lamp[3];
    
    uint32_t temp_light = Get_Light();
    uint16_t car_num = get_car_num(); 
    
    data.data[0] = (uint8_t)(car_num >> 0);
    data.data[1] = (uint8_t)(car_num >> 8);
    data.data[2] = (uint8_t)(temp_light >> 0);
    data.data[3] = (uint8_t)(temp_light >> 8);
    data.data[4] = (uint8_t)(temp_light >> 16);
    data.data[5] = (uint8_t)(temp_light >> 24);
    data.data[6] = temp_config.control_mode;
    
    if(temp_config.control_mode == 0)
    {
        Get_mode_from_num(car_num , temp_config,temp_mode);
        Get_value_from_light(temp_light , temp_config,temp_lamp);
    }        
    for(int i = 0 ; i < 3 ; i++)
    {
        data.data[7 + i*2] = temp_config.control_mode > 0 ? temp_config.lamp_mode[i] : temp_mode[i];
        data.data[7 + i*2 + 1] = temp_config.control_mode > 0 ? temp_config.lamp_value[i] : temp_lamp[i];
    }
    data.data[13] = (uint8_t)(temp_config.control_period >> 0);
    data.data[14] = (uint8_t)(temp_config.control_period >> 8);
    data.data[15] = temp_config.threshold1;
    data.data[16] = temp_config.threshold2;
    
    float current_value[3] = {0};
    Get_current(current_value);
    for(int i = 0 ; i < sizeof(current_value) / sizeof(float) ; i++)
    {
        data.data[17 + i*4]     = ((int)(current_value[i] * 1000));
        data.data[17 + i*4 + 1] = ((int)(current_value[i] * 1000) >> 8);
        data.data[17 + i*4 + 2] = ((int)(current_value[i] * 1000) >> 16);
        data.data[17 + i*4 + 3] = ((int)(current_value[i] * 1000) >> 24);
    }
    Comm_Send(&data , data.length , STATUS_TX_NOREPLY);    
}
static void Analysis_data(uint8_t* buf , uint16_t len)
{
    while( buf[0] != 0xAA || buf[1] != 0x55)
    {
        memcpy(buf , buf+1,len-1);
        len--;
        if(len < 10)
            return;
    }
    pDataFrame_t pkg = (pDataFrame_t)buf;
    if((pkg->destID != g_tConfig.deviceid ) && (pkg->destID != 0xFFFF))
        return ;
    switch(pkg->cmd)
    {
        case CMD_GET_ID :
            get_id(pkg);
        break;
        case CMD_SET_ID :
            set_id(pkg);
        break;
        case CMD_GET_LIGHT :
            get_light(pkg);
        break;
        case CMD_SET_LIGHT :
            set_light(pkg);
        break;
        case CMD_GET_CONTFREQ :
            get_contfreq(pkg);
        break;
        case CMD_SET_CONTFREQ :
            set_contfreq(pkg);
        break;
        case CMD_GET_STATUS :
            get_status(pkg);
        break;
        case CMD_RESET :
            reset(pkg);
        break;
        default : break;
    }
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

static volatile uint8_t send_flag = 0;
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
                uint8_t temp_buf[MAX_LEN] = {0};
                uint16_t len = 0;
                memcpy(temp_buf,rx_comm_buf,rx_comm_index);
                len = rx_comm_index;
                End_Comm();
                Analysis_data(temp_buf,len);
            }
            break;
        case STATUS_TX :
            /* 发送数据 */
            HAL_UART_Transmit(&huart4, tx_comm_buf, tx_comm_index, 500);
            send_time_tick = xTaskGetTickCount();
            Comm_Status = STATUS_RX;
            send_flag = 1;
            break;
        case STATUS_TX_NOREPLY :
            /* 发送数据 */
            HAL_UART_Transmit(&huart4, tx_comm_buf, tx_comm_index, 500);
            Comm_Status = STATUS_IDLE;
            send_flag = 1;
            if(xSemaphoreTake( xReset_seam, 0 ) == pdPASS)
            {
                __disable_irq();   
                HAL_NVIC_SystemReset();
            }
            break;            
        case STATUS_RX : /*接受数据（主动数据）*/
            if(rx_comm_index != 0)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
                send_flag = 1;
                /*开始处理接收的数据 增加协议处理*/
                Analysis_data(rx_comm_buf,rx_comm_index);
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

    uint8_t send_time = 0;
    while(1)
    {
        if(send_flag == 1)
        {
            BSP_LED_Toggle (LIGHT_RED);
            send_time++;
            if(send_time >= 10)
            {
                send_flag = 0;
                BSP_LED_Off (LIGHT_RED);
            }
        }
        if((xTaskGetTickCount() - last_report) > 1 * 60 * 1000)
        {
            /*上报一次当前数据*/
            Auto_report();
            last_report = xTaskGetTickCount();
            //Comm_Send(const void * buf , uint16_t len , STATUS_TX_NOREPLY);
        }
        Comm_Poll();
        Light_Poll();
        check_poll(g_tConfig.control_period);
        ADC_Poll();
        TaskMonitor(USE_COMM_TASK);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}