#ifndef _MENU_H_
#define _MENU_H_

#include "main.h"
#include "freertos.h"
#include "Key.h"
#include "ShowMenu.h"
#include "Time_Task.h"
#include "Setting.h"
#include "UI.h"
#include "Flashlight.h"
#include "MPU6050_Task.h"
#include "Game.h"
#include "Gradienter.h"
#include "AlarmService.h"

extern TaskHandle_t KeyTaskHandle;//键值获取任务句柄
extern TaskHandle_t MenuTaskHandle;//菜单任务句柄
extern TaskHandle_t UITaskHandle;//UI任务句柄
extern TaskHandle_t SetTaskHandle;//设置任务句柄
extern TaskHandle_t TimeTaskHandle;//时间任务句柄
extern TaskHandle_t FlashlightTaskHandle;//手电筒任务句柄
extern TaskHandle_t MPU6050TaskHandle;//MPU6050任务句柄
extern TaskHandle_t GameTaskHandle;//游戏任务句柄
extern TaskHandle_t SleepManagerTaskHandle;//睡眠管理任务句柄
extern TaskHandle_t AlarmTaskHandle;//闹钟服务任务句柄
extern TaskHandle_t StackMonitorTaskHandle;//栈监控任务句柄


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

#endif
