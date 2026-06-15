#include "display_service.h"
#include "menu_engine.h"
#include "osal.h"
#include "common_macro.h"
#include "button_service.h"
#include "log.h"
#include "main.h"
#include "oled_driver.h"

// 首页节点
extern MenuNode_t menu_home;

// 显示任务句柄
osal_task_handle_t displaytask_handle = NULL;

// 静态函数声明
static void displaytask(void* pvParameters);
static MenuEvent_t translate(Btn_pkg_t *btn);

// 显示任务创建
void displaytask_init(void)
{
    displaytask_handle = osal_task_create("DisplayTask", displaytask, DISPLAYTASK_STACK, NULL, DISPLAYTASK_PRIORITY);
    if(!displaytask_handle) {
        LOG_E("DISPLAY","Initialization failed");
        Error_Handler();
    }
    LOG_I("DISPLAY","Initialization completed");
}

// 显示任务
static void displaytask(void* pvParameters)
{
    (void)pvParameters; // 避免未使用参数的编译警告
    OLED_Init();
    menu_engine_init(&menu_home);

    while(1)
    {
        // 接收按键队列并处理，翻译为菜单事件
        Btn_pkg_t btn; 
        MenuEvent_t event = MENU_EVENT_NONE;

        // 根据当前动画状态设置不同的等待时间，动画过程中更频繁地检查输入以保持响应性
        uint32_t wait_ticks = menu_engine_is_animating() ? 10 : 200;
        
        if(btn_service_getnum(&btn, wait_ticks) == COMMON_ERR_OK){
           event = translate(&btn);
           if(event) LOG_I("DISPLAY","Received menu event %d", event);
        }

        // 接收MPU6050处理任务发来的数据

        // 接收ADC电池电压采集任务发来的数据

        menu_engine_tick(event);
    }
}

// 将按键队列接收信息翻译为菜单事件
static MenuEvent_t translate(Btn_pkg_t *btn)
{
    if(btn->id == BTN_LAST){
        if(btn->event == BTN_EVENT_SHORT_PRESS){
            return MENU_EVENT_UP_SHORT;
        }else if(btn->event == BTN_EVENT_LONG_PRESS || btn->event == BTN_EVENT_LONG_PRESS_REPEAT){
            return MENU_EVENT_UP_LONG;
        }
    }else if(btn->id == BTN_NEXT){
        if(btn->event == BTN_EVENT_SHORT_PRESS){
            return MENU_EVENT_DOWN_SHORT;
        }else if(btn->event == BTN_EVENT_LONG_PRESS || btn->event == BTN_EVENT_LONG_PRESS_REPEAT){
            return MENU_EVENT_DOWN_LONG;
        }
    }else if(btn->id == BTN_CFM){
        if(btn->event == BTN_EVENT_SHORT_PRESS){
            return MENU_EVENT_CONFIRM_SHORT;
        }else if(btn->event == BTN_EVENT_LONG_PRESS || btn->event == BTN_EVENT_LONG_PRESS_REPEAT){
            return MENU_EVENT_CONFIRM_LONG;
        }
    }
    return MENU_EVENT_NONE; // 无事件
}
