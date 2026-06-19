#include "bsp_pwm.h"
#include "tim.h"
/* 包含你的 HAL 头文件和 tim.h */

/* 启动 PWM 输出 */
void bsp_pwm_start(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
}

/* 停止 PWM 输出 */
void bsp_pwm_stop(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
}

/**
 * 设置 PWM 占空比
 * @param value  0 = 0%，1000 = 100%
 */
void bsp_pwm_set(uint16_t value)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, value);
}