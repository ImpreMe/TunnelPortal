#ifndef __COMM_H__
#define __COMM_H__

#include "includes.h"



#define MAX_LEN   256    //串口驱动层最大数据包256字节
#define Comm_TIME_OUT  500   //通信超时时间
#define MAX_TRY_TIMES  3

typedef enum
{
    STATUS_IDLE,
    STATUS_TX,
    STATUS_TX_NOREPLY,
    STATUS_RX,
    STATUS_TIMEOUT
}CommStatus_t;

void Comm_Init(void);

void vTaskCommCode( void * pvParameters );

void Comm_Send(const void * buf , uint16_t len , CommStatus_t mod);

#endif
