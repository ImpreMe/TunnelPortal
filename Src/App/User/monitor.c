#include "includes.h"

/* 现有三个任务：
lora任务：主要实现lora数据的收发
comm任务：串口和上位机通信，485和光照度传感器通信，和微波车检器通信
light任务：根据条件，实现灯光的变换
*/



void TaskMonitor(uint8_t taskid)
{
    uint16_t task = 0, timeOutFlag = 0;
#ifdef USE_LORA_TASK
	static uint32_t uiLORATaskTimeOutCnt = 0;
#endif
#ifdef USE_COMM_TASK
	static uint32_t uiCOMMTaskTimeOutCnt = 0;
#endif
#ifdef USE_LIGHT_TASK
	static uint32_t uiLIGHTTaskTimeOutCnt = 0;
#endif
#ifdef USE_LORA_TASK	
	if( USE_LORA_TASK == taskid )
		uiLORATaskTimeOutCnt = 0;
	else
	{
		if( ++uiLORATaskTimeOutCnt > TASK_TIME_OUT )
		{
			task = USE_LORA_TASK;
			timeOutFlag = 1;
		}
	}
#endif	
    
#ifdef USE_COMM_TASK	
	if( USE_COMM_TASK == taskid )
		uiCOMMTaskTimeOutCnt = 0;
	else
	{
		if( ++uiCOMMTaskTimeOutCnt > TASK_TIME_OUT )
		{
			task = USE_COMM_TASK;
			timeOutFlag = 1;
		}
	}
#endif	
    
#ifdef USE_LIGHT_TASK	
	if( USE_LIGHT_TASK == taskid )
		uiLIGHTTaskTimeOutCnt = 0;
	else
	{
		if( ++uiLIGHTTaskTimeOutCnt > TASK_TIME_OUT )
		{
			task = USE_LIGHT_TASK;
			timeOutFlag = 1;
		}
	}
#endif	
    
    if(timeOutFlag)
    {
        Debug_Printf("task %d is timeout,system reset!",task);
        __disable_irq();
        HAL_NVIC_SystemReset();
    }
}