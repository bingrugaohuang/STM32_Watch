#include <stdint.h>
#include "gpio.h"

static void (*btn_cfm_callback)(void) = NULL; // 按钮确认的回调函数指针

void bsp_exti_register_callback(void (*callback)(void))
{
    // 这里可以根据 GPIO_Pin 来注册不同的回调函数
    // 例如，可以使用一个数组或链表来存储回调函数
    // 这里只是一个简单的示例，实际实现可能需要更复杂的数据结构
    // 注册按钮确认的回调函数
    btn_cfm_callback = callback;

}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == BTN_CFM_Pin)
  {
    if (btn_cfm_callback != NULL)
    {
        btn_cfm_callback(); // 调用注册的回调函数
    }
  }
}