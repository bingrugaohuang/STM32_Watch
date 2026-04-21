#ifndef _SLEEPMANAGER_H
#define _SLEEPMANAGER_H

#include "main.h"
#include "FreeRTOS.h"
#include "OLED.h"
#include "task.h"
#include "MPU6050.h"
#include "app_handles.h"
#include "AlarmService.h"
#include "TaskMgr.h"

typedef enum{
    SLEEP_STATE_ACTIVE = 0,       
    SLEEP_STATE_PREPARE_SLEEP,    
    SLEEP_STATE_SLEEPING,         
    SLEEP_STATE_PREPARE_STOP,     
    SLEEP_STATE_STOP,             
    SLEEP_STATE_WAKEUP           
} SleepState_t;

void SleepMgr_ReportActivity(void);
void SleepManager_Task(void *argument);

void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin);
uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void);

#endif
