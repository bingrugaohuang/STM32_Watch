#ifndef MENU_ENGINE_H
#define MENU_ENGINE_H

#include <stdint.h>

/*菜单事件*/
typedef enum{
    MENU_EVENT_NONE = 0,
    MENU_EVENT_UP_SHORT,              //1
    MENU_EVENT_UP_LONG,               //2
    MENU_EVENT_UP_RELEASE,            //3
    MENU_EVENT_DOWN_SHORT,            //4
    MENU_EVENT_DOWN_LONG,             //5
    MENU_EVENT_DOWN_RELEASE,          //6
    MENU_EVENT_CONFIRM_SHORT,         //7
    MENU_EVENT_CONFIRM_LONG,          //8
    MENU_EVENT_CONFIRM_RELEASE,       //9

}MenuEvent_t;

/*菜单导航*/
typedef enum{
    MENU_RESULT_NONE = 0,
    MENU_RESULT_ENTER,
    MENU_RESULT_BACK,
}MenuResult_t;

/*菜单节点*/
typedef struct MenuNode MenuNode_t;  // 前向声明，告诉编译器 MenuNode_t 是一个结构体类型

struct MenuNode {
    const char  *title;
    MenuNode_t  *parent;
    MenuNode_t  **children;
    uint8_t     child_count;
    uint8_t     cursor;

    void         (*on_enter)(MenuNode_t *self);
    void         (*on_exit)(MenuNode_t *self);
    void         (*on_render)(MenuNode_t *self);
    MenuResult_t (*on_input)(MenuNode_t *self, MenuEvent_t event);

    void       *user_data;
};

/* 公共函数声明*/
void menu_engine_init(MenuNode_t *root);
void menu_engine_tick(MenuEvent_t);
void menu_engine_set_animating(uint8_t animating);
uint8_t menu_engine_is_animating(void);

#endif
