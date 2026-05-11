#include "bsp_button.h"
#include "stm32f1xx_hal.h"
#include "main.h"

/**
 * 函    数：读取按钮Last的状态
 */
uint8_t bsp_btn_last_read(void)
{
    if(HAL_GPIO_ReadPin(BTN_LAST_GPIO_Port, BTN_LAST_Pin) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * 函    数：读取按钮Next的状态
 */
uint8_t bsp_btn_next_read(void)
{
    if(HAL_GPIO_ReadPin(BTN_NEXT_GPIO_Port, BTN_NEXT_Pin) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/**
 * 函    数：读取按钮Cfm的状态
 */
uint8_t bsp_btn_cfm_read(void)
{
    if(HAL_GPIO_ReadPin(BTN_CFM_GPIO_Port, BTN_CFM_Pin) == 0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
