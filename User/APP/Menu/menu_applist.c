#include "menu_engine.h"
#include "oled_driver.h"
#include <stdio.h>
#include "menu_common.h"

//  cover flow 常量
#define CF_INTERVAL   28   // 图标间距（比32px小 → 相邻层叠4px）
#define CF_BASE_Y     12   // 中心图标的 y 坐标
#define CF_Y_STEP      5   // 每远离中心一层，y 下移的像素数

/* ── 滑动动画速度 ── */
#define ANIME_STEP_SLOW    3
#define ANIME_STEP_MEDIUM  5
#define ANIME_STEP_FAST   10
#define ANIME_STEP_DEFAULT ANIME_STEP_MEDIUM

/* ── 三角形指示器 ── */
#define TRI_H            5    // 三角形高度（像素）
#define TRI_GAP          6    // 三角形离图标边缘的距离
#define TRI_CX_OFFSET   16    // 三角形水平中心相对 CENTER_X 的偏移

/* ── 退出确认动画（on_exit 同步播放）── */
#define EXIT_FRAMES        6    // 总帧数
#define EXIT_HALF          3    // 收缩帧数，之后回弹
#define EXIT_MAX_INSET     6    // 最大内缩像素

/* ── 入场动画 ── */
#define ENTRY_OFFSET_INIT  160  // 初始右偏移（图标从此处滑入屏幕）
#define ENTRY_STEP          16  // 每帧移动像素数


static uint8_t anime_step = ANIME_STEP_DEFAULT; //当前动画速度

typedef enum{
    ANIME_STATE_IDLE = 0,    //静止状态，无动画
    ANIME_STATE_ANIMATING,   //单次动画
    ANIME_STATE_AUTO_SCROLL, //自动滚动状态，
}AppState_t;

//动画状态结构体定义
typedef struct{
    int16_t    anime_offset;
    int8_t     direction; // -1:向左滑动 1:向右滑动
    uint8_t    last_cursor; //上一个光标位置，用于动画过渡
    AppState_t anime_active; //动画状态
    int16_t    entry_offset;   // 入场动画：整体右偏量，从 ENTRY_OFFSET_INIT 减到 0
    uint8_t    entry_active;   // 入场动画进行中
}ApplistState_t;

//动画状态结构体，存储在 user_data 中
static ApplistState_t anime_state = {0};

//虚构返回节点，用于导航至主页
MenuNode_t menu_return;
//用extern声明其他节点，避免循环引用问题
//extern MenuNode_t menu_home;
/*extern*/ MenuNode_t menu_alarm;
// /*extern*/ MenuNode_t menu_stopwatch;
///*extern*/ MenuNode_t menu_flashlight;
/*extern*/ MenuNode_t menu_gradienter;
/*extern*/ MenuNode_t menu_game;
/*extern*/ MenuNode_t menu_settings;

//图标数据声明
extern const uint8_t Menu_Graph[][128];

//子节点数组
MenuNode_t *applist_children[] = {
    &menu_return, //第0项为返回主页的虚构节点
    &menu_stopwatch,
    &menu_flashlight,
    &menu_game,
    &menu_gradienter,
    &menu_alarm,
    &menu_settings,
};

/* 静态函数声明*/
static void         applist_on_enter (MenuNode_t *self);
static void         applist_on_exit  (MenuNode_t *self);
static void         applist_on_render(MenuNode_t *self);
static MenuResult_t applist_on_input (MenuNode_t *self, MenuEvent_t event);

//节点初始化
MenuNode_t menu_applist = {
    .title       = "App_List",
    .parent      = &menu_home,
    .children    = applist_children,
    .child_count = 7,
    .cursor      = 0,

    .on_enter    = applist_on_enter,
    .on_exit     = applist_on_exit,
    .on_render   = applist_on_render,
    .on_input    = applist_on_input,

    .user_data   = &anime_state,
};

const uint8_t *app_icons[] = {
    Menu_Graph[0], //返回图标
    Menu_Graph[1], //秒表图标
    Menu_Graph[2], //手电筒图标
    Menu_Graph[3], //游戏图标
    Menu_Graph[4], //水平仪图标
    Menu_Graph[5], //闹钟图标
    Menu_Graph[6], //设置图标
};

/* ══════════════════════════════════════════════════════
   三角形绘制
══════════════════════════════════════════════════════ */
static void draw_tri_down_fill(int16_t cx, int16_t top_y, uint8_t h)
{
    for (uint8_t i = 0; i < h; i++)
        OLED_DrawLine(cx - (h-1-i), top_y + i, cx + (h-1-i), top_y + i);
}
static void draw_tri_up_fill(int16_t cx, int16_t bot_y, uint8_t h)
{
    for (uint8_t i = 0; i < h; i++)
        OLED_DrawLine(cx - (h-1-i), bot_y - i, cx + (h-1-i), bot_y - i);
}
static void draw_tri_down_hollow(int16_t cx, int16_t top_y, uint8_t h)
{
    OLED_DrawLine(cx - (h-1), top_y,         cx + (h-1), top_y        );
    OLED_DrawLine(cx - (h-1), top_y,         cx,         top_y + (h-1));
    OLED_DrawLine(cx + (h-1), top_y,         cx,         top_y + (h-1));
}
static void draw_tri_up_hollow(int16_t cx, int16_t bot_y, uint8_t h)
{
    OLED_DrawLine(cx - (h-1), bot_y,         cx + (h-1), bot_y        );
    OLED_DrawLine(cx - (h-1), bot_y,         cx,         bot_y - (h-1));
    OLED_DrawLine(cx + (h-1), bot_y,         cx,         bot_y - (h-1));
}
/* ══════════════════════════════════════════════════════
   start_animation
══════════════════════════════════════════════════════ */
static void start_animation(MenuNode_t *self, int8_t dir)
{
    ApplistState_t *s = (ApplistState_t *)self->user_data;
    s->last_cursor  = self->cursor;
    self->cursor    = (self->cursor + self->child_count + dir) % self->child_count;
    s->anime_offset = 0;
    s->direction    = dir;
    menu_engine_set_animating(1);
}


/* ══════════════════════════════════════════════════════
   applist_draw：核心渲染（on_render 和 on_exit 共用）

   ref  : 以哪个图标索引为中心基准
   slide: 所有图标整体 x 偏移（入场动画用，正值=整体右偏）
   inset: 三角形内缩量
          0        → 空心三角（正常状态）
          1..MAX   → 实心三角+收缩（确认动画）
          负数     → 不画三角（入场动画期间）
══════════════════════════════════════════════════════ */
static void applist_draw(MenuNode_t *self, uint8_t ref, int16_t slide, int16_t inset)
{
    const int16_t CENTER_X = 48;
    const uint8_t n = self->child_count;

    typedef struct { uint8_t idx; int16_t x; } Entry;
    Entry   visible[7];
    uint8_t cnt = 0;

    /* 收集屏幕范围内的图标 */
    for (uint8_t i = 0; i < n; i++) {
        int16_t delta = (int16_t)i - (int16_t)ref;
        if (delta >  (int16_t)(n / 2)) delta -= (int16_t)n;
        if (delta < -(int16_t)(n / 2)) delta += (int16_t)n;

        int16_t x = CENTER_X + delta * (int16_t)CF_INTERVAL + slide;
        if (x > -32 && x < 128) {
            visible[cnt].idx = i;
            visible[cnt].x   = x;
            cnt++;
        }
    }

    /* 按距中心从远到近排序（远的先画 = 在最下层）*/
    for (uint8_t a = 0; a + 1 < cnt; a++) {
        for (uint8_t b = 0; b + 1 < cnt - a; b++) {
            if (abs(visible[b].x - CENTER_X) < abs(visible[b+1].x - CENTER_X)) {
                Entry tmp    = visible[b];
                visible[b]   = visible[b+1];
                visible[b+1] = tmp;
            }
        }
    }

    /* 第一层：除最近中心外的所有图标 */
    for (uint8_t k = 0; k + 1 < cnt; k++) {
        int16_t x    = visible[k].x;
        int16_t dist = (int16_t)abs(x - CENTER_X);
        int16_t y    = CF_BASE_Y + dist * CF_Y_STEP / CF_INTERVAL;
        OLED_ShowImage(x, y, 32, 32, app_icons[visible[k].idx]);
    }

    /* 第二层：三角形（inset < 0 时跳过，入场动画不显示三角）*/
    if (inset >= 0) {
        int16_t tri_cx    = CENTER_X + TRI_CX_OFFSET;
        int16_t top_tip_y = CF_BASE_Y       - TRI_GAP + inset;
        int16_t bot_tip_y = CF_BASE_Y + 32  + TRI_GAP - inset - 1;

        if (inset > 0) {
            draw_tri_down_fill  (tri_cx, top_tip_y - (TRI_H - 1), TRI_H);
            draw_tri_up_fill    (tri_cx, bot_tip_y + (TRI_H - 1), TRI_H);
        } else {
            draw_tri_down_hollow(tri_cx, top_tip_y - (TRI_H - 1), TRI_H);
            draw_tri_up_hollow  (tri_cx, bot_tip_y + (TRI_H - 1), TRI_H);
        }
    }

    /* 第三层：最近中心的图标，压在三角形之上 */
    if (cnt > 0) {
        int16_t x    = visible[cnt - 1].x;
        int16_t dist = (int16_t)abs(x - CENTER_X);
        int16_t y    = CF_BASE_Y + dist * CF_Y_STEP / CF_INTERVAL;
        OLED_ShowImage(x, y, 32, 32, app_icons[visible[cnt - 1].idx]);
    }
}

//输入处理函数
static MenuResult_t applist_on_input(MenuNode_t *self, MenuEvent_t event)
{
    ApplistState_t *s = (ApplistState_t *)self->user_data;
    
    // 入场动画期间屏蔽输入
    if (s->entry_active) return MENU_RESULT_NONE;  

    switch(s->anime_active){
        case ANIME_STATE_IDLE:
            if(event == MENU_EVENT_UP_SHORT){
                anime_step = ANIME_STEP_MEDIUM; //长按加速动画
                s->anime_active = ANIME_STATE_ANIMATING; //进入自动滚动状态
                start_animation(self, -1); //向左滑动
            }else if(event == MENU_EVENT_DOWN_SHORT){
                anime_step = ANIME_STEP_MEDIUM; //长按加速动画
                s->anime_active = ANIME_STATE_ANIMATING; //进入自动滚动状态
                start_animation(self, 1); //向右滑动
            }else if(event == MENU_EVENT_UP_LONG){
                anime_step = ANIME_STEP_FAST; //长按加速动画
                s->anime_active = ANIME_STATE_AUTO_SCROLL; //进入自动滚动状态
                start_animation(self, -1); //向左滑动
            }else if(event == MENU_EVENT_DOWN_LONG){
                anime_step = ANIME_STEP_FAST; //长按加速动画
                s->anime_active = ANIME_STATE_AUTO_SCROLL; //进入自动滚动状态
                start_animation(self, 1); //向右滑动
            }else if(event == MENU_EVENT_CONFIRM_SHORT){
                return (self->cursor == 0) ? 
                 MENU_RESULT_BACK : MENU_RESULT_ENTER;
            }
            break;

        //ANIME_STATE_ANIMATING状态下不响应输入，因为执行一次后自动进入IDLE状态
        case ANIME_STATE_AUTO_SCROLL:
           if(event == MENU_EVENT_UP_RELEASE || event == MENU_EVENT_DOWN_RELEASE){
                //松开按键，停止自动滚动，进入单次动画状态完成当前动画
                s->anime_active = ANIME_STATE_ANIMATING; 
            }
            break;
        default: 
            break;
    }
    return MENU_RESULT_NONE;
}

/* ══════════════════════════════════════════════════════
   on_enter：重置所有状态，启动入场动画

   入场效果：图标从右侧依次滑入，最终 cursor=0（返回）居中
   出现顺序（左→右）：闹钟 → 设置 → 返回 → 秒表 → 手电
══════════════════════════════════════════════════════ */
static void applist_on_enter(MenuNode_t *self)
{
    ApplistState_t *s = (ApplistState_t *)self->user_data;
    //self->cursor    = 0;
    s->anime_active = ANIME_STATE_IDLE;
    s->anime_offset = 0;
    s->direction    = 0;
    s->entry_offset = ENTRY_OFFSET_INIT;
    s->entry_active = 1;
    menu_engine_set_animating(1);
}

/* ══════════════════════════════════════════════════════
   on_exit：阻塞式确认动画（在导航前同步播完，零状态残留）

   若动画过快，在 OLED_Update() 后加 HAL_Delay(25) 即可
══════════════════════════════════════════════════════ */
static void applist_on_exit(MenuNode_t *self)
{
    for (uint8_t t = 0; t < EXIT_FRAMES; t++) {
        int16_t inset = (t < EXIT_HALF)
            ? (int16_t) t            * EXIT_MAX_INSET / EXIT_HALF
            : (int16_t)(EXIT_FRAMES - t) * EXIT_MAX_INSET / EXIT_HALF;
        OLED_Clear();
        applist_draw(self, self->cursor, 0, inset);
        OLED_Update();
    }
}

/* ══════════════════════════════════════════════════════
   on_render：推进动画状态，计算 ref/slide，调用 applist_draw
══════════════════════════════════════════════════════ */
static void applist_on_render(MenuNode_t *self)
{
    ApplistState_t *s = (ApplistState_t *)self->user_data;

     /* 推进入场动画 */
    if (s->entry_active) {
        s->entry_offset -= ENTRY_STEP;
        if (s->entry_offset <= 0) {
            s->entry_offset = 0;
            s->entry_active = 0;
            menu_engine_set_animating(0);
        }
    }

    /* 推进动画偏移 */
    if (s->anime_active) {
        s->anime_offset += anime_step;
        if (s->anime_offset >= CF_INTERVAL) {
            s->anime_offset = CF_INTERVAL;
            switch (s->anime_active) {
                case ANIME_STATE_ANIMATING:
                    s->anime_active = ANIME_STATE_IDLE;
                    anime_step = ANIME_STEP_DEFAULT;
                    menu_engine_set_animating(0);
                    break;
                case ANIME_STATE_AUTO_SCROLL:
                    start_animation(self, s->direction);
                    break;
                default: break;
            }
        }
    }

    /* 根据当前状态决定渲染参数 */
    uint8_t ref;
    int16_t slide;
    int16_t inset;

    if (s->entry_active) {
        ref   = self->cursor;          // 入场：以 cursor=0 为中心
        slide = s->entry_offset;       // 整体右偏，逐帧减小
        inset = -1;                    // 入场期间不显示三角形
    } else if (s->anime_active) {
        ref   = s->last_cursor;        // 切换动画：以旧 cursor 为基准
        slide = s->anime_offset * (int16_t)(-s->direction);
        inset = 0;                     // 切换期间显示空心三角
    } else {
        ref   = self->cursor;
        slide = 0;
        inset = 0;                     // 静止：空心三角
    }

    OLED_Clear();
    applist_draw(self, ref, slide, inset);
    OLED_Update();
}
