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
#include "Queue.h"
#include "Key.h"
#include "usart.h"
#include "OLED.h"
#include "tasks/task_entry.h"
#include "StopWatch_APP.h"
#include "dino.h"
#include <timers.h>
#include <stdio.h>
#include <string.h>
#include "SleepManager.h"
#include "StackMonitor_Task.h"
#include "AlarmService.h"
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
	xTaskCreate(Key_Task, "Key_Task", 128, NULL, osPriorityHigh, &KeyTaskHandle);
  xTaskCreate(UI_Task, "UI_Task", 192, NULL, osPriorityNormal, &UITaskHandle);
  xTaskCreate(Set_Task, "Set_Task", 192, NULL, osPriorityNormal, &SetTaskHandle);
  xTaskCreate(Menu_Task, "Menu_Task", 128, NULL, osPriorityNormal, &MenuTaskHandle);
  xTaskCreate(Time_Task, "Time_Task", 192, NULL, osPriorityNormal, &TimeTaskHandle);
  xTaskCreate(Flashlight_Task, "Flashlight_Task", 128, NULL, osPriorityNormal, &FlashlightTaskHandle);
  xTaskCreate(MPU6050_Task, "MPU6050_Task", 256, NULL, osPriorityNormal, &MPU6050TaskHandle);
  xTaskCreate(Game_Task, "Game_Task", 128, NULL, osPriorityNormal, &GameTaskHandle);
  xTaskCreate(SleepManager_Task, "SleepManager_Task", 128, NULL, osPriorityNormal, &SleepManagerTaskHandle);
  xTaskCreate(StackMonitor_Task, "StackMon_Task", 192, NULL, osPriorityNormal, &StackMonitorTaskHandle);
  xTaskCreate(Alarm_Task, "Alarm_Task", 256, NULL, osPriorityAboveNormal, &AlarmTaskHandle);

  Alarm_ServiceInit();//初始化闹钟配置并下发RTC闹钟

  StopWatchTimerHandle = 
    xTimerCreate( "StopWatch_Timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, StopWatch_Tick);
  KeyTimerHandle = 
    xTimerCreate( "Key_Timer", pdMS_TO_TICKS(10), pdTRUE, NULL, Key_Tick);
  DinoTimerHandle =
    xTimerCreate( "Dino_Timer", pdMS_TO_TICKS(10), pdTRUE, NULL, Dino_Tick);

  vTaskSuspend(MenuTaskHandle);
  vTaskSuspend(SetTaskHandle);
  vTaskSuspend(TimeTaskHandle);
  vTaskSuspend(FlashlightTaskHandle);
  vTaskSuspend(MPU6050TaskHandle);
  vTaskSuspend(GameTaskHandle);
	
  vTaskSuspend(StackMonitorTaskHandle);

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
