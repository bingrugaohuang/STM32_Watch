#include "menu_engine.h"
#include "log.h"
#include "rtc_service.h"
#include "oled_driver.h"
#include "oled_common.h"
#include "osal.h"
#include "common_macro.h"
#include <stdio.h> /* 用于 sprintf */

//用extern声明其他节点，避免循环引用问题
extern MenuNode_t menu_applist;
//extern MenuNode_t menu_shutdown;

//子节点数组
MenuNode_t *home_children[] = {&menu_applist, 
    //&menu_shutdown
};

#define ANIME_SPEED 4 //动画步进值，越大动画越快，越小动画越慢

/* 静态函数声明*/
static void home_on_enter(MenuNode_t *self);
static void home_on_exit(MenuNode_t *self);
static void home_on_render(MenuNode_t *self);
static MenuResult_t home_on_input(MenuNode_t *self, MenuEvent_t event);
static const char *weekday_short(uint8_t w);

//节点初始化
MenuNode_t menu_home = {
    .title = "Home",
    .parent = NULL,
    .children = home_children,
    .child_count = 2,
    .cursor = 0,

    .on_enter = home_on_enter,
    .on_exit = home_on_exit,
    .on_render = home_on_render,
    .on_input = home_on_input,
};

//也许可以加些简单的进入动画效果
static void home_on_enter(MenuNode_t *self)
{
    (void)self;
    self->cursor = 0;
    int i;
    LOG_I("MENU","Entering Home menu");
    OLED_Clear();
    OLED_Update();
    
    OLED_DrawLine(16, 44, 112, 44); 
    // 1. 横线展开动画 (步进值设置大一点，比如4，动画更清脆)
    for (i = 0; i <= 48; i += ANIME_SPEED) 
    {
        osal_task_delay(15); // 约 15ms 一帧，控制展开速度;延时放前面使进入渲染时动画更平滑
        //OLED_DrawLine(64 - i, 44, 64 + i, 44); 
        OLED_UpdateArea(64 - i, 44, i * 2 + 1, 1);
        //OLED_UpdateArea(16, 44, 97, 1); // 只更新横线所在区域，减少刷新时间
        //OLED_Update(); // 全屏刷新，确保动画流畅（因为OLED_UpdateArea在某些情况下可能会有残影）
    }
    menu_engine_set_animating(0); //动画结束

    // 2. 动画结束，确保线画满，但会引入渲染前的延迟
    // OLED_DrawLine(16, 44, 112, 44);
    // OLED_Update();
    
    // 3. 接下来系统会自动调用 home_on_render 渲染文字，无需在此处写文字逻辑
}

//也许可以做些简单的退出动画效果
static void home_on_exit(MenuNode_t *self)
{
    (void)self;
    int i;
    LOG_I("MENU","Exiting Home menu");
    // 1. 抹除文字，只保留中间那条横线
    OLED_ClearArea(16, 0, 97, 40); // 清除主页内容区域，保留底部横线
    // OLED_DrawLine(16, 44, 112, 44);
    OLED_UpdateArea(16, 0, 97, 40);
    osal_task_delay(50); // 稍微停顿一下，增加呼吸感
    
    // 2. 横线收缩动画
    for (i = 48; i >= 0; i -= ANIME_SPEED) 
    {
        OLED_ClearArea(16, 44, 97, 1); // 清除横线所在区域，准备重绘
        OLED_DrawLine(64 - i, 44, 64 + i, 44);
        OLED_UpdateArea(16, 44, 97, 1); // 只更新横线所在区域，减少刷新时间
        osal_task_delay(15);
    }
    menu_engine_set_animating(0); //动画结束
    
    // 3. 收缩成一个点后，彻底清屏
    // OLED_Clear();
    // OLED_Update();
}

//主页的渲染函数，负责绘制主页界面
static void home_on_render(MenuNode_t *self)
{
    OLED_ClearArea(16, 0, 97, 40); // 清除主页内容区域，保留底部横线
    /* 获取当前时间 */
    RTCTime_t time = rtc_service_get_time_struct();
    RTCDate_t date = rtc_service_get_date_struct();

    /* ── 日期 (6x8, 居中) ── */
    char date_str[10];
    snprintf(date_str, sizeof(date_str), "%02d/%02d %s",
             date.month, date.date, weekday_short(date.week_day));
    /* weekday_short: 1="Mon" 2="Tue" 3="Wed" 4="Thu" 5="Fri" 6="Sat" 7="Sun" */
    OLED_ShowString(22, 2, date_str, OLED_6X8);

    /* ── 时分 (12x24) ── */
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d",
             time.hours, time.minutes);
    OLED_ShowString(22, 16, time_str, OLED_12X24);

    /* ── 秒数 (8x16, 底部对齐 12x24) ── */
    char sec_str[4];
    snprintf(sec_str, sizeof(sec_str), ":%02d", time.seconds);
    OLED_ShowString(82, 24, sec_str, OLED_8X16);

    /* ── 分隔线 ── */
    OLED_DrawLine(16, 44, 112, 44);

    /* 传输延时优化 */
    OLED_UpdateArea(16, 0, 97, 40); // 只更新主页内容区域，减少刷新时间

    /* 下方留白 - 简约大气 */
    
}

//输入处理函数
static MenuResult_t home_on_input(MenuNode_t *self, MenuEvent_t event)
{
    switch (event){
    case MENU_EVENT_CONFIRM_SHORT:
        self->cursor = 0;
        menu_engine_set_animating(1); //设置动画状态
        return MENU_RESULT_ENTER;
    case MENU_EVENT_CONFIRM_LONG:
        self->cursor = 1;
        menu_engine_set_animating(1); //设置动画状态
        return MENU_RESULT_ENTER;
    default:
        break;
    }
    return MENU_RESULT_NONE;
}

//工具函数：将星期数字转换为短字符串
static const char *weekday_short(uint8_t w) {
    static const char *tbl[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    return (w >= 1 && w <= 7) ? tbl[w - 1] : "???";
}