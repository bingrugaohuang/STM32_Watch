#include "menu.h"
#include "Key.h"
#include "ui/UI.h"
#include "tasks/Setting.h"
#include "tasks/ShowMenu.h"
#include "tasks/Time_Task.h"
#include "tasks/Flashlight.h"
#include "tasks/MPU6050_Task.h"
#include "tasks/Game.h"
#include "services/AlarmService.h"

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
