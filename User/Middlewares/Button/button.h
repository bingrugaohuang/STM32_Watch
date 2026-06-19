#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 按键事件类型 */
typedef enum {
    BTN_EVENT_NONE = 0,           /**< 无事件 */
    BTN_EVENT_SHORT_PRESS,        /**< 短按事件（按下后放开，未超过长按阈值） */
    BTN_EVENT_LONG_PRESS,         /**< 长按事件（按住超过阈值时触发） */
    BTN_EVENT_LONG_PRESS_REPEAT,  /**< 长按持续触发事件（每隔一定时间重复触发） */
    BTN_EVENT_RELEASE,            /**< 释放事件（按键放开时触发） */
}ButtonEvent;

/* 按键状态 */
typedef enum {
    BTN_STATE_IDLE = 0,         /**< 空闲状态 */
    BTN_STATE_PRESS,            /**< 按下确认状态（去抖完成后） */
    BTN_STATE_LONG_PRESS,        /**< 长按已触发状态 */
    BTN_STATE_RELEASE,          /**< 释放状态（按键已放开，等待回到空闲） */
}ButtonState;

/* 回调函数类型：参数为 user_data 指针 */
typedef void (*ButtonCallback)(ButtonEvent event, void *user_data);

/* 按键控制块 */
typedef struct {
    ButtonState     state;              /**< 当前状态 */
    uint16_t        press_ticks;        /**< 按下持续计数（每个 tick 加 1） */
    uint8_t         debounce_cnt;       /**< 消抖计数器 */
    uint8_t         filtered_level;     /**< 消抖后的稳定电平 */
    uint8_t         active_level;       /**< 按下时的有效电平（0 或 1） */
    uint8_t         (*readpin)(void);   /**< 读取 GPIO 电平的函数指针，返回 0 或 1 */

    ButtonCallback  short_cb;           /**< 短按回调函数 */
    void            *short_user_data;   /**< 短按回调的用户数据 */
    ButtonCallback  long_cb;            /**< 长按回调函数 */
    void            *long_user_data;    /**< 长按回调的用户数据 */
    ButtonCallback  release_cb;         /**< 释放回调函数 */
    void            *release_user_data; /**< 释放回调的用户数据 */
}Button;

/* ------------------------- API 函数声明 ------------------------- */

/**
  * 函    数：初始化按键结构体
  * 参    数：btn     - 按键句柄
  *           readpin - 读取按键引脚电平的函数，需由用户实现，返回 0 或 1
  *           active_level - 按键按下时的有效电平（0 = 低电平有效，1 = 高电平有效）
  * 返 回 值：无
  * 说    明：初始化按键控制块，设置状态、计数器
  */
void button_init(Button *btn,
                 uint8_t (*readpin)(void),
                 uint8_t active_level);

/**
  * 函    数：注册短按回调函数及对应的用户数据
  * 参    数：btn       - 按键句柄
  *           cb        - 回调函数指针
  *           user_data - 回调时传入的用户自定义数据
  * 返 回 值：无
  * 说    明：保存短按回调函数和用户数据到控制块
  */
void button_attach_short(Button *btn,
                         ButtonCallback cb,
                         void *user_data);

/**
  * 函    数：注册长按回调函数及对应的用户数据
  * 参    数：btn       - 按键句柄
  *           cb        - 回调函数指针
  *           user_data - 回调时传入的用户自定义数据
  * 返 回 值：无
  * 说    明：保存长按回调函数和用户数据到控制块
  */
void button_attach_long(Button *btn,
                        ButtonCallback cb,
                        void *user_data);

/**
  * 函    数：注册释放回调函数及对应的用户数据
  * 参    数：btn       - 按键句柄
  *           cb        - 回调函数指针
  *           user_data - 回调时传入的用户自定义数据
  * 返 回 值：无
  * 说    明：保存释放回调函数和用户数据到控制块
  */
void button_attach_release(Button *btn,
                           ButtonCallback cb,
                           void *user_data);

/**
  * 函    数：按键状态机核心处理，需周期性调用（例如每 10ms）
  * 参    数：btn - 按键句柄
  * 返 回 值：无
  * 说    明：完成消抖、状态跳转、事件回调等功能
  */
void button_ticks(Button *btn);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */