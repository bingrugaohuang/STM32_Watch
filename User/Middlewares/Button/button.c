#include "button.h"
#include <string.h> /* 用于 memset */

/* ---------- 可配置参数（宏定义） ---------- */
#define DEBOUNCE_TICKS   3          /**< 消抖次数（例如 3 次 = 15~30ms） */
#define LONG_PRESS_TICKS 100        /**< 长按判定阈值（例如 100 次 = 1 秒@10ms 周期） */
#define LONG_PRESS_REPEAT_TICKS 20  /**< 长按持续触发间隔（例如 20 次 = 200ms） */
#define ButtonASSERT( x )    do { if (!(x)) { /* 可选：添加错误处理或日志输出 */Error_Handler();/* return;*/ } } while (0)

/* 
 * 函数：button_init
 * 功能：初始化按键控制块，绑定 GPIO 读取函数与有效电平
 *       - 将所有成员置零
 *       - 设置 active_level
 *       - 保存 read_pin 函数指针
 **/
void button_init(Button *btn,
                 uint8_t (*read_pin)(void),
                 uint8_t active_level)
{
    ButtonASSERT(btn != NULL);
    ButtonASSERT(read_pin != NULL);

    memset(btn, 0, sizeof(Button));
    btn->active_level = active_level;
    btn->filtered_level = !active_level;     /* 初始化时假设按键处于未按下状态 */
    btn->readpin = read_pin;
}

/* 
 * 函数：button_attach_short
 * 功能：注册短按回调函数及对应的用户数据
 **/
void button_attach_short(Button *btn,
                         ButtonCallback cb,
                         void *user_data)
{
    ButtonASSERT(btn != NULL);
    // ButtonASSERT(cb != NULL);           // 短按回调可选，允许用户传入 NULL 以不使用短按事件

    btn->short_cb = cb;
    btn->short_user_data = user_data;
}

/* 
 * 函数：button_attach_long
 * 功能：注册长按回调函数及对应的用户数据
 **/
void button_attach_long(Button *btn,
                        ButtonCallback cb,
                        void *user_data)
{
    ButtonASSERT(btn != NULL);
    // ButtonASSERT(cb != NULL);           // 长按回调可选，允许用户传入 NULL 以不使用长按事件

    btn->long_cb = cb;
    btn->long_user_data = user_data;
}

/* 
 * 函数：button_ticks
 * 功能：按键状态机核心处理，需周期性调用（例如每 10ms）
 *       主要完成：消抖、状态跳转、事件回调
 **/
void button_ticks(Button *btn)
{
    ButtonASSERT(btn != NULL);

    uint8_t current_level = btn->readpin();
    if(current_level == btn->filtered_level){
        btn->debounce_cnt = 0;                      // 电平无变化，清零消抖计数
    } else {
        btn->debounce_cnt++;                        // 电平有变化，增加消抖计数
        if(btn->debounce_cnt >= DEBOUNCE_TICKS){
            btn->filtered_level = current_level;    // 消抖完成，更新稳定电平
            btn->debounce_cnt = 0;                  // 清零消抖计数
        }else{
            return;                                 // 消抖未完成，暂不处理状态机
        }
    }

    switch(btn->state){                                     //电平变化驱动状态机
        case BTN_STATE_IDLE:               
            if(btn->filtered_level == btn->active_level){
                btn->state = BTN_STATE_PRESS;               // 进入按下状态
                btn->press_ticks = 0;                       // 清零按下计数
            }
            break;
        case BTN_STATE_PRESS:
            if(btn->filtered_level != btn->active_level){
                if(btn->short_cb != NULL){
                    btn->short_cb(BTN_EVENT_SHORT_PRESS, btn->short_user_data);    // 触发短按回调
                }
                btn->state = BTN_STATE_IDLE;                // 返回空闲状态
            }else{
                btn->press_ticks++;
                if(btn->press_ticks >= LONG_PRESS_TICKS){
                    if(btn->long_cb != NULL){
                        btn->long_cb(BTN_EVENT_LONG_PRESS, btn->long_user_data);  // 触发长按回调
                    }
                    btn->press_ticks = 0;                   // 清零长按计数
                    btn->state = BTN_STATE_LONG_PRESS;      // 进入长按状态
                }
            }
            break;
        case BTN_STATE_LONG_PRESS:
            if(btn->filtered_level != btn->active_level){
                btn->state = BTN_STATE_IDLE;                // 长按结束，返回空闲状态
            }else{
                btn->press_ticks++;                         // 长按持续计数
                if(btn->press_ticks >= LONG_PRESS_REPEAT_TICKS){
                    if(btn->long_cb != NULL){
                        btn->long_cb(BTN_EVENT_LONG_PRESS_REPEAT, btn->long_user_data);  // 触发长按持续回调
                    }
                    btn->press_ticks = 0;                   // 清零长按计数
                }
            }
            break;
        default:
            btn->state = BTN_STATE_IDLE;                    // 异常状态，重置为初始状态
            break;  
    }
}