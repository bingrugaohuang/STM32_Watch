/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "common_macro.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : BTN_CFM_Pin */
  GPIO_InitStruct.Pin = BTN_CFM_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_CFM_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_NEXT_Pin BTN_LAST_Pin */
  GPIO_InitStruct.Pin = BTN_NEXT_Pin|BTN_LAST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 2 */
#if MPU_MOT_TEST_ENABLE

#define MPU_INT_Pin GPIO_PIN_0
#define MPU_INT_GPIO_Port GPIOB

void mot_test_gpio_init(void){
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : PB0 for tickless timing marker */
    GPIO_InitStruct.Pin = MPU_INT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(MPU_INT_GPIO_Port, &GPIO_InitStruct);

}

void mot_test_gpio_set_high(void){
  HAL_GPIO_WritePin(MPU_INT_GPIO_Port, MPU_INT_Pin, GPIO_PIN_SET);
}

void mot_test_gpio_set_low(void){
  HAL_GPIO_WritePin(MPU_INT_GPIO_Port, MPU_INT_Pin, GPIO_PIN_RESET);
}

void mot_test_gpio_togle_pin(void){
  HAL_GPIO_TogglePin(MPU_INT_GPIO_Port, MPU_INT_Pin);
}
#endif
/* USER CODE END 2 */
