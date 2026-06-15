#include "menu_engine.h"
#include "string.h"

//唯一与驱动耦合处
extern int OLED_Clear(void);
extern int OLED_Update(void);

//全局动画标志
static uint8_t g_is_animating = 1; 

//当前节点指针
static MenuNode_t *current = NULL;

//链树初始化，节点指针指向根节点
void menu_engine_init(MenuNode_t *root)
{
    current = root;
    if(current->on_enter) current->on_enter(current);
}

//每一帧的事件处理
void menu_engine_tick(MenuEvent_t event)
{
    /*处理及翻译事件*/
    MenuResult_t ret = current->on_input(current,event);

    /*根据返回值导航*/
    if(ret == MENU_RESULT_ENTER){
        if(current->children && current->cursor < current->child_count){
            if(current->on_exit) current->on_exit(current);
            current = current->children[current->cursor];
            if(current->on_enter) current->on_enter(current);
        }
    }else if(ret == MENU_RESULT_BACK){
        if(current->parent){
            if(current->on_exit) current->on_exit(current);
            current = current->parent;
            if(current->on_enter) current->on_enter(current);
        }
    }

    /*渲染*/
    // OLED_Clear(); // 注意：如果在这里清屏，可能会导致动画效果不流畅,因为每次都要更新全屏，增加了刷新时间
    if(current->on_render) current->on_render(current);
    // OLED_Update(); // 注意：如果在这里刷新，可能会导致动画效果不流畅,因为每次都要更新全屏，增加了刷新时间
}

//动画控制接口，外部调用以设置动画状态
void menu_engine_set_animating(uint8_t animating)
{
    g_is_animating = animating;
}

//查询当前动画状态，外部调用以决定是否响应输入事件
uint8_t menu_engine_is_animating(void)
{
    return g_is_animating;
}