#include "flashlight_service.h"
#include "bsp_pwm.h"

#define CCR_MAX 1000

static uint8_t  s_is_on  = 0;      // 当前手电是否点亮
static int16_t  s_target = 500;    // 目标占空比值（0-1000）
static float    s_filter = 500.0f; // 当前滤波状态（0.0-1000.0），避免整数步进截断
static uint16_t s_ccr    = 500;    // 当前实际 CCR（0-1000），经过滤波后四舍五入

/* 打开手电（保留当前亮度）*/
void flashlight_on(void)
{
    s_is_on = 1;
    bsp_pwm_start();
    bsp_pwm_set(s_ccr);
}

/* 关闭手电 */
void flashlight_off(void)
{
    s_is_on = 0;
    bsp_pwm_stop();
}

/* 当前是否点亮 */  
uint8_t flashlight_is_on(void) { return s_is_on; }

/* 修改目标亮度，正=增亮，负=减暗 */
void flashlight_adjust(int16_t delta)
{
    int32_t t = (int32_t)s_target + delta;
    if (t < 0)       t = 0;
    if (t > CCR_MAX) t = CCR_MAX;
    s_target = (int16_t)t;
}

/* 
 * 推进低通滤波并写入 BSP，每渲染帧调用一次
 * 不点亮时也可调用（只更新内部状态，不写 BSP）
 */
void flashlight_tick(void)
{
    float diff    = (float)s_target - s_filter;
    float absDiff = (diff >= 0.0f) ? diff : -diff;

    if (absDiff > 0.5f) {
        /* 差值越大，alpha 越大，追踪越快，减少体感延迟 */
        float alpha = 0.26f + absDiff * 0.00042f;
        if (alpha > 0.85f) alpha = 0.85f;

        float step = diff * alpha;

        /* 最小步进，防止"快到目标却慢慢挪"的拖尾感 */
        const float MIN_STEP = 2.5f;
        if (step > 0.0f && step <  MIN_STEP) step =  MIN_STEP;
        if (step < 0.0f && step > -MIN_STEP) step = -MIN_STEP;

        /* 防止单帧跨过目标值 */
        if (step > 0.0f && step > diff) step = diff;
        if (step < 0.0f && step < diff) step = diff;

        s_filter += step;
    } else {
        s_filter = (float)s_target;
    }

    s_ccr = (uint16_t)(s_filter + 0.5f);
    if (s_is_on) bsp_pwm_set(s_ccr);
}

/* 当前实际 CCR（经过滤波）0-1000 */
uint16_t flashlight_get_ccr(void) { return s_ccr; }

/* 瞬间调整亮度（无滤波） */
void flashlight_adjust_snap(int16_t delta)
{
    int32_t t = (int32_t)s_target + delta;
    if (t < 0)       t = 0;
    if (t > CCR_MAX) t = CCR_MAX;
    s_target = (int16_t)t;
    s_filter = (float)s_target;          /* 直接吸附，跳过低通 */
    s_ccr    = (uint16_t)(s_target);
    if (s_is_on) bsp_pwm_set(s_ccr);    /* 立即写 PWM */
}
