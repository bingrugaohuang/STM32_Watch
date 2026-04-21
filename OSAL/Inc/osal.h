#ifndef OSAL_H
#define OSAL_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef SemaphoreHandle_t OSAL_MutexHandle;

OSAL_MutexHandle OSAL_MutexCreateRecursive(void);
void OSAL_MutexLock(OSAL_MutexHandle mutex, TickType_t timeoutTicks);
void OSAL_MutexUnlock(OSAL_MutexHandle mutex);

BaseType_t OSAL_TaskResumeIfSuspended(TaskHandle_t task);
BaseType_t OSAL_TaskSuspendIfRunning(TaskHandle_t task);
TickType_t OSAL_GetTickCount(void);
TickType_t OSAL_GetTickCountFromISR(void);
void OSAL_Delay(uint32_t delayMs);
void OSAL_DelayUntil(TickType_t *const previousWakeTime, TickType_t timeIncrement);

#endif
