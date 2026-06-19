#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>

/* 启动 PWM 输出 */
void bsp_pwm_start(void);

/* 停止 PWM 输出 */
void bsp_pwm_stop(void);

/**
 * 设置 PWM 占空比
 * @param value  0 = 0%，1000 = 100%
 */
void bsp_pwm_set(uint16_t value);

#endif