#ifndef _SLEEPMANAGER_H
#define _SLEEPMANAGER_H

#include "main.h"
#include "freertos.h"
#include "OLED.h"
//#include "Key.h"
#include "task.h"
#include "MPU6050.h"
#include "common/app_handles.h"

#define SLEEP_TASK_BIT_UI         (1U << 0)
#define SLEEP_TASK_BIT_MENU       (1U << 1)
#define SLEEP_TASK_BIT_STOPWATCH  (1U << 2)
#define SLEEP_TASK_BIT_FLASHLIGHT (1U << 3)
#define SLEEP_TASK_BIT_MPU6050    (1U << 4)
#define SLEEP_TASK_BIT_GAME       (1U << 5)
#define SLEEP_TASK_BIT_SET        (1U << 6)

typedef enum{
    SLEEP_STATE_ACTIVE = 0,       // ��Ծ״̬
    SLEEP_STATE_PREPARE_SLEEP,    // ׼��˯��״̬
    SLEEP_STATE_SLEEPING,         // ˯��״̬
    SLEEP_STATE_PREPARE_STOP,     // ׼��ֹͣ״̬
    SLEEP_STATE_STOP,             // ֹͣ״̬
    SLEEP_STATE_WAKEUP            // ����״̬
} SleepState_t;

void SleepMgr_ReportActivity(void);
void SleepManager_Task(void *argument);

void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin);
uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void);

#endif
