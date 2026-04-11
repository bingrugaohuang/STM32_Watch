#include "tasks/Time_Task.h"

void ShowTimeUI(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ShowString(0, 16, "1.秒表", OLED_8X16);
	OLED_ShowString(0, 32, "2.闹钟", OLED_8X16);
	ShowFrames();
}

void ShowTimeUI_Selection(int8_t Cursor)
{
	switch(Cursor)
	{
		case 0:
			OLED_ReverseArea(0, 0, 16, 16);
			break;
		case 1:
			OLED_ReverseArea(16, 16, 32, 16);
			break;
		case 2:
			OLED_ReverseArea(16, 32, 32, 16);
			break;
		default:
			break;
	}
}

void TimeUI(void)
{
	int8_t Cursor = 0;
	while(1)
	{
		uint8_t KeyNum = Key_GetNum();
		switch (KeyNum)
		{
			case KEY_CONFIRM:
				if(Cursor == 0){
					vTaskResume(MenuTaskHandle);//恢复菜单任务
					vTaskSuspend(NULL);//挂起时间任务
				}
				else if(Cursor == 1){
					Show_StopWatch();
				}
				else if(Cursor == 2){
					AlarmUI_ShowSettingPage();//进入闹钟设置页
				}
				break;
			case KEY_NEXT:
				Cursor = (Cursor + 1) % 3;
				break;
			case KEY_LAST:
				Cursor = (Cursor - 1 + 3) % 3;
				break;
			default:
				break;
		}
		OLED_Clear();

		ShowTimeUI();

		ShowTimeUI_Selection(Cursor);
			
		OLED_Update();

		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
