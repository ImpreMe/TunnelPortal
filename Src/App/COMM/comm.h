#ifndef __COMM_H__
#define __COMM_H__

#include "includes.h"



#define MAX_LEN   256    //串口驱动层最大数据包256字节
#define COMM_TIME_OUT  500   //通信超时时间
#define MAX_TRY_TIMES  3

#define CONFIG_ADDR           0
#define BACK_CONFIG_ADDR      256


typedef enum
{
    STATUS_IDLE,
    STATUS_TX,
    STATUS_TX_NOREPLY,
    STATUS_RX,
    STATUS_TIMEOUT
}CommStatus_t;

__packed typedef struct
{
    uint8_t  control_mode;      //自动控制还是手动控制 0:自动  1:手动
    uint8_t  lamp_mode[3];     //三种颜色灯的运行模式
    uint8_t  lamp_value[3];     //三种颜色灯的亮度
    uint16_t control_period;    //自动控制的控制周期(以分钟为单位)
    uint16_t manual_duration;   //手动控制时的控制时间(以分钟为单位)
    uint32_t manual_tick;
    uint16_t crc16;
}Config_t;

void Comm_Init(void);
void Config_Init(void);
void vTaskCommCode( void * pvParameters );

void Comm_Send(const void * buf , uint16_t len , CommStatus_t mod);

uint8_t Set_Config(Config_t config);
uint8_t Get_Config(Config_t *config);
#endif
