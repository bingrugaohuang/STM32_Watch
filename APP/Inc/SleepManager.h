#ifndef _SLEEPMANAGER_H
#define _SLEEPMANAGER_H

#include "main.h"
#include "freertos.h"
#include "OLED.h"
#include "menu.h"
//#include "Key.h"
#include "task.h"
#include "MPU6050.h"

#define SLEEP_TASK_BIT_UI         (1U << 0)
#define SLEEP_TASK_BIT_MENU       (1U << 1)
#define SLEEP_TASK_BIT_STOPWATCH  (1U << 2)
#define SLEEP_TASK_BIT_FLASHLIGHT (1U << 3)
#define SLEEP_TASK_BIT_MPU6050    (1U << 4)
#define SLEEP_TASK_BIT_GAME       (1U << 5)
#define SLEEP_TASK_BIT_SET        (1U << 6)

typedef enum{
    SLEEP_STATE_ACTIVE = 0,       // »îÔ¾×´Ì¬
    SLEEP_STATE_PREPARE_SLEEP,    // ×¼±¸Ë¯Ãß×´Ì¬
    SLEEP_STATE_SLEEPING,         // Ë¯Ãß×´Ì¬
    SLEEP_STATE_PREPARE_STOP,     // ×¼±¸Í£Ö¹×´Ì¬
    SLEEP_STATE_STOP,             // Í£Ö¹×´Ì¬
    SLEEP_STATE_WAKEUP            // »½ÐÑ×´Ì¬
} SleepState_t;

void SleepMgr_ReportActivity(void);
void SleepManager_Task(void *argument);

void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin);
uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void);

#endif
