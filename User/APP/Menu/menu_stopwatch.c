#include "oled_driver.h" 
#include <stdio.h>
#include "menu_engine.h"
#include "osal.h"
#include "menu_common.h"
#include "log.h"

// 引入外部图标
extern const uint8_t Return[];

// 严格限制计次数量为3次，防屏幕溢出
#define MAX_LAPS 3 

// 秒表运行状态枚举
typedef enum {
    STOPWATCH_STATE_IDLE = 0,    // 复位闲置
    STOPWATCH_STATE_RUNNING,     // 正在计时
    STOPWATCH_STATE_PAUSED       // 暂停中
} StopwatchState_t;

// 秒表数据结构体
typedef struct {
    uint32_t total_ms;           // 累计时间（毫秒）
    uint32_t lap_times[MAX_LAPS];// 计次时间数组
    uint8_t lap_count;           // 当前计次数量
    StopwatchState_t state;      // 当前状态
} StopwatchData_t;

static StopwatchData_t sw_data = {
    .total_ms = 0,
    .lap_count = 0,
    .state = STOPWATCH_STATE_IDLE
};

// 软件定时器句柄
static osal_timer_handle_t sw_timer = NULL;

//软件定时器回调函数，后台精准执行，不受屏幕刷新和I2C通讯延迟影响
static void sw_timer_callback(osal_timer_handle_t xTimer)
{
    sw_data.total_ms += 100;
}

/* 静态函数声明 */
static void stopwatch_on_enter(MenuNode_t *self);
static void stopwatch_on_render(MenuNode_t *self);
static MenuResult_t stopwatch_on_input(MenuNode_t *self, MenuEvent_t event);

// 实例化秒表节点
MenuNode_t menu_stopwatch = {
    .title = "Stopwatch",
    .parent = &menu_applist,   
    .children = NULL,     
    .child_count = 3,     // 3 个选项：返回(左上角)、START/PAUSE、LAP/CLR
    .cursor = 0,

    .on_enter = stopwatch_on_enter,
    .on_exit = NULL,
    .on_render = stopwatch_on_render,
    .on_input = stopwatch_on_input,

    .user_data = &sw_data, 
};

static void stopwatch_on_enter(MenuNode_t *self)
{
    // 懒汉式初始化：第一次进入时创建软件定时器
    // OSAL 系统 Tick 为 1000Hz (1ms)，100个 Tick 就是精准的 100ms
    if (sw_timer == NULL) {
        sw_timer = osal_timer_create("sw_tmr", 100, 1, NULL, sw_timer_callback);
    }

    self->cursor = 1; // 默认光标停在 Start 按钮上更人性化
    menu_engine_set_animating(0); 
}

static MenuResult_t stopwatch_on_input(MenuNode_t *self, MenuEvent_t event)
{
    StopwatchData_t *data = (StopwatchData_t *)self->user_data;

    switch(event){
        case MENU_EVENT_UP_LONG:
        case MENU_EVENT_UP_SHORT:
            self->cursor = (self->cursor - 1 + self->child_count) % self->child_count; 
            break;
        case MENU_EVENT_DOWN_LONG:
        case MENU_EVENT_DOWN_SHORT:
            self->cursor = (self->cursor + 1) % self->child_count; 
            break;
        case MENU_EVENT_CONFIRM_SHORT:
            if(self->cursor == 0){ // 0: 左上角返回
                LOG_D(TAG_STPW,"Menu:%s->%s", self->title, self->parent->title);
                return MENU_RESULT_BACK;
            }else if(self->cursor == 1){ // 1: START/PAUSE
                if(data->state == STOPWATCH_STATE_IDLE || data->state == STOPWATCH_STATE_PAUSED){
                    data->state = STOPWATCH_STATE_RUNNING; 
                    if (sw_timer) {
                        osal_timer_start(sw_timer, 0); // 启动软件定时器
                        LOG_D(TAG_STPW,"Stopwatch started");
                    }
                }else if(data->state == STOPWATCH_STATE_RUNNING){
                    data->state = STOPWATCH_STATE_PAUSED; 
                    if (sw_timer) {
                        osal_timer_stop(sw_timer, 0);  // 暂停软件定时器
                        LOG_D(TAG_STPW,"Stopwatch paused");
                    }
                }
            }else if(self->cursor == 2){ // 2: LAP/CLR
                if(data->state == STOPWATCH_STATE_RUNNING && data->lap_count < MAX_LAPS){
                    data->lap_times[data->lap_count] = data->total_ms;
                    data->lap_count++;
                    LOG_D(TAG_STPW,"Lap time recorded");
                }else if(data->state == STOPWATCH_STATE_PAUSED){
                    data->total_ms = 0;
                    data->lap_count = 0;
                    data->state = STOPWATCH_STATE_IDLE;
                    LOG_D(TAG_STPW,"Stopwatch cleared");
                }
            }
            break;
        default:
            break;
    }
    return MENU_RESULT_NONE;
}

// 辅助函数：格式化时间
static void format_time(uint32_t ms, char *buf)
{
    uint32_t min = ms / 60000;
    uint32_t sec = (ms % 60000) / 1000;
    uint32_t hms = (ms % 1000) / 100; 
    sprintf(buf, "%02d:%02d.%01d", (int)min, (int)sec, (int)hms);
}

// 左右分栏界面渲染
static void stopwatch_on_render(MenuNode_t *self)
{
    StopwatchData_t *data = (StopwatchData_t *)self->user_data;
    char buf[24];

    OLED_Clear();

    /* ================= 1. 左半侧区域 (X: 0~63) ================= */
    
    // 1.1 左上角返回图标 (16x16)
    OLED_ShowImage(0, 0, 16, 16, Return);
    // 移除了原来的箭头指示

    // 1.2 左侧居中：主计时器 OLED_8X16 (高16px)
    // 8个字符 * 8像素 = 64像素宽，刚好占满左边！
    format_time(data->total_ms, buf);
    OLED_ShowString(0, 26, buf, OLED_8X16);

    /* ================= 2. 右半侧区域 (X: 65~127) ================= */
    
    // 2.1 画一个方框 (上，左，右，下)
    OLED_DrawLine(65, 0,   127, 0);   
    OLED_DrawLine(65, 0,   65,  48);  
    OLED_DrawLine(127,0,   127, 48); 
    OLED_DrawLine(65, 48,  127, 48);  

    // 2.2 框内显示计次列表 OLED_6X8
    for (uint8_t i = 0; i < data->lap_count; i++) {
        char lap_time[16];
        format_time(data->lap_times[i], lap_time);
        sprintf(buf, "%d:%s", (int)(i + 1), lap_time);
        
        // 框内从 X=68 开始，上下边距为 4，每行间隔 14px
        OLED_ShowString(68, 4 + i * 14, buf, OLED_6X8);
    }

    /* ================= 3. 底部按键区域 (Y: 51~63) ================= */
    
    // 3.1 底部全局分隔线
    OLED_DrawLine(0, 51, 127, 51);

    // 3.2 选项1：Start/Pause (布置在左半边偏右一点)
    char *btn1 = (data->state == STOPWATCH_STATE_RUNNING) ? "Pause" : "Start";
    OLED_ShowString(16, 54, btn1, OLED_6X8); // 直接显示文字，去掉了括号

    // 3.3 选项2：Lap / Clr / --- (布置在右半边居中)
    char *btn2 = "---";
    if (data->state == STOPWATCH_STATE_RUNNING) btn2 = "Lap";
    else if (data->state == STOPWATCH_STATE_PAUSED) btn2 = "Clr";
    OLED_ShowString(84, 54, btn2, OLED_6X8); // 直接显示文字，去掉了括号

    /* ================= 4. 处理光标反色 ================= */
    if (self->cursor == 0) {
        // 0: 反色左上角图标区域 (X:0, Y:0, W:16, H:16)
        OLED_ReverseArea(0, 0, 16, 16);
    } else if (self->cursor == 1) {
        // 1: 反色 Start/Pause 区域 (包围文字并加 2px 内边距)
        OLED_ReverseArea(14, 52, 34, 12);
    } else if (self->cursor == 2) {
        // 2: 反色 Lap/Clr/--- 区域
        OLED_ReverseArea(82, 52, 22, 12);
    }

    OLED_Update();
}