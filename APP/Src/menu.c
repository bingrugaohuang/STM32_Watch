#include "menu.h"

//键值获取任务
void Key_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    Key();
  }
}
//UI任务
void UI_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
	UI();
  }
}
//设置任务
void Set_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    Setting();
  }
}

//菜单任务
void Menu_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    ShowMenu();
  }
}

//秒表任务
void Time_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    TimeUI();
  }
}

//手电筒任务
void Flashlight_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    Show_Flashlight();
  }
}

//MPU6050任务
void MPU6050_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    Show_MPU6050();
  }
}

//游戏任务
void Game_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  while(1)
  {
    Game();
  }
}

//闹钟服务任务
void Alarm_Task(void *argument)
{
  (void)argument;//避免编译器警告，表示该参数未被使用
  Alarm_ServiceTask(NULL);
}

