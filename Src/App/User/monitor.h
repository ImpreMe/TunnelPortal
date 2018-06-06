#ifndef __MONITOR_H_
#define __MONITOR_H_

#include <stdint.h>

//#define USE_LORA_TASK     0
#define USE_COMM_TASK     1
#define USE_LIGHT_TASK    2

#define TASK_TIME_OUT     50000	

void TaskMonitor(uint8_t taskid);

#endif