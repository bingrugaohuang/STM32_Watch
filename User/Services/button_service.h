#ifndef BUTTON_SERVICE_H
#define BUTTON_SERVICE_H

#include "button.h"
#include <stdint.h>

/* 按键ID宏定义 */
#define BTN_LAST 1      
#define BTN_NEXT 2
#define BTN_CFM  3

/* 按键事件包结构体，用于在队列中传递按键事件 */
typedef struct{
    ButtonEvent event; /* 按键事件类型 */
    uint8_t id;        /* 按键ID：BTN_LAST, BTN_NEXT, BTN_CFM */
}Btn_pkg_t;

/*==================公共函数==================*/

/*
 * 函    数：btn_service_getnum
 * 参    数：pkg - 输出参数，存储获取到的按键事件包
 *          timeout - 超时时间（ms）
 * 返 回 值：0 - 成功获取事件，-1 - 获取事件失败
 */
int btn_service_getnum(Btn_pkg_t *pkg, uint32_t timeout);

/*
 * 函    数：button_service_init
 * 参    数：无
 * 返 回 值：无
 * 说    明：初始化按钮服务，创建事件队列和扫描定时器，配置按键控制块。
 */
void button_service_init(void);

#endif /* BUTTON_SERVICE_H */
