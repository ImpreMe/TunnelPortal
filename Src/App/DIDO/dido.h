#ifndef __DIDO_H
#define __DIDO_H

#include "stm32f1xx_hal.h"

#define CH1_CTR_PORT     GPIOA
#define CH1_CTR_PIN      GPIO_PIN_8

#define CH2_CTR_PORT     GPIOA
#define CH2_CTR_PIN      GPIO_PIN_0

#define CH3_CTR_PORT     GPIOC
#define CH3_CTR_PIN      GPIO_PIN_6

#define TOTAL_CHANNEL    3
#define AVERAGE_NUM      100


typedef enum
{
    WHITE,
    YELLOW,
    RED
}LightType_t;

extern float adc[TOTAL_CHANNEL];

void DIDO_Init(void);
void Set_Lighteness(LightType_t lighttype , uint16_t lighteness);
uint16_t Get_Lighteness(LightType_t lighttype);

void ADC_Poll(void);
void Get_current(float adc_value[]);
#endif