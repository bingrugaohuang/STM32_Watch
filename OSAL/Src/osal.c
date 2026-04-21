#include "osal.h"

OSAL_MutexHandle OSAL_MutexCreateRecursive(void)
{
    OSAL_MutexHandle handle = xSemaphoreCreateRecursiveMutex();
    configASSERT(handle != NULL);
    return handle;
}

void OSAL_MutexLock(OSAL_MutexHandle mutex, TickType_t timeoutTicks)
{
    configASSERT(mutex != NULL);
    BaseType_t taken = xSemaphoreTakeRecursive(mutex, timeoutTicks);
    configASSERT(taken == pdPASS);
}

void OSAL_MutexUnlock(OSAL_MutexHandle mutex)
{
    configASSERT(mutex != NULL);
    BaseType_t given = xSemaphoreGiveRecursive(mutex);
    configASSERT(given == pdPASS);
}

BaseType_t OSAL_TaskResumeIfSuspended(TaskHandle_t task)
{
    configASSERT(task != NULL);
    if(eTaskGetState(task) == eSuspended)
    {
        vTaskResume(task);
        return pdTRUE;
    }
    return pdFALSE;
}

BaseType_t OSAL_TaskSuspendIfRunning(TaskHandle_t task)
{
    configASSERT(task != NULL);
    if(eTaskGetState(task) != eSuspended)
    {
        vTaskSuspend(task);
        return pdTRUE;
    }
    return pdFALSE;
}

TickType_t OSAL_GetTickCount(void)
{
    return xTaskGetTickCount();
}

TickType_t OSAL_GetTickCountFromISR(void)
{
    return xTaskGetTickCountFromISR();
}

void OSAL_Delay(uint32_t delayMs)
{
    configASSERT(delayMs > 0U);
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}

void OSAL_DelayUntil(TickType_t *const previousWakeTime, TickType_t timeIncrement)
{
    configASSERT(previousWakeTime != NULL);
    configASSERT(timeIncrement > 0U);
    vTaskDelayUntil(previousWakeTime, timeIncrement);
}
