#ifndef APP_TASKMGR_H
#define APP_TASKMGR_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "OLED.h"
#include "app_handles.h"
#include "MPU6050.h"

typedef enum{
    TASKMGR_TASK_NULL = 0,
    TASKMGR_TASK_UI,
    TASKMGR_TASK_MENU,
    TASKMGR_TASK_SET,
    TASKMGR_TASK_TIME,
    TASKMGR_TASK_FLASHLIGHT,
    TASKMGR_TASK_MPU6050,
    TASKMGR_TASK_GAME,
    TASKMGR_TASK_STACK_MONITOR,
    TASKMGR_TASK_SLEEP_MANAGER,
    TASKMGR_TASK_ALARM,
    TASKMGR_TASK_COUNT
}TaskMgrTaskId_t;

typedef struct{
    TaskMgrTaskId_t ResumeTaskId;
    TaskMgrTaskId_t SuspendTaskId;
}TaskMgrSwitchPlan_t;

BaseType_t TaskMgr_ApplySwitchPlan(const TaskMgrSwitchPlan_t* plan);
uint32_t SuspendForegroundTasks(void);
void ResumeForegroundTasks(uint32_t mark);

#endif // APP_TASKMGR_H
