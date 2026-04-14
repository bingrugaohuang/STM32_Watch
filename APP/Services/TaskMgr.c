#include "TaskMgr.h"

// 任务映射表: 用任务ID统一管理句柄和前台挂起掩码
typedef struct {
    TaskMgrTaskId_t TaskId;
    TaskHandle_t* HandleRef;
    uint32_t ForefroundMark;
}TaskMgrMap_t;

static const TaskMgrMap_t gTaskMap[] = {
    {TASKMGR_TASK_UI,            &UITaskHandle,            (1U << 0U)},
    {TASKMGR_TASK_MENU,          &MenuTaskHandle,          (1U << 1U)},
    {TASKMGR_TASK_SET,           &SetTaskHandle,           (1U << 2U)},
    {TASKMGR_TASK_TIME,          &TimeTaskHandle,          (1U << 3U)},
    {TASKMGR_TASK_FLASHLIGHT,    &FlashlightTaskHandle,    (1U << 4U)},
    {TASKMGR_TASK_MPU6050,       &MPU6050TaskHandle,       (1U << 5U)},
    {TASKMGR_TASK_GAME,          &GameTaskHandle,          (1U << 6U)},
    {TASKMGR_TASK_STACK_MONITOR, &StackMonitorTaskHandle,  (1U << 7U)},
    {TASKMGR_TASK_SLEEP_MANAGER, &SleepManagerTaskHandle,          0U},
    {TASKMGR_TASK_ALARM,         &AlarmTaskHandle,                 0U}
};

/*
 * 函数功能：根据任务ID获取任务句柄
 * 入口参数：taskId 任务ID
 * 返回值  ：任务句柄
 */
static TaskHandle_t TaskMgr_GetHandle(TaskMgrTaskId_t taskId)
{
    for(uint8_t i = 0U; i < sizeof(gTaskMap) / sizeof(gTaskMap[0]); i++)
    {
        if(gTaskMap[i].TaskId == taskId)
        {
            return *(gTaskMap[i].HandleRef);
        }
    }
    return NULL;
}

/*
 * 函数功能：如果任务句柄有效且未挂起，则恢复任务
 * 入口参数：xTaskHandle 任务句柄
 * 返回值  : pdTRUE 已恢复，pdFALSE 无需恢复或句柄无效
 */
static BaseType_t ResumeIfNeeded(TaskHandle_t taskHandle)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) == eSuspended)
    {
        vTaskResume(taskHandle);
        return pdTRUE;
    }
    return pdFALSE;
}

/*
 * 函数功能：如果任务句柄有效且未挂起，则挂起任务
 * 入口参数：xTaskHandle 任务句柄
 * 返回值  : pdTRUE 已挂起，pdFALSE 无需挂起或句柄无效
 */
static BaseType_t SuspendIfNeeded(TaskHandle_t taskHandle)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) != eSuspended)
    {
        vTaskSuspend(taskHandle);
        return pdTRUE;
    }
    return pdFALSE;
}

/*
 * 函数功能：按切换策略执行任务切换
 * 入口参数：plan 切换策略（恢复哪个任务，挂起哪个任务）
 * 返回值  ：pdTRUE 至少命中一个有效句柄，pdFALSE 参数无效或均未命中
 */
BaseType_t TaskMgr_ApplySwitchPlan(const TaskMgrSwitchPlan_t* plan)
{
    if(plan == NULL)
    {
        return pdFALSE;
    }

    TaskHandle_t resumeTaskHandle = TaskMgr_GetHandle(plan->ResumeTaskId);
    TaskHandle_t suspendTaskHandle = TaskMgr_GetHandle(plan->SuspendTaskId);

    BaseType_t ret = pdFALSE;
    if(ResumeIfNeeded(resumeTaskHandle)) ret = pdTRUE;
    if(suspendTaskHandle != resumeTaskHandle)
    {
        if(SuspendIfNeeded(suspendTaskHandle)) ret = pdTRUE;
    }
    return ret;
}

/*
 * 函数功能：挂起前台任务
 * 入口参数：无
 * 返回值  ：挂起任务的掩码
 */
uint32_t SuspendForegroundTasks(void)
{
    uint32_t mark = 0U;

    OLED_I2C_Lock(); // 挂起前先锁住OLED，避免在挂起过程中被UI任务打断导致死锁
    MPU6050_I2C_Lock(); // 挂起前先锁住MPU6050，避免在挂起过程中被MPU6050任务打断导致死锁
    for(uint8_t i = 0U; i < sizeof(gTaskMap) / sizeof(gTaskMap[0]); i++)
    {
        if(gTaskMap[i].ForefroundMark == 0U)
        {
            continue;
        }

        if(SuspendIfNeeded(*(gTaskMap[i].HandleRef)))
        {
            mark |= gTaskMap[i].ForefroundMark;
        }
    }
    MPU6050_I2C_Unlock(); // 解锁MPU6050
    OLED_I2C_Unlock(); // 解锁OLED

    return mark;
}

/*
 * 函数功能：按掩码恢复前台任务
 * 入口参数：mask 挂起掩码
 * 返回值  ：无
 */
void ResumeForegroundTasks(uint32_t mark)
{
    if(mark == 0U)
    {
        // 如果没有指定恢复哪个任务，默认恢复UI任务，避免场景丢失
        TaskHandle_t uiHandle = TaskMgr_GetHandle(TASKMGR_TASK_UI);
        ResumeIfNeeded(uiHandle);
        return;
    }

    for(uint8_t i = 0U; i < sizeof(gTaskMap) / sizeof(gTaskMap[0]); i++)
    {
        if(gTaskMap[i].ForefroundMark == 0U)
        {
            continue;
        }
        if(mark & gTaskMap[i].ForefroundMark)
        {
            ResumeIfNeeded(*(gTaskMap[i].HandleRef));
        }
    }
}
