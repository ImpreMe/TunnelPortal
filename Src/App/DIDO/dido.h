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

void DIDO_Init(void);

#endif