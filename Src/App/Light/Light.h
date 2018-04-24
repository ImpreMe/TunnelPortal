#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "stm32f1xx_hal.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>


extern UART_HandleTypeDef huart1;

#define MAX_BUF_LEN   256    //串口驱动层最大数据包256字节
#define UART_TIME_OUT  200   //串口超时时间
 


void Light_Init(void);
void Light_Poll(void);

uint32_t Get_Light(void);
#endif
