#ifndef __STOPWATCH_APP_H
#define __STOPWATCH_APP_H

#include "Time_Task.h"

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t TimeTaskHandle;

extern TimerHandle_t StopWatchTimerHandle;//软件定时器句柄


uint8_t Show_StopWatch(void);
void StopWatch_Tick(TimerHandle_t xTimer);


#endif
