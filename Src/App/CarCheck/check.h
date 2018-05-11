#ifndef __CHECK_H
#define __CHECK_H
 
//PA1/TIM5_CH2

#define CHECK_PORT       GPIOA
#define CHECK_PIN        GPIO_PIN_1

#define CONTROL_INTERVAL   1   //每五分钟控制一次


extern volatile uint32_t  Capture_Status; //输入捕获状态

void check_init(void);
void check_poll(uint16_t interval);
uint16_t get_car_num(void);
#endif