#ifndef FLASHLIGHT_SERVICE_H
#define FLASHLIGHT_SERVICE_H

#include <stdint.h>

/** 打开手电（保留当前亮度）*/
void     flashlight_on(void);

/** 关闭手电 */
void     flashlight_off(void);

/** 当前是否点亮 */
uint8_t  flashlight_is_on(void);

/**
 * 修改目标亮度，正=增亮，负=减暗
 * @param delta  建议步进：短按 50，长按 100
 */
void     flashlight_adjust(int16_t delta);

/**
 * 推进低通滤波并写入 BSP，每渲染帧调用一次
 * 不点亮时也可调用（只更新内部状态，不写 BSP）
 */
void     flashlight_tick(void);

/** 当前实际 CCR（经过滤波）0-1000 */
uint16_t flashlight_get_ccr(void);

/* 调节模式专用，跳过滤波 */
void flashlight_adjust_snap(int16_t delta);   

#endif

