#include "menu.h"

void Key_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    Key();
  }
}

void UI_Task(void *argument)
{
  (void)argument;
  while(1)
  {
	UI();
  }
}

void Set_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    Setting();
  }
}


void Menu_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    ShowMenu();
  }
}


void Time_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    TimeUI();
  }
}


void Flashlight_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    Show_Flashlight();
  }
}


void MPU6050_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    Show_MPU6050();
  }
}


void Game_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    Game();
  }
}


void Alarm_Task(void *argument)
{
  (void)argument;
  Alarm_ServiceTask(NULL);
}
