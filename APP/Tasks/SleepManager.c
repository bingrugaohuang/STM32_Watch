#include "SleepManager.h"
#include "AlarmService.h"

extern void SystemClock_Config(void);

static SleepState_t gSleepState = SLEEP_STATE_ACTIVE; // 全局睡眠状态变量，初始为活跃状态

static TickType_t gLastActiveTick = 0; // 上次活跃的系统时钟计数，用于判断是否进入睡眠状态
static TickType_t gSleepEnterTick = 0; // 实际进入睡眠状态的系统时钟计数，用于计算睡眠持续时间

static TickType_t gSleepTimeoutTicks = 0; // 进入短时睡眠（Sleep）的超时时间
static TickType_t gStopTimeoutTicks = 0; // 从Sleep升级到Stop的超时时间

static volatile uint8_t gWakeupPending = 0; // 是否有待处理的唤醒事件，避免重复处理同一事件
static volatile uint8_t gDiscardWakeKey = 0; // 睡眠期间用于唤醒的按键是否需要丢弃

static uint32_t gSuspendedTaskMask = 0U;// 记录进入睡眠前被本任务挂起的前台任务，唤醒时只恢复这批任务，避免固定恢复UI导致场景丢失
static uint8_t gMotionWakeArmed = 0U; // 记录是否已经配置了MPU6050运动唤醒
static uint8_t gStopContext = 0U; // 记录本次唤醒是否来自Stop返回

// 清除可能残留的唤醒中断挂起，避免刚入睡就被历史中断立刻拉起
static void SleepMgr_ClearWakeIrqPending(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(MPU_INT_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_LAST_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_CONFIRM_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(KEY_NEXT_Pin);

    HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI4_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

    __HAL_RTC_ALARM_EXTI_CLEAR_FLAG();
    HAL_NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
}


// 恢复任务，如果任务句柄有效且当前处于挂起状态
static void ResumeIfNeeded(TaskHandle_t taskHandle)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) == eSuspended)
    {
        vTaskResume(taskHandle);
    }
}

// 挂起任务并记录该任务在睡眠前是运行态，唤醒时只恢复这批任务
static void SuspendAndRecord(TaskHandle_t taskHandle, uint32_t taskBit)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) != eSuspended)
    {
        vTaskSuspend(taskHandle);
        gSuspendedTaskMask |= taskBit;
    }
}

// 挂起所有前台任务，准备进入睡眠状态
static void SuspendForegroundTasks(void)
{
    gSuspendedTaskMask = 0U;
    SuspendAndRecord(UITaskHandle, SLEEP_TASK_BIT_UI);
    SuspendAndRecord(MenuTaskHandle, SLEEP_TASK_BIT_MENU);
    SuspendAndRecord(TimeTaskHandle, SLEEP_TASK_BIT_STOPWATCH);
    SuspendAndRecord(FlashlightTaskHandle, SLEEP_TASK_BIT_FLASHLIGHT);
    SuspendAndRecord(MPU6050TaskHandle, SLEEP_TASK_BIT_MPU6050);
    SuspendAndRecord(GameTaskHandle, SLEEP_TASK_BIT_GAME);
    SuspendAndRecord(SetTaskHandle, SLEEP_TASK_BIT_SET);
}

// 唤醒时恢复睡眠前被本任务挂起的前台任务，避免固定恢复UI导致场景丢失
static uint8_t ResumeForegroundTasks(void)
{
    uint8_t resumedCount = 0U;

    if((gSuspendedTaskMask & SLEEP_TASK_BIT_UI) != 0U)
    {
        ResumeIfNeeded(UITaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_MENU) != 0U)
    {
        ResumeIfNeeded(MenuTaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_STOPWATCH) != 0U)
    {
        ResumeIfNeeded(TimeTaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_FLASHLIGHT) != 0U)
    {
        ResumeIfNeeded(FlashlightTaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_MPU6050) != 0U)
    {
        ResumeIfNeeded(MPU6050TaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_GAME) != 0U)
    {
        ResumeIfNeeded(GameTaskHandle);
        resumedCount++;
    }
    if((gSuspendedTaskMask & SLEEP_TASK_BIT_SET) != 0U)
    {
        ResumeIfNeeded(SetTaskHandle);
        resumedCount++;
    }

    gSuspendedTaskMask = 0U;
    return resumedCount;
}

// 配置MPU6050运动唤醒，用于Sleep和Stop期间的抬腕唤醒
static void SleepMgr_ArmMotionWakeup(void)
{
    if(gMotionWakeArmed == 0U)
    {
        MPU6050_EnableMotionWakeup(10, 6, 3); 
        MPU6050_ClearIntStatus();
    
        SleepMgr_ClearWakeIrqPending();
        gMotionWakeArmed = 1U;
    }
}

// 恢复MPU6050正常采样模式，退出运动唤醒配置
static void SleepMgr_DisarmMotionWakeup(void)
{
    if(gMotionWakeArmed != 0U)
    {
        MPU6050_ClearIntStatus();
        MPU6050_DisableMotionWakeup();
        SleepMgr_ClearWakeIrqPending();
        gMotionWakeArmed = 0U;
    }
}


// 初始化睡眠管理器，设置进入睡眠状态的超时时间
static void SleepMgr_Init(TickType_t SleepTimeoutTicks, TickType_t StopTimeoutTicks)
{
    gSleepState = SLEEP_STATE_ACTIVE; // 初始状态为活跃
    gLastActiveTick = xTaskGetTickCount();
    gSleepTimeoutTicks = SleepTimeoutTicks;
    gStopTimeoutTicks = StopTimeoutTicks;
    gSuspendedTaskMask = 0U;
    gWakeupPending = 0U;
    gMotionWakeArmed = 0U;
    gStopContext = 0U;
}

// 进入Stop并在唤醒后返回，函数返回即代表已收到唤醒事件
static void SleepMgr_EnterStopAndRecover(void)
{
    SleepMgr_ArmMotionWakeup();

    taskENTER_CRITICAL();
    gWakeupPending = 0U;
    taskEXIT_CRITICAL();

    // 清EXTI和NVIC pending，避免进入Stop后被旧中断立刻拉起
    SleepMgr_ClearWakeIrqPending();

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU); // 清除唤醒标志，准备进入停止模式
    HAL_SuspendTick(); // 暂停系统时钟，准备进入停止模式
    gStopContext = 1U;

    // 进入停止模式，等待唤醒事件；返回后表示已经被中断唤醒
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); // 进入停止模式，等待唤醒事件

    gWakeupPending = 1U;
    gSleepState = SLEEP_STATE_WAKEUP;
}

// 睡眠管理任务，负责监测系统活动并管理睡眠状态切换
void SleepManager_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  TickType_t now = 0;
    SleepMgr_Init(pdMS_TO_TICKS(30000), pdMS_TO_TICKS(30000)); 
    
  while(1)
  {
    switch (gSleepState)
    {
        case SLEEP_STATE_ACTIVE:
            now = xTaskGetTickCount();
            if(now - gLastActiveTick >= gSleepTimeoutTicks)
            {
                gSleepState = SLEEP_STATE_PREPARE_SLEEP; // 切换到准备睡眠状态
            }
            break;
        case SLEEP_STATE_PREPARE_SLEEP:
            OLED_I2C_Lock();
            MPU6050_I2C_Lock();

            SuspendForegroundTasks(); // 在确保I2C空闲后挂起任务，避免在I2C发送间隙把任务卡死死锁

            MPU6050_I2C_Unlock(); // 已经挂起前台任务，MPU6050肯定空闲了

            OLED_Clear();
            OLED_Update();

            OLED_I2C_Unlock(); // 解锁OLED

            SleepMgr_ArmMotionWakeup(); // Sleep阶段也保持抬腕唤醒能力
            taskENTER_CRITICAL();
            gWakeupPending = 0U;// 切换到睡眠状态前清除待处理标志，避免刚入睡就被历史事件立刻拉起
            taskEXIT_CRITICAL();
            SleepMgr_ClearWakeIrqPending();

            gSleepEnterTick = xTaskGetTickCount(); // 记录进入睡眠状态的时间
            gSleepState = SLEEP_STATE_SLEEPING; // 切换到睡眠状态
            break;
        case SLEEP_STATE_SLEEPING:
            if(gWakeupPending) // 如果有待处理的唤醒事件
            {
                gWakeupPending = 0; // 清除待处理标志，进入唤醒流程
                gSleepState = SLEEP_STATE_WAKEUP; // 切换到唤醒状态
                break;
            }
            else
            {
                now = xTaskGetTickCount();
                if(now - gSleepEnterTick >= gStopTimeoutTicks)
                {
                    gSleepState = SLEEP_STATE_PREPARE_STOP; // 切换到准备停止状态
                    break;
                }
            }

            // 短时睡眠：仅停止CPU，不关闭外设；任意中断（按键/MPU/系统节拍）都可唤醒CPU
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;
        case SLEEP_STATE_PREPARE_STOP:
            // 进入停止模式前的准备工作，确保所有外设都已安全关闭
            SleepMgr_EnterStopAndRecover(); // 准备进入停止模式并处理唤醒
            break;
        case SLEEP_STATE_STOP:
            // 保留兼容状态，正常流程不会停留在该状态
            gSleepState = SLEEP_STATE_WAKEUP;
            break;
        case SLEEP_STATE_WAKEUP:
            gLastActiveTick = xTaskGetTickCount(); // 更新最后活跃时间

            if(gStopContext != 0U)
            {
                SystemClock_Config(); // 仅Stop返回后才需要恢复系统时钟
                HAL_ResumeTick();
                MX_I2C1_Init();
                MX_I2C2_Init();
                gStopContext = 0U;
            }

            SleepMgr_DisarmMotionWakeup(); // Sleep/Stop唤醒后统一恢复MPU6050正常模式

            // 若闹钟提醒页已接管显示，则不在这里恢复前台任务，避免抢屏闪屏
            if(Alarm_ServiceIsRinging() != 0U)
            {
                gSleepState = SLEEP_STATE_ACTIVE;
                break;
            }

            // 恢复睡眠前实际运行的任务，确保被挂起在显示/I2C路径中的任务可以继续收尾
            if(ResumeForegroundTasks() == 0U)
            {
                ResumeIfNeeded(UITaskHandle);
            }
            gSleepState = SLEEP_STATE_ACTIVE; // 切换回活跃状态
            break;
    }

    if(gSleepState == SLEEP_STATE_SLEEPING)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    else
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

// 活动报告函数，供其他任务调用以更新最后活跃时间并可能触发唤醒
void SleepMgr_ReportActivity(void)
{
    gLastActiveTick = xTaskGetTickCount(); // 更新最后活跃时间
    taskENTER_CRITICAL();
    if(gSleepState == SLEEP_STATE_SLEEPING ||
       gSleepState == SLEEP_STATE_PREPARE_STOP ||
       gSleepState == SLEEP_STATE_STOP)
    {
        gWakeupPending = 1U;
    }
    taskEXIT_CRITICAL();
}

/*
 * 函数功能：从中断服务程序中报告唤醒事件
 * 入口参数：GPIO_Pin 唤醒按键的GPIO引脚
 * 返回值  ：无
 */
void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin)
{
    gLastActiveTick = xTaskGetTickCount(); // 更新最后活跃时间

    if((GPIO_Pin == KEY_CONFIRM_Pin) || (GPIO_Pin == KEY_NEXT_Pin) || (GPIO_Pin == KEY_LAST_Pin))
    {
        if((gSleepState == SLEEP_STATE_SLEEPING) ||
           (gSleepState == SLEEP_STATE_PREPARE_STOP) ||
           (gSleepState == SLEEP_STATE_STOP))
        {
            gDiscardWakeKey = 1U;
        }
    }
    gWakeupPending = 1U; // 设置有待处理的唤醒事件标志
}

/*
 * 函数功能：从中断服务程序中消费唤醒按键丢弃标志
 * 入口参数：无
 * 返回值  ：丢弃标志状态
 */
uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void)
{
    uint8_t discard = gDiscardWakeKey;
    gDiscardWakeKey = 0U;
    return discard;
}
