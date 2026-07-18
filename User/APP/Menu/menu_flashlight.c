#include "menu_engine.h"
#include "flashlight_service.h"
#include "oled_driver.h"
#include "menu_common.h"
#include <stdio.h>
#include "math.h"
#include "string.h"
#include "log.h"

/* ── 外部返回图标 ── */
extern const uint8_t Return[];

/* ── 菜单选项索引（复用原有 3 选逻辑，但映射到仪表盘各元素上） ── */
#define OPT_COUNT   3
#define OPT_BACK    0   /* 左上角返回图标 */
#define OPT_TOGGLE  1   /* 圆环中心 ON/OFF 开关 */
#define OPT_ADJUST  2   /* 圆环中心 亮度的百分比数值 */

/* ── 亮度调节步进 ── */
#define ADJ_STEP_SHORT   50
#define ADJ_STEP_LONG   100

/* ── 仪表盘圆心与半径 ── */
#define DIAL_X      64   /* 水平居中 */
#define DIAL_Y      32   /* 垂直居中 */
#define DIAL_R      28   /* 稍微放大 */

typedef enum {
    FL_STATE_IDLE   = 0,
    FL_STATE_ADJUST,
} FlState_t;

typedef struct {
    FlState_t state;
} FlData_t;

static FlData_t fl_data = { .state = FL_STATE_IDLE };

static void         fl_on_enter (MenuNode_t *self);
static void         fl_on_exit  (MenuNode_t *self);
static void         fl_on_render(MenuNode_t *self);
static MenuResult_t fl_on_input (MenuNode_t *self, MenuEvent_t event);

MenuNode_t menu_flashlight = {
    .title       = "Flashlight",
    .parent      = &menu_applist,
    .children    = NULL,
    .child_count = OPT_COUNT,
    .cursor      = OPT_BACK,
    .on_enter    = fl_on_enter,
    .on_exit     = fl_on_exit,
    .on_render   = fl_on_render,
    .on_input    = fl_on_input,
    .user_data   = &fl_data,
};

/* ── 实线弧（填充弧用，每步 2°）── */
static void draw_arc_solid(int16_t cx, int16_t cy, uint8_t r,
                            int16_t start_deg, int16_t end_deg)
{
    for (int16_t deg = start_deg; deg <= end_deg; deg += 2) {
        float   rad = (float)deg * (3.14159265f / 180.0f);
        int16_t x   = cx + (int16_t)((float)r * cosf(rad) + 0.5f);
        int16_t y   = cy + (int16_t)((float)r * sinf(rad) + 0.5f);
        if (x >= 0 && x < 128 && y >= 0 && y < 64)
            OLED_DrawPoint(x, y);
    }
}

/* ── 虚线弧（背景弧用，每隔一步跳过，视觉上变虚）── */
static void draw_arc_dashed(int16_t cx, int16_t cy, uint8_t r,
                             int16_t start_deg, int16_t end_deg)
{
    uint8_t draw = 1;
    for (int16_t deg = start_deg; deg <= end_deg; deg += 2) {
        if (draw) {
            float   rad = (float)deg * (3.14159265f / 180.0f);
            int16_t x   = cx + (int16_t)((float)r * cosf(rad) + 0.5f);
            int16_t y   = cy + (int16_t)((float)r * sinf(rad) + 0.5f);
            if (x >= 0 && x < 128 && y >= 0 && y < 64)
                OLED_DrawPoint(x, y);
        }
        draw ^= 1;   /* 交替绘制/跳过 */
    }
}

/* ══════════════════════════════════════════════════════
   on_enter / on_exit
══════════════════════════════════════════════════════ */
static void fl_on_enter(MenuNode_t *self)
{
    FlData_t *s   = (FlData_t *)self->user_data;
    s->state      = FL_STATE_IDLE;
    self->cursor  = OPT_TOGGLE; // 默认聚焦在 ON/OFF 开关上，更符合调光习惯
    menu_engine_set_animating(0);
}

static void fl_on_exit(MenuNode_t *self)
{
    (void)self;
    //flashlight_off();   /* 离开手电页面自动关闭硬件输出 */
}

/* ══════════════════════════════════════════════════════
   on_input
══════════════════════════════════════════════════════ */
static MenuResult_t fl_on_input(MenuNode_t *self, MenuEvent_t event)
{
    FlData_t *s = (FlData_t *)self->user_data;

    /* ── 亮度调节模式（在此模式下，上下按键直接增减亮度） ── */
   if (s->state == FL_STATE_ADJUST) {
        switch (event) {
            case MENU_EVENT_UP_SHORT:
                flashlight_adjust(-ADJ_STEP_SHORT);   
                LOG_D(TAG_FLSH,"Flashlight brightness decreased by %d, current CCR: %d", ADJ_STEP_SHORT, flashlight_get_ccr());
                break;
            case MENU_EVENT_DOWN_SHORT:
                flashlight_adjust(+ADJ_STEP_SHORT);
                LOG_D(TAG_FLSH,"Flashlight brightness increased by %d, current CCR: %d", ADJ_STEP_SHORT, flashlight_get_ccr());
                break;
            case MENU_EVENT_UP_LONG:
                flashlight_adjust(-ADJ_STEP_LONG);
                LOG_D(TAG_FLSH,"Flashlight brightness decreased by %d, current CCR: %d", ADJ_STEP_LONG, flashlight_get_ccr());
                break;
            case MENU_EVENT_DOWN_LONG:
                flashlight_adjust(+ADJ_STEP_LONG);
                LOG_D(TAG_FLSH,"Flashlight brightness increased by %d, current CCR: %d", ADJ_STEP_LONG, flashlight_get_ccr());
                break;
            case MENU_EVENT_CONFIRM_SHORT:
            case MENU_EVENT_CONFIRM_LONG:
                s->state = FL_STATE_IDLE;
                menu_engine_set_animating(0);   /* 退出调节 → 200ms 慢速刷新 */
                break;
            default: break;
        }
        return MENU_RESULT_NONE;
    }
    /* ── 普通选择菜单模式 ── */
    switch (event) {
        case MENU_EVENT_UP_SHORT:
            self->cursor = (self->cursor + OPT_COUNT - 1) % OPT_COUNT;
            break;
        case MENU_EVENT_DOWN_SHORT:
            self->cursor = (self->cursor + 1) % OPT_COUNT;
            break;
        case MENU_EVENT_CONFIRM_SHORT:
            switch (self->cursor) {
                case OPT_BACK:
                    LOG_D(TAG_FLSH,"Menu:%s->%s", self->title, self->parent->title);
                    return MENU_RESULT_BACK;
                case OPT_TOGGLE:
                    if (flashlight_is_on()) {
                        flashlight_off();
                        LOG_D(TAG_FLSH,"Flashlight turned OFF");
                    } else {
                        flashlight_on();
                        LOG_D(TAG_FLSH,"Flashlight turned ON");
                    }
                    break;
                case OPT_ADJUST:
                    s->state = FL_STATE_ADJUST; // 进入亮度调节
                    menu_engine_set_animating(1);   /* 进入调节 → 10ms 高速刷新 */
                    break;
            }
            break;
        case MENU_EVENT_CONFIRM_LONG:
            return MENU_RESULT_BACK;
        default: break;
    }
    return MENU_RESULT_NONE;
}

/* ══════════════════════════════════════════════════════
   on_render
══════════════════════════════════════════════════════ */
static void fl_on_render(MenuNode_t *self)
{
    /* 获取用户数据，用于判断当前是否处于 FL_STATE_ADJUST 调节状态 */
    FlData_t *s = (FlData_t *)self->user_data;

    flashlight_tick();

    uint16_t ccr = flashlight_get_ccr();
    OLED_Clear();

    /* ── 1. 左上角返回图标 ── */
    OLED_ShowImage(0, 0, 16, 16, Return);

    /* ── 2. 背景弧：虚线，135°→405° ── */
    draw_arc_dashed(DIAL_X, DIAL_Y, DIAL_R, 135, 405);

    /* ── 3. 填充弧：实线三层加粗，跟随 ccr ── */
    if (ccr > 0) {
        int16_t arc_end = 135 + (int16_t)((uint32_t)ccr * 270 / 1000);
        draw_arc_solid(DIAL_X, DIAL_Y, DIAL_R,     135, arc_end);
        draw_arc_solid(DIAL_X, DIAL_Y, DIAL_R - 1, 135, arc_end);
        draw_arc_solid(DIAL_X, DIAL_Y, DIAL_R - 2, 135, arc_end);
    }

    /* ── 4. 圆内中心：精致小圆点 ── */
    uint8_t center_r = 3; // 缩小半径到 3，犹如机械手表的中心轴，非常精致

    if (s->state == FL_STATE_ADJUST) {
        // 【状态3：正在扭动调节】：纯白实心圆点，视觉重量增加
        OLED_DrawCircle(DIAL_X, DIAL_Y, center_r, OLED_FILLED);
    } 
    else if (self->cursor == OPT_ADJUST) {
        // 【状态2：光标悬停选中】：依然是空心圆！
        // 我们在外面加一圈大两号的细空心圆，形成一个“同心准星”的效果。
        // 这完美替代了丑陋的正方形反色，且“选中感”极其明显！
        OLED_DrawCircle(DIAL_X, DIAL_Y, center_r, OLED_UNFILLED);
        OLED_DrawCircle(DIAL_X, DIAL_Y, center_r + 3, OLED_UNFILLED); 
    } 
    else {
        // 【状态1：未选中】：极简的单层细线空心小圆
        OLED_DrawCircle(DIAL_X, DIAL_Y, center_r, OLED_UNFILLED);
    }

    /* ── 5. 底部居中缺口处：ON/OFF 状态 ── */
    char status_str[4];
    strcpy(status_str, flashlight_is_on() ? "ON" : "OFF");
    uint8_t status_x = flashlight_is_on() ? (DIAL_X - 6) : (DIAL_X - 9);
    OLED_ShowString(status_x, 54, status_str, OLED_6X8);

    /* ── 6. 处理反色光标反馈 (去除了丑陋的中心方形反色) ── */
    if (s->state != FL_STATE_ADJUST) {
        if (self->cursor == OPT_BACK) {
            // 反色左上角返回
            OLED_ReverseArea(0, 0, 16, 16);
        } else if (self->cursor == OPT_TOGGLE) {
            // 反色底部的 ON/OFF 文字
            OLED_ReverseArea(status_x - 2, 52, (flashlight_is_on() ? 12 : 18) + 4, 12);
        }
        // 注意：OPT_ADJUST 的光标反馈已经在第 4 步用同心圆解决了，这里直接略过！
    }
    OLED_Update();
}
