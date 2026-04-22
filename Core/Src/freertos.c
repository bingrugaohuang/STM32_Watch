/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "Key.h"
#include "usart.h"
#include "OLED.h"
#include "task_entry.h"
#include "StopWatch_APP.h"
#include "dino.h"
#include <timers.h>
#include <stdio.h>
#include <string.h>
#include "SleepManager.h"
#include "StackMonitor_Task.h"
#include "AlarmService.h"
#include "Log.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
TaskHandle_t KeyTaskHandle;
TaskHandle_t MenuTaskHandle;
TaskHandle_t UITaskHandle;
TaskHandle_t SetTaskHandle;
TaskHandle_t TimeTaskHandle;
TaskHandle_t FlashlightTaskHandle;
TaskHandle_t MPU6050TaskHandle;
TaskHandle_t GameTaskHandle;
TaskHandle_t StackMonitorTaskHandle;
TaskHandle_t SleepManagerTaskHandle;
TaskHandle_t AlarmTaskHandle;

TimerHandle_t StopWatchTimerHandle;
TimerHandle_t KeyTimerHandle;
TimerHandle_t DinoTimerHandle;

QueueHandle_t xKeyQueue;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

static BaseType_t CreateTaskWithLog(TaskFunction_t taskFunc,
                                    const char *name,
                                    uint16_t stackWords,
                                    UBaseType_t priority,
                                    TaskHandle_t *handle)
{
  BaseType_t ret = xTaskCreate(taskFunc, name, stackWords, NULL, priority, handle);
  if(ret == pdPASS)
  {
    LOG_I("TASK", "Create %s ok stack=%u prio=%lu", name, stackWords, (unsigned long)priority);
  }
  else
  {
    LOG_E("TASK", "Create %s failed stack=%u prio=%lu", name, stackWords, (unsigned long)priority);
  }
  return ret;
}

static TimerHandle_t CreateTimerWithLog(const char *name,
                                        uint32_t periodMs,
                                        TimerCallbackFunction_t callback)
{
  TimerHandle_t timer = xTimerCreate(name,
                                     pdMS_TO_TICKS(periodMs),
                                     pdTRUE,
                                     NULL,
                                     callback);
  if(timer != NULL)
  {
    LOG_I("TIMER", "Create %s ok period=%lums", name, (unsigned long)periodMs);
  }
  else
  {
    LOG_E("TIMER", "Create %s failed period=%lums", name, (unsigned long)periodMs);
  }
  return timer;
}

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	LOG_I("TASK", "StartDefaultTask begin");
	CreateTaskWithLog(Key_Task, "Key_Task", 128, osPriorityHigh, &KeyTaskHandle);
  CreateTaskWithLog(UI_Task, "UI_Task", 192, osPriorityNormal, &UITaskHandle);
  CreateTaskWithLog(Set_Task, "Set_Task", 192, osPriorityNormal, &SetTaskHandle);
  CreateTaskWithLog(Menu_Task, "Menu_Task", 128, osPriorityNormal, &MenuTaskHandle);
  CreateTaskWithLog(Time_Task, "Time_Task", 192, osPriorityNormal, &TimeTaskHandle);
  CreateTaskWithLog(Flashlight_Task, "Flashlight_Task", 128, osPriorityNormal, &FlashlightTaskHandle);
  CreateTaskWithLog(MPU6050_Task, "MPU6050_Task", 256, osPriorityNormal, &MPU6050TaskHandle);
  CreateTaskWithLog(Game_Task, "Game_Task", 128, osPriorityNormal, &GameTaskHandle);
  CreateTaskWithLog(SleepManager_Task, "SleepManager_Task", 128, osPriorityNormal, &SleepManagerTaskHandle);
  CreateTaskWithLog(StackMonitor_Task, "StackMon_Task", 192, osPriorityNormal, &StackMonitorTaskHandle);

  Alarm_ServiceInit();//初始化闹钟配置并下发RTC闹钟
  LOG_I("ALARM", "Alarm service initialized");
  CreateTaskWithLog(Alarm_Task, "Alarm_Task", 256, osPriorityAboveNormal, &AlarmTaskHandle);


  StopWatchTimerHandle = CreateTimerWithLog("StopWatch_Timer", 1000U, StopWatch_Tick);
  KeyTimerHandle = CreateTimerWithLog("Key_Timer", 10U, Key_Tick);
  DinoTimerHandle = CreateTimerWithLog("Dino_Timer", 10U, Dino_Tick);

  vTaskSuspend(MenuTaskHandle);
  vTaskSuspend(SetTaskHandle);
  vTaskSuspend(TimeTaskHandle);
  vTaskSuspend(FlashlightTaskHandle);
  vTaskSuspend(MPU6050TaskHandle);
  vTaskSuspend(GameTaskHandle);
	
  vTaskSuspend(StackMonitorTaskHandle);

  LOG_I("TASK", "Foreground tasks suspended for startup scene");


	vTaskDelete(NULL);
  (void)argument;
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  (void)xTask;
  (void)pcTaskName;
  taskDISABLE_INTERRUPTS();
  HAL_UART_Transmit_DMA(&huart1, (uint8_t * )pcTaskName, strlen(pcTaskName));
  for(;;)
  {
  }
}

/* USER CODE END Application */
