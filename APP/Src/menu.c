#include "menu.h"
#include "Key.h"
#include "UI.h"
#include "Setting.h"
#include "ShowMenu.h"
#include "Time_Task.h"
#include "Flashlight.h"
#include "MPU6050_Task.h"
#include "Game.h"
#include "AlarmService.h"

//��ֵ��ȡ����
void Key_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    Key();
  }
}
//UI����
void UI_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
	UI();
  }
}
//��������
void Set_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    Setting();
  }
}

//�˵�����
void Menu_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    ShowMenu();
  }
}

//�������
void Time_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    TimeUI();
  }
}

//�ֵ�Ͳ����
void Flashlight_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    Show_Flashlight();
  }
}

//MPU6050����
void MPU6050_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    Show_MPU6050();
  }
}

//��Ϸ����
void Game_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  while(1)
  {
    Game();
  }
}

//���ӷ�������
void Alarm_Task(void *argument)
{
  (void)argument;//������������棬��ʾ�ò���δ��ʹ��
  Alarm_ServiceTask(NULL);
}
