#include "menu_engine.h"
#include "oled_driver.h"
#include <stdio.h>

#define ICON_INTERVAL 55 //图标间距，单位像素
#define ANIME_STEP_MEDIUM 12    //动画速度中，单位像素
#define ANIME_STEP_SLOW 8       //动画速度慢，单位像素
#define ANIME_STEP_FAST 16      //动画速度快，单位像素
#define ANIME_STEP_DEFAULT ANIME_STEP_MEDIUM //默认动画速度

static uint8_t anime_step = ANIME_STEP_DEFAULT; //当前动画速度

typedef enum{
    ANIME_STATE_IDLE = 0,    //静止状态，无动画
    ANIME_STATE_ANIMATING,   //单次动画
    ANIME_STATE_AUTO_SCROLL, //自动滚动状态，
}AppState_t;

//动画状态结构体定义
typedef struct{
    int16_t anime_offset;
    int8_t   direction; // -1:向左滑动 1:向右滑动
    uint8_t last_cursor; //上一个光标位置，用于动画过渡
    AppState_t anime_active; //动画状态
}ApplistState_t;

//动画状态结构体，存储在 user_data 中
static ApplistState_t anime_state = {
    .anime_offset = 0,
    .direction = 0,
    .last_cursor = 0,
    .anime_active = ANIME_STATE_IDLE,
};

//虚构返回节点，用于导航至主页
MenuNode_t menu_return;
//用extern声明其他节点，避免循环引用问题
extern MenuNode_t menu_home;
/*extern*/ MenuNode_t menu_alarm;
/*extern*/ MenuNode_t menu_stopwatch;
/*extern*/ MenuNode_t menu_flashlight;
/*extern*/ MenuNode_t menu_gradienter;
/*extern*/ MenuNode_t menu_game;
/*extern*/ MenuNode_t menu_settings;

//图标数据声明
extern const uint8_t Menu_Graph[][128];
extern const uint8_t Frame[];

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
static MenuResult_t applist_on_input(MenuNode_t *self, MenuEvent_t event);
static void applist_on_enter(MenuNode_t *self);
static void applist_on_render(MenuNode_t *self);

//节点初始化
MenuNode_t menu_applist = {
    .title = "App_List",
    .parent = &menu_home,
    .children = applist_children,
    .child_count = 7,
    .cursor = 0,

    .on_enter = applist_on_enter,
    .on_exit = NULL,
    .on_render = applist_on_render,
    .on_input = applist_on_input,

    .user_data = &anime_state,
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

//输入处理函数
static MenuResult_t applist_on_input(MenuNode_t *self, MenuEvent_t event)
{
    ApplistState_t *state = (ApplistState_t *)self->user_data;

    switch(event){
        case MENU_EVENT_UP_LONG:
            anime_step = ANIME_STEP_FAST; //长按加速动画
        case MENU_EVENT_UP_SHORT:
            if(state->anime_active) break; //动画进行中，忽略输入
            state->last_cursor = self->cursor; //记录当前光标位置，用于动画过渡
            self->cursor = (self->cursor + self->child_count - 1) % self->child_count;
            state->anime_active = 1; //激活动画
            state->anime_offset = 0; //重置动画偏移
            state->direction = -1; //设置动画方向为向左滑动

            menu_engine_set_animating(1); //设置全局动画状态

            return MENU_RESULT_NONE; //不导航，按键事件以消化
        
        case MENU_EVENT_DOWN_LONG:
            anime_step = ANIME_STEP_FAST; //长按加速动画
        case MENU_EVENT_DOWN_SHORT:
            if(state->anime_active) break; //动画进行中，忽略输入
            state->last_cursor = self->cursor; //记录当前光标位置，用于动画过渡
            self->cursor = (self->cursor + 1) % self->child_count;
            state->anime_active = 1; //激活动画
            state->anime_offset = 0; //重置动画偏移
            state->direction = 1; //设置动画方向为向右滑动

            menu_engine_set_animating(1); //设置全局动画状态

            return MENU_RESULT_NONE; //不导航，按键事件以消化

        case MENU_EVENT_CONFIRM_SHORT:
            if(self->cursor >= 1 && self->cursor <= self->child_count){
                return MENU_RESULT_ENTER; //进入选中应用
            }else if(self->cursor == 0){
                return MENU_RESULT_BACK; //返回主页
            }
            break;
    }
    return MENU_RESULT_NONE;
}

// 进入动画和渲染函数可以在这里实现，利用 anime_state 来控制动画效果
static void applist_on_enter(MenuNode_t *self)
{
    self->cursor = 0; //进入时重置光标位置
}

// 渲染函数根据动画状态渲染不同的帧，实现图标滑动过渡效果
static void applist_on_render(MenuNode_t *self)
{
    ApplistState_t *state = (ApplistState_t *)self->user_data;

    int16_t center_x = 48; //图标中心的基准x坐标

    //若动画激活，根据 anime_offset 计算图标位置，实现滑动效果
    if(state->anime_active){
        state->anime_offset += anime_step; //增加动画偏移
        if(state->anime_offset >= ICON_INTERVAL){ //动画完成
            state->anime_offset = ICON_INTERVAL;//确保偏移不超过图标间距
            state->anime_active = 0; //动画结束

            menu_engine_set_animating(0); //结束动画状态
            anime_step = ANIME_STEP_DEFAULT; //重置动画速度为默认值
        }
    }

    OLED_Clear();

    if(state->anime_active){
        /* 动画过渡滑动帧渲染*/
        //根据动画方向计算像素偏移
        OLED_ShowImage(42, 10, 44, 44, Frame);

        int16_t move_px = state->anime_offset * state->direction; 

        // 计算旧图标位置
        int16_t old_x = center_x - move_px;
        OLED_ShowImage(old_x, 16, 32, 32, app_icons[state->last_cursor]); 

        // 计算新图标位置
        int16_t new_start_x = center_x + (ICON_INTERVAL * state->direction);
        int16_t new_x = new_start_x - move_px;
        OLED_ShowImage(new_x, 16, 32, 32, app_icons[self->cursor]);
        
    }else{
        /* 静止帧渲染 */
        // 渲染当前选中图标，位置固定在中心
        OLED_ShowImage(42, 10, 44, 44, Frame);
        OLED_ShowImage(center_x, 16, 32, 32, app_icons[self->cursor]);
        
        // 渲染左右两侧的图标预览，位置固定
        uint8_t left_cursor = (self->cursor + self->child_count - 1) % self->child_count;
        uint8_t right_cursor = (self->cursor + 1) % self->child_count;

        OLED_ShowImage(center_x - ICON_INTERVAL, 16, 32, 32, app_icons[left_cursor]);
        OLED_ShowImage(center_x + ICON_INTERVAL, 16, 32, 32, app_icons[right_cursor]);
    }

    OLED_Update();
}

