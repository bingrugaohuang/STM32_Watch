#ifndef _TASK_ENTRY_H_
#define _TASK_ENTRY_H_

#include "common/app_common.h"

void Key_Task(void *argument);
void UI_Task(void *argument);
void Set_Task(void *argument);
void Menu_Task(void *argument);
void Time_Task(void *argument);
void Flashlight_Task(void *argument);
void MPU6050_Task(void *argument);
void Game_Task(void *argument);
void Gradienter_Task(void *argument);
void Alarm_Task(void *argument);
void SleepManager_Task(void *argument);
void StackMonitor_Task(void *argument);

#endif
