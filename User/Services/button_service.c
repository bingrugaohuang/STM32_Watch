#include "button_service.h"
#include "bsp_button.h"
#include "osal.h"
#include "main.h"         /* 引入 Error_Handler 声明 */
#include "common_macro.h" /* 包含通用宏定义 */

/****************** 用户按钮服务层实现 ******************/
/* 宏配置*/
#define BTN_EVENT_QUEUE_LENGTH              8   /* 按键事件队列长度 */
#define BTN_SCAN_TIMER_PERIOD pdMS_TO_TICKS(10) /* 10ms 扫描周期 */
#define BTN_LAST_ACTIVE_LEVEL               0   /* "上一个"按钮有效电平 */
#define BTN_NEXT_ACTIVE_LEVEL               0   /* "下一个"按钮有效电平 */
#define BTN_CFM_ACTIVE_LEVEL                0   /* "确认"按钮有效电平 */

/* 按键控制块实例 */
static Button btn_last;  /* 上一个 */
static Button btn_next;  /* 下一个 */
static Button btn_cfm;   /* 确认 */

/* 私有变量*/
static osal_queue_handle_t btn_event_queue; /* 按键事件队列 */
static osal_timer_handle_t btn_scan_timer;  /* 按键扫描定时器 */

/****************** 私有函数声明 ******************/
static void btn_callback(ButtonEvent event, void *user_data); /* 按键事件回调函数 */
static void btn_scan_timer_callback(osal_timer_handle_t xTimer); /* 按键扫描定时器回调函数 */

/*=====================公共函数=====================*/

/**
 * 函    数：接口函数，供其他模块调用获取按键包，具体内容包括按键事件类型和按键ID，交给上层判断处理
 */
int btn_service_getnum(Btn_pkg_t *pkg, uint32_t timeout)
{
    configASSERT(pkg != NULL);

    if(osal_queue_receive(btn_event_queue, pkg, timeout == 0 ? OSAL_NO_WAIT : pdMS_TO_TICKS(timeout)) == OSAL_OK)
    {
        return COMMON_ERR_OK; /* 成功获取事件 */
    }
    return COMMON_ERR_QUEUE_FULL; /* 获取事件失败 */
}

/**
 * 函    数：按钮服务初始化
 */
void button_service_init(void)
{
    /* 创建按键事件队列 */
    btn_event_queue = osal_queue_create(BTN_EVENT_QUEUE_LENGTH, sizeof(Btn_pkg_t));
    if(btn_event_queue == NULL)
    {
        Error_Handler();
    }

    /* 初始化按键控制块，传入对应的读取函数和有效电平 */
    button_init(&btn_last, bsp_btn_last_read, BTN_LAST_ACTIVE_LEVEL);
    button_init(&btn_next, bsp_btn_next_read, BTN_NEXT_ACTIVE_LEVEL);
    button_init(&btn_cfm,  bsp_btn_cfm_read,  BTN_CFM_ACTIVE_LEVEL);

    /* 注册回调函数和用户数据（如果需要） */
    button_attach_short(&btn_last, btn_callback, (void*)(uintptr_t)BTN_LAST);
    button_attach_short(&btn_next, btn_callback, (void*)(uintptr_t)BTN_NEXT);
    button_attach_short(&btn_cfm,  btn_callback, (void*)(uintptr_t)BTN_CFM);

    button_attach_long(&btn_last, btn_callback, (void*)(uintptr_t)BTN_LAST);
    button_attach_long(&btn_next, btn_callback, (void*)(uintptr_t)BTN_NEXT);
    button_attach_long(&btn_cfm,  btn_callback, (void*)(uintptr_t)BTN_CFM);

    button_attach_release(&btn_last, btn_callback, (void*)(uintptr_t)BTN_LAST);
    button_attach_release(&btn_next, btn_callback, (void*)(uintptr_t)BTN_NEXT);
    button_attach_release(&btn_cfm,  btn_callback, (void*)(uintptr_t)BTN_CFM);

    /* 创建按键扫描定时器 */
    btn_scan_timer  = osal_timer_create("Btn_Scan_Timer",
                                        BTN_SCAN_TIMER_PERIOD, 
                                        1,                          /* 自动重载 */
                                        NULL, 
                                        btn_scan_timer_callback);
    if(btn_scan_timer == NULL)
    {
        Error_Handler();
    }

    osal_timer_start(btn_scan_timer, 0);
}

/*=====================私有函数=====================*/

/**
 * 函    数：按键事件回调函数
 */
static void btn_callback(ButtonEvent event, void *user_data)
{
    Btn_pkg_t pkg;
    pkg.event = event;
    pkg.id = (uint8_t)(uintptr_t)user_data; /* 将用户数据转换为按键ID */
    if(osal_queue_send(btn_event_queue, &pkg, 0) != OSAL_OK )
    {
        // 可选：日志记录或错误计数，方便排查
    }
}

/**
 * 函    数：按键扫描定时器回调函数
 */
static void btn_scan_timer_callback(osal_timer_handle_t xTimer)
{
    (void)xTimer;
    /* 定时器回调中调用扫描函数，更新按键状态并触发事件 */
    button_ticks(&btn_last);
    button_ticks(&btn_next);
    button_ticks(&btn_cfm);
}
