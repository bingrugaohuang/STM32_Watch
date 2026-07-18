#include "button_service.h"
#include "bsp_button.h"
#include "osal.h"
#include "main.h"         /* 引入 Error_Handler 声明 */
#include "common_macro.h" /* 包含通用宏定义 */
#include "log.h"          /* 包含日志模块 */

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
static void btn_scan_timer_start_deferred(void *param1, uint32_t param2); /* 延迟启动按键扫描定时器回调函数 */
static void btn_scan_timer_stop_deferred(void *param1, uint32_t param2);  /* 延迟停止按键扫描定时器回调函数 */
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
    return COMMON_ERR_TIMEOUT; /* 获取事件失败 */
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

    //osal_timer_start(btn_scan_timer, 0);
}

/**
 * 函    数：启动按键扫描定时器
 * 注：由于mpu的配置是在开启调度后进行，
 *    因此如果一开始就开启定时器会导致误扫描到确认信号一次
 *    因此需要再mpu初始化后再开启定时器
 */
void button_serve_start_timer(void){
    osal_timer_start(btn_scan_timer, 0);  
}

/**
 * 函    数：延迟启动按键扫描定时器（从中断服务例程中调用）
 * 注：由于直接在中断中开启定时器会绑定中断执行时的tick，
 *    由于tickless唤醒后需要蓝软件定时器服务任务跳过补偿按键扫描定时器的回调，以避免影响消抖
 *    因此需要在中断中延迟启动定时器，避免绑定中断执行时的tick，而是绑定更新后的tick（定时器任务中调用）
 */
void button_serve_start_timer_from_isr(void){
    BaseType_t osal_higher_priority_task_woken = pdFALSE;
    osal_timer_deferred(btn_scan_timer_start_deferred, &osal_higher_priority_task_woken);
    portYIELD_FROM_ISR(osal_higher_priority_task_woken);
}

/**
 * 函    数：停止按键扫描定时器
 */
void button_serve_stop_timer(void){
    osal_timer_stop(btn_scan_timer, 0);  
}

/**
 * 函    数：从中断服务例程中延迟停止按键扫描定时器
 */
void button_serve_stop_timer_from_isr(void){
    BaseType_t osal_higher_priority_task_woken = pdFALSE;
    osal_timer_deferred(btn_scan_timer_stop_deferred, &osal_higher_priority_task_woken);
    portYIELD_FROM_ISR(osal_higher_priority_task_woken);
}


/*=====================私有函数=====================*/

/**
 * 函    数：延迟启动按键扫描定时器回调函数（从中断服务例程中调用）
 */
static void btn_scan_timer_start_deferred(void *param1, uint32_t param2){
    (void)param1;
    (void)param2;
    osal_timer_start(btn_scan_timer, 0);
}

static void btn_scan_timer_stop_deferred(void *param1, uint32_t param2){
    (void)param1;
    (void)param2;
    osal_timer_stop(btn_scan_timer, 0);
}

/**
 * 函    数：按键事件回调函数
 */
static void btn_callback(ButtonEvent event, void *user_data)
{
    Btn_pkg_t pkg;
    pkg.event = event;
    pkg.id = (uint8_t)(uintptr_t)user_data; /* 将用户数据转换为按键ID */

    /* 诊断日志：BTN_CFM 事件入队时记录事件类型，用于排查误触发来源 */
    if (pkg.id == BTN_CFM) {
        LOG_D(TAG_BTN, "BTN_CFM event queued: type=%d", (int)event);
    }

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

    /* 诊断日志：btn_cfm 消抖前置状态 + 当前 PA0 电平
       MPU INT = 195ms HIGH + 5ms LOW → 定时器 10ms 后读 PA0 应为 HIGH(1)
       按键按下 → PA0 持续 LOW(0) 数百 ms                                */
    // {
    //     uint8_t cfm_gpio = HAL_GPIO_ReadPin(BTN_CFM_GPIO_Port, BTN_CFM_Pin);
    //     LOG_D(TAG_BTN, "CFM scan: GPIO=%d state=%d flv=%d dbc=%d",
    //           cfm_gpio, (int)btn_cfm.state, (int)btn_cfm.filtered_level, (int)btn_cfm.debounce_cnt);
    // }

    /* 定时器回调中调用扫描函数，更新按键状态并触发事件 */
    button_ticks(&btn_last);
    button_ticks(&btn_next);
    button_ticks(&btn_cfm);

    // 三个按键都空闲时停止定时器，给Tickless正确的空闲时间
    if( btn_last.state == BTN_STATE_IDLE &&
        btn_next.state == BTN_STATE_IDLE &&
        btn_cfm.state  == BTN_STATE_IDLE &&
        btn_last.debounce_cnt == 0 &&
        btn_next.debounce_cnt == 0 &&
        btn_cfm.debounce_cnt == 0 )
    {
        osal_timer_stop(btn_scan_timer, 0);
        LOG_D(TAG_BTN, "All buttons idle, stopping scan timer.");
    }
}
