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

#define RTC_INIT_MAGIC 0x5AA5U
#define RTC_DATE_BKP_DR RTC_BKP_DR6
#define RTC_DATE_CRC_BKP_DR RTC_BKP_DR7

static uint16_t RTC_PackDate(const RTC_DateTypeDef *date)
{
  return (uint16_t)((((uint16_t)date->Year & 0x7FU) << 9) |
                    (((uint16_t)date->Month & 0x0FU) << 5) |
                    ((uint16_t)date->Date & 0x1FU));
}

static uint8_t RTC_LoadDateFromBackup(RTC_DateTypeDef *date)
{
  uint16_t packed;
  uint16_t crc;

  if (date == NULL)
  {
    return 0U;
  }

  packed = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_DATE_BKP_DR);
  crc = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_DATE_CRC_BKP_DR);

  if ((uint16_t)(packed ^ 0xA5A5U) != crc)
  {
    return 0U;
  }

  date->Year = (uint8_t)((packed >> 9) & 0x7FU);
  date->Month = (uint8_t)((packed >> 5) & 0x0FU);
  date->Date = (uint8_t)(packed & 0x1FU);
  date->WeekDay = RTC_WEEKDAY_MONDAY;

  if ((date->Month < 1U) || (date->Month > 12U) || (date->Date < 1U) || (date->Date > 31U))
  {
    return 0U;
  }

  return 1U;
}

void RTC_SaveDateToBackup(const RTC_DateTypeDef *date)
{
  uint16_t packed;

  if (date == NULL)
  {
    return;
  }

  packed = RTC_PackDate(date);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_DATE_BKP_DR, packed);
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_DATE_CRC_BKP_DR, (uint16_t)(packed ^ 0xA5A5U));
}

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
	/*
	 * 函数功能：只在首次上电时写入默认时间，避免每次复位都重置RTC
	 * 实现方式：使用BKP寄存器DR1保存初始化魔术字
	 */
	if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != RTC_INIT_MAGIC)
	{
	  sTime.Hours = 23;
	  sTime.Minutes = 59;
	  sTime.Seconds = 0;

	  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	  {
		Error_Handler();
	  }

	  DateToUpdate.WeekDay = RTC_WEEKDAY_FRIDAY;
	  DateToUpdate.Month = RTC_MONTH_APRIL;
	  DateToUpdate.Date = 20;
	  DateToUpdate.Year = 26;

	  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
	  {
		Error_Handler();
	  }

	  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, RTC_INIT_MAGIC);
    RTC_SaveDateToBackup(&DateToUpdate);
  }
  else
  {
    if (RTC_LoadDateFromBackup(&DateToUpdate) != 0U)
    {
    if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
    {
      Error_Handler();
    }
    }
	}

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
 
  /* USER CODE BEGIN RTC_Init 2 */

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

    /*
     * 功能：使能RTC闹钟中断，用于Alarm唤醒与提醒
     * 说明：优先级设置为5，与当前FreeRTOS中断优先级约束保持一致
     */
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

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
