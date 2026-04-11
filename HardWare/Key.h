#ifndef _KEY_H
#define _KEY_H

#include "main.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"
//#include "services/SleepManager.h"
#include "common/app_handles.h"

extern void SleepMgr_ReportActivity(void);
extern void SleepMgr_ReportWakeupFromISR(uint16_t GPIO_Pin);
extern uint8_t SleepMgr_ConsumeWakeKeyDiscardFlagFromISR(void);

uint8_t Key_GetNum(void);//��ȡ��ֵ

#define KEY_CONFIRM      1
#define KEY_CONFIRM_LONG 2
#define KEY_NEXT         3
#define KEY_NEXT_LONG    4
#define KEY_LAST         5
#define KEY_LAST_LONG    6  

void Key(void);
void Key_Tick(TimerHandle_t xTimer);
uint8_t Key_GetNum(void);//��ֵ��ȡ����������ֵΪ0��ʾ�ް�����1��2��3�ֱ��ʾȷ�ϼ�����һ��������һ����


#endif
