#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdint.h>

/**
 * 函    数：读取 "上一个" 按钮状态
 * 参    数：无
 * 返 回 值：0 按下，1 未按下
 */
uint8_t bsp_btn_last_read(void);

/**
 * 函    数：读取 "下一个" 按钮状态
 * 参    数：无
 * 返 回 值：0 按下，1 未按下
 */
uint8_t bsp_btn_next_read(void);

/**
 * 函    数：读取 "确认" 按钮状态
 * 参    数：无
 * 返 回 值：0 按下，1 未按下
 */
uint8_t bsp_btn_cfm_read(void);

#endif /* BSP_BUTTON_H */