#include "SleepManager.h"

extern void SystemClock_Config(void);

static SleepState_t gSleepState = SLEEP_STATE_ACTIVE; // ȫ��˯��״̬��������ʼΪ��Ծ״̬

static TickType_t gLastActiveTick = 0; // �ϴλ�Ծ��ϵͳʱ�Ӽ����������ж��Ƿ����˯��״̬
static TickType_t gSleepEnterTick = 0; // ʵ�ʽ���˯��״̬��ϵͳʱ�Ӽ��������ڼ���˯�߳���ʱ��

static TickType_t gSleepTimeoutTicks = 0; // �����ʱ˯�ߣ�Sleep���ĳ�ʱʱ��
static TickType_t gStopTimeoutTicks = 0; // ��Sleep������Stop�ĳ�ʱʱ��

static volatile uint8_t gWakeupPending = 0; // �Ƿ��д������Ļ����¼��������ظ�����ͬһ�¼�
static volatile TickType_t gWakeupIgnoreKeyUntil = 0; // ���߰����¼��Ľ�ֹTickʱ�䣬����������ͱ�����������������˯��

static uint32_t gSuspendedTaskMask = 0U;// ��¼����˯��ǰ������������ǰ̨���񣬻���ʱֻ�ָ��������񣬱���̶��ָ�UI���³�����ʧ
static uint8_t gMotionWakeArmed = 0U; // ��¼�Ƿ��Ѿ�������MPU6050�˶�����
static uint8_t gStopContext = 0U; // ��¼���λ����Ƿ�����Stop����

// ������ܲ����Ļ����жϹ��𣬱������˯�ͱ���ʷ�ж���������
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


// �ָ����������������Ч�ҵ�ǰ���ڹ���״̬
static void ResumeIfNeeded(TaskHandle_t taskHandle)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) == eSuspended)
    {
        vTaskResume(taskHandle);
    }
}

// �������񲢼�¼��������˯��ǰ������̬������ʱֻ�ָ���������
static void SuspendAndRecord(TaskHandle_t taskHandle, uint32_t taskBit)
{
    if(taskHandle != NULL && eTaskGetState(taskHandle) != eSuspended)
    {
        vTaskSuspend(taskHandle);
        gSuspendedTaskMask |= taskBit;
    }
}

// ��������ǰ̨����׼������˯��״̬
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

// ����ʱ�ָ�˯��ǰ������������ǰ̨���񣬱���̶��ָ�UI���³�����ʧ
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

// ����MPU6050�˶����ѣ�����Sleep��Stop�ڼ��̧����
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

// �ָ�MPU6050��������ģʽ���˳��˶���������
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


// ��ʼ��˯�߹����������ý���˯��״̬�ĳ�ʱʱ��
static void SleepMgr_Init(TickType_t SleepTimeoutTicks, TickType_t StopTimeoutTicks)
{
    gSleepState = SLEEP_STATE_ACTIVE; // ��ʼ״̬Ϊ��Ծ
    gLastActiveTick = xTaskGetTickCount();
    gSleepTimeoutTicks = SleepTimeoutTicks;
    gStopTimeoutTicks = StopTimeoutTicks;
    gSuspendedTaskMask = 0U;
    gWakeupPending = 0U;
    gMotionWakeArmed = 0U;
    gStopContext = 0U;
}

// ����Stop���ڻ��Ѻ󷵻أ��������ؼ��������յ������¼�
static void SleepMgr_EnterStopAndRecover(void)
{
    SleepMgr_ArmMotionWakeup();

    taskENTER_CRITICAL();
    gWakeupPending = 0U;
    taskEXIT_CRITICAL();

    // ��EXTI��NVIC pending���������Stop�󱻾��ж���������
    SleepMgr_ClearWakeIrqPending();

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU); // ������ѱ�־��׼������ֹͣģʽ
    HAL_SuspendTick(); // ��ͣϵͳʱ�ӣ�׼������ֹͣģʽ
    gStopContext = 1U;

    // ����ֹͣģʽ���ȴ������¼������غ��ʾ�Ѿ����жϻ���
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); // ����ֹͣģʽ���ȴ������¼�

    gWakeupPending = 1U;
    gSleepState = SLEEP_STATE_WAKEUP;
}

// ˯�߹������񣬸�����ϵͳ�������˯��״̬�л�
void SleepManager_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  TickType_t now = 0;
    SleepMgr_Init(pdMS_TO_TICKS(10000), pdMS_TO_TICKS(10000)); 
    
  while(1)
  {
    switch (gSleepState)
    {
        case SLEEP_STATE_ACTIVE:
            now = xTaskGetTickCount();
            if(now - gLastActiveTick >= gSleepTimeoutTicks)
            {
                gSleepState = SLEEP_STATE_PREPARE_SLEEP; // �л���׼��˯��״̬
            }
            break;
        case SLEEP_STATE_PREPARE_SLEEP:
            OLED_I2C_Lock();
            MPU6050_I2C_Lock();

            SuspendForegroundTasks(); // ��ȷ��I2C���к�������񣬱�����I2C���ͼ�϶������������

            MPU6050_I2C_Unlock(); // �Ѿ�����ǰ̨����MPU6050�϶�������

            OLED_Clear();
            OLED_Update();

            OLED_I2C_Unlock(); // ����OLED

            SleepMgr_ArmMotionWakeup(); // Sleep�׶�Ҳ����̧��������
            taskENTER_CRITICAL();
            gWakeupPending = 0U;// �л���˯��״̬ǰ�����������־���������˯�ͱ���ʷ�¼���������
            taskEXIT_CRITICAL();
            SleepMgr_ClearWakeIrqPending();

            gSleepEnterTick = xTaskGetTickCount(); // ��¼����˯��״̬��ʱ��
            gSleepState = SLEEP_STATE_SLEEPING; // �л���˯��״̬
            break;
        case SLEEP_STATE_SLEEPING:
            if(gWakeupPending) // ����д������Ļ����¼�
            {
                gWakeupPending = 0; // �����������־�����뻽������
                gSleepState = SLEEP_STATE_WAKEUP; // �л�������״̬
                break;
            }
            else
            {
                now = xTaskGetTickCount();
                if(now - gSleepEnterTick >= gStopTimeoutTicks)
                {
                    gSleepState = SLEEP_STATE_PREPARE_STOP; // �л���׼��ֹͣ״̬
                    break;
                }
            }

            // ��ʱ˯�ߣ���ֹͣCPU�����ر����裻�����жϣ�����/MPU/ϵͳ���ģ����ɻ���CPU
            HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
            break;
        case SLEEP_STATE_PREPARE_STOP:
            // ����ֹͣģʽǰ��׼��������ȷ���������趼�Ѱ�ȫ�ر�
            SleepMgr_EnterStopAndRecover(); // ׼������ֹͣģʽ����������
            break;
        case SLEEP_STATE_STOP:
            // ��������״̬���������̲���ͣ���ڸ�״̬
            gSleepState = SLEEP_STATE_WAKEUP;
            break;
        case SLEEP_STATE_WAKEUP:
            gLastActiveTick = xTaskGetTickCount(); // ��������Ծʱ��

            if(gStopContext != 0U)
            {
                SystemClock_Config(); // ��Stop���غ����Ҫ�ָ�ϵͳʱ��
                HAL_ResumeTick();
                MX_I2C1_Init();
                MX_I2C2_Init();
                gStopContext = 0U;
            }

            SleepMgr_DisarmMotionWakeup(); // Sleep/Stop���Ѻ�ͳһ�ָ�MPU6050����ģʽ

            // ����������ҳ�ѽӹ���ʾ����������ָ�ǰ̨���񣬱�����������
            if(Alarm_ServiceIsRinging() != 0U)
            {
                gSleepState = SLEEP_STATE_ACTIVE;
                break;
            }

            // �ָ�˯��ǰʵ�����е�����ȷ������������ʾ/I2C·���е�������Լ�����β
            if(ResumeForegroundTasks() == 0U)
            {
                ResumeIfNeeded(UITaskHandle);
            }
            gSleepState = SLEEP_STATE_ACTIVE; // �л��ػ�Ծ״̬
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

// ����溯������������������Ը�������Ծʱ�䲢���ܴ�������
void SleepMgr_ReportActivity(void)
{
    gLastActiveTick = xTaskGetTickCount(); // ��������Ծʱ��
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
 * �������ܣ����жϷ�������б��滽���¼�
 * ��ڲ�����GPIO_Pin ���Ѱ�����GPIO����
 * ����ֵ  ����
 */
void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin)
{
    gLastActiveTick = xTaskGetTickCountFromISR(); // ��������Ծʱ��

    if((GPIO_Pin == KEY_CONFIRM_Pin) || (GPIO_Pin == KEY_NEXT_Pin) || (GPIO_Pin == KEY_LAST_Pin))
    {
        if((gSleepState == SLEEP_STATE_SLEEPING) ||
           (gSleepState == SLEEP_STATE_PREPARE_STOP) ||
           (gSleepState == SLEEP_STATE_STOP))
        {
            // ��¼��һ���������ܰ����¼���Tickʱ�䣬�����Ǵ�˯�������������ǰ�ס��ť�ܾ�
            // ������������300ms�İ����������򶶶����´����˵�����
            gWakeupIgnoreKeyUntil = gLastActiveTick + pdMS_TO_TICKS(400); 
        }
    }
    gWakeupPending = 1U; // �����д������Ļ����¼���־
}

/*
 * �������ܣ����жϷ�����������ѻ��Ѱ���������־
 * ��ڲ�������
 * ����ֵ  ��״̬�жϣ�1=������ǰ�����¼���0=����
 */
uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void)
{
    // Wrap-safe check: deadline is in the future when (deadline - now) < half the tick range
    if ((gWakeupIgnoreKeyUntil - xTaskGetTickCountFromISR()) < (portMAX_DELAY >> 1U))
    {
        return 1U;
    }
    return 0U;
}
