#include <stdint.h>
#include "gpio.h"
#include "log.h"

extern uint8_t button_serve_is_ready(void); // 声明外部函数，用于检查mpu是否初始化完成
extern void button_serve_start_timer_from_isr(void); // 声明外部函数，用于启动按键扫描定时器

static void (*btn_cfm_callback)(void) = NULL; // 按钮确认的回调函数指针

// 注册外部中断回调函数
void bsp_exti_register_callback(void (*callback)(void))
{
    // 这里可以根据 GPIO_Pin 来注册不同的回调函数
    // 例如，可以使用一个数组或链表来存储回调函数
    // 这里只是一个简单的示例，实际实现可能需要更复杂的数据结构
    // 注册按钮确认的回调函数
    btn_cfm_callback = callback;
}

// EXTI 中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  //判断引脚，CFM引脚与INT共用PA0
  if (GPIO_Pin == BTN_CFM_Pin){
    // 判断mpu是否已经初始化完成
    if(button_serve_is_ready()){ 
      if (btn_cfm_callback != NULL){
          btn_cfm_callback(); // 调用注册的回调函数
      }
      button_serve_start_timer_from_isr(); // 启动按键扫描定时器
    }
    return; // 直接返回，避免重复启动定时器
  }
  button_serve_start_timer_from_isr(); // 启动按键扫描定时器
}