#ifndef _KEY_H
#define _KEY_H

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
//#include "SleepManager.h"

extern QueueHandle_t xKeyQueue;//键值队列
extern TaskHandle_t KeyTaskHandle;

extern TimerHandle_t KeyTimerHandle;//软件定时器句柄

extern void SleepMgr_ReportActivity(void);
extern void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin);
extern uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void);

uint8_t Key_GetNum(void);//读取键值

#define KEY_CONFIRM      1
#define KEY_CONFIRM_LONG 2
#define KEY_NEXT         3
#define KEY_NEXT_LONG    4
#define KEY_LAST         5
#define KEY_LAST_LONG    6  

void Key(void);
void Key_Tick(TimerHandle_t xTimer);
uint8_t Key_GetNum(void);//键值获取函数，返回值为0表示无按键，1、2、3分别表示确认键、下一个键和上一个键


#endif
