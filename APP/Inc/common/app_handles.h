#ifndef _APP_HANDLES_H_
#define _APP_HANDLES_H_

#include "common/app_common.h"

extern TaskHandle_t KeyTaskHandle;
extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t UITaskHandle;
extern TaskHandle_t SetTaskHandle;
extern TaskHandle_t TimeTaskHandle;
extern TaskHandle_t FlashlightTaskHandle;
extern TaskHandle_t MPU6050TaskHandle;
extern TaskHandle_t GameTaskHandle;
extern TaskHandle_t StackMonitorTaskHandle;
extern TaskHandle_t SleepManagerTaskHandle;
extern TaskHandle_t AlarmTaskHandle;

extern TimerHandle_t StopWatchTimerHandle;
extern TimerHandle_t KeyTimerHandle;
extern TimerHandle_t DinoTimerHandle;

extern QueueHandle_t xKeyQueue;

#endif
