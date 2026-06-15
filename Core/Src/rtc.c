/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */
#define RTC_INIT_MAGIC 0x5AA5U //RTC初始化魔术字
/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
//检测是否写入魔术字，若写入则说明RTC的时间已经初始化过
if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != RTC_INIT_MAGIC){
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_INIT_MAGIC); //写入魔术字
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 14;
  sTime.Minutes = 30;
  sTime.Seconds = 0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_SATURDAY;
  DateToUpdate.Month = RTC_MONTH_JUNE;
  DateToUpdate.Date = 13;
  DateToUpdate.Year = 26;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, (((uint16_t)DateToUpdate.Year) << 8) | DateToUpdate.Month); //将年月写入备份寄存器，方便后续读取
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, (((uint16_t)DateToUpdate.Date) << 8) | DateToUpdate.WeekDay); //将日和星期写入备份寄存器，方便后续读取
}else{
  /* 已经初始化过RTC，直接从备份寄存器读取年月日和星期，设置RTC的日期 */
  uint16_t year_month = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
  uint16_t date_weekday = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
  DateToUpdate.Year = (uint8_t)(year_month >> 8);
  DateToUpdate.Month = (uint8_t)(year_month & 0xFF);
  DateToUpdate.Date = (uint8_t)(date_weekday >> 8);
  DateToUpdate.WeekDay = (uint8_t)(date_weekday & 0xFF);
  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
}
  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();

    /* RTC interrupt Init */
    HAL_NVIC_SetPriority(RTC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();

    /* RTC interrupt Deinit */
    HAL_NVIC_DisableIRQ(RTC_IRQn);
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
