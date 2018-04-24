#include "bsp_can.h"
#include "assert.h"


static CAN_HandleTypeDef hcan1;

static CanTxMsgTypeDef  TxMessage;  
static CanRxMsgTypeDef  RxMessage; 

void BSP_CAN_Init(void)
{
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = 16;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SJW = CAN_SJW_1TQ;
    hcan1.Init.BS1 = CAN_BS1_3TQ;
    hcan1.Init.BS2 = CAN_BS2_2TQ;
    hcan1.Init.TTCM = DISABLE;
    hcan1.Init.ABOM = DISABLE;
    hcan1.Init.AWUM = ENABLE;
    hcan1.Init.NART = ENABLE;
    hcan1.Init.RFLM = DISABLE;
    hcan1.Init.TXFP = DISABLE;
    if (HAL_CAN_Init(&hcan1) != HAL_OK)
    {
        assert(0);
    }
    hcan1.pTxMsg =&TxMessage;
    hcan1.pRxMsg =&RxMessage;
    
    //设置接受过滤器，在此设置为32为掩码模式，针对标准ID
    CAN_FilterConfTypeDef  sFilterConfig;
    uint16_t StdIdArray[10] ={0x7e0,0x7e1,0x7e2,0x7e3,0x7e4,0x7e5,0x7e6,0x7e7,0x7e8,0x7e9}; //定义一组标准CAN ID
        
    sFilterConfig.FilterNumber = 2;				            //使用过滤器2
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;		//配置为掩码模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;	    //设置为32位宽
    sFilterConfig.FilterIdHigh =( StdIdArray[0] <<5 );      //验证码可以设置为StdIdArray[]数组中任意一个，这里使用StdIdArray[0]作为验证码
    sFilterConfig.FilterIdLow =0;

    uint16_t mask,num,tmp;
    mask = 0x7ff;                                           //下面开始计算屏蔽码
    num = sizeof(StdIdArray)/sizeof(StdIdArray[0]);
    for(int i =0 ; i < num ; i++)                           //屏蔽码位StdIdArray[]数组中所有成员的同或结果
    {
        tmp = StdIdArray[i] ^ (~StdIdArray[0]);             //所有数组成员与第0个成员进行同或操作
        mask &= tmp;
    }
    sFilterConfig.FilterMaskIdHigh =(mask<<5);
    sFilterConfig.FilterMaskIdLow =0|0x02;                  //只接收数据帧

    sFilterConfig.FilterFIFOAssignment = 0;                 //设置通过的数据帧进入到FIFO0中
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.BankNumber = 14;

    if(HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
    {
        assert(0);
    }
    //开始接受数据
    
}

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    if(hcan->Instance==CAN1)
    {
        /* Peripheral clock enable */
        __HAL_RCC_CAN1_CLK_ENABLE();

        /**CAN GPIO Configuration    
        PA11     ------> CAN_RX
        PA12     ------> CAN_TX 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* CAN1 interrupt Init */
        HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
    }
}