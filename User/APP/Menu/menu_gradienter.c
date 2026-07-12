#include "menu_common.h"
#include <string.h>
#include <math.h>
#include "mpu6050_service.h"
#include "log.h"
#include "common_macro.h"
#include "oled_driver.h"
#include "stdio.h"

/* ====================静态变量======================*/
static Attitude_t current_attitude = {0}; // 用于存储当前的角度和步数

extern const uint8_t Return[]; // 返回图标


/*=====================公共函数======================*/
uint8_t menu_gradienter_get_attitude(void) {
    // 尝试从队列获取最新的角度和步数数据
    while (mpu6050_getqueue(&current_attitude) == COMMON_ERR_OK);
    LOG_D(TAG_DISP, "Current Attitude: Roll=%.2f, Pitch=%.2f, Steps=%lu",
         current_attitude.roll, current_attitude.pitch, current_attitude.step_cnt);
    return COMMON_ERR_OK;
}

/*=========================静态函数声明=========================*/
static void gradienter_on_enter(MenuNode_t *self);
static void gradienter_on_exit(MenuNode_t *self);  
static void gradienter_on_render(MenuNode_t *self);
static MenuResult_t gradienter_on_input(MenuNode_t *self, MenuEvent_t event);

/* ── 水平仪节点 ── */
MenuNode_t menu_gradienter = {
    .title = "Gradienter",
    .parent = &menu_applist,
    .children = NULL,
    .child_count = 0,
    .cursor = 0,

    .on_enter = gradienter_on_enter,
    .on_exit = gradienter_on_exit,
    .on_render = gradienter_on_render,
    .on_input = gradienter_on_input,

    .user_data = NULL,
};

/*=========================静态函数=========================*/
// 进入函数，用于在进入水平仪菜单时进行初始化
static void gradienter_on_enter(MenuNode_t *self) {
    menu_engine_set_animating(1);
}

// 退出函数，用于在退出水平仪菜单时进行清理
static void gradienter_on_exit(MenuNode_t *self) {
    // 在退出水平仪菜单时可以进行一些清理工作
    // 例如停止数据采集或释放资源
    // 目前没有需要清理的资源
    menu_engine_set_animating(0);
}

// 输入处理函数，用于处理用户输入事件
static MenuResult_t gradienter_on_input(MenuNode_t *self, MenuEvent_t event) {
    switch (event)
    {
        case MENU_EVENT_CONFIRM_SHORT:
            return MENU_RESULT_BACK; // 确认短按返回上一级菜单    
            //break;
    
        default:
            return MENU_RESULT_NONE; // 其他事件不处理
            //break;
    }
}

/*=========================静态辅助函数=========================*/
// 在地平线上距离中心 dist 处绘制垂直刻度
static void draw_tick(int16_t cx, int16_t cy, float cos_r, float sin_r, int16_t dist) {
    int16_t bx = cx + (int16_t)(dist * cos_r);
    int16_t by = cy + (int16_t)(dist * sin_r);
    // 垂直方向 (旋转 90°)
    int16_t tick_half = 3;
    int16_t tx0 = bx - (int16_t)(tick_half * sin_r);
    int16_t ty0 = by + (int16_t)(tick_half * cos_r);
    int16_t tx1 = bx + (int16_t)(tick_half * sin_r);
    int16_t ty1 = by - (int16_t)(tick_half * cos_r);
    OLED_DrawLine(tx0, ty0, tx1, ty1);
}

// 渲染函数，绘制图形化水平仪
static void gradienter_on_render(MenuNode_t *self) {
    OLED_Clear();

    /* ── 返回图标 ── */
    OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ReverseArea(0, 0, 16, 16);

    /* ── 角度数值显示 (左侧, Return 图标下方) ── */
    char buf[12];
    snprintf(buf, sizeof(buf), "R:%+5.1f", (double)current_attitude.roll);
    OLED_ShowString(0, 18, buf, OLED_6X8);
    snprintf(buf, sizeof(buf), "P:%+5.1f", (double)current_attitude.pitch);
    OLED_ShowString(0, 27, buf, OLED_6X8);

    /* ── 水平仪主体 (右移, 缩框) ── */
    int16_t cx = 78, cy = 34;          // 仪器中心
    float roll_rad = current_attitude.roll / 57.2958f;  // 度 → 弧度

    // 外框 (x=44, 宽68, 高40)
    OLED_DrawRectangle(44, 14, 68, 40, 0);

    // 固定参考：竖线
    OLED_DrawLine(cx, 18, cx, 50);
    // 固定参考：机翼 (短横线)
    OLED_DrawLine(cx - 14, cy, cx - 6, cy);
    OLED_DrawLine(cx + 6, cy, cx + 14, cy);
    // 中心点
    OLED_DrawPoint(cx, cy);

    // 俯仰偏移 (clamp 防止出框)
    float pitch_offset = current_attitude.pitch * 0.35f;
    if (pitch_offset > 18.0f) pitch_offset = 18.0f;
    if (pitch_offset < -18.0f) pitch_offset = -18.0f;
    int16_t horizon_cy = cy + (int16_t)pitch_offset;

    // 地平线端点计算
    float cos_r = cosf(roll_rad);
    float sin_r = sinf(roll_rad);
    int16_t half_len = 30;
    int16_t x0 = cx - (int16_t)(half_len * cos_r);
    int16_t y0 = horizon_cy - (int16_t)(half_len * sin_r);
    int16_t x1 = cx + (int16_t)(half_len * cos_r);
    int16_t y1 = horizon_cy + (int16_t)(half_len * sin_r);
    OLED_DrawLine(x0, y0, x1, y1);

    // 地平线刻度标记 (按比例缩小)
    draw_tick(cx, horizon_cy, cos_r, sin_r, 13);
    draw_tick(cx, horizon_cy, cos_r, sin_r, -13);
    draw_tick(cx, horizon_cy, cos_r, sin_r, 22);
    draw_tick(cx, horizon_cy, cos_r, sin_r, -22);
    draw_tick(cx, horizon_cy, cos_r, sin_r, 30);
    draw_tick(cx, horizon_cy, cos_r, sin_r, -30);

    OLED_Update();
}