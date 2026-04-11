#include "MPU6050_Task.h"

static void ShowUI_MPU6050(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	ShowFrames();
	OLED_Printf(0, 16, OLED_8X16, "1.MPU6050");
	OLED_Printf(0, 32, OLED_8X16, "2.Gradienter");
}

void Show_MPU6050(void)
{
	uint8_t Key = 0;
	uint8_t Cursor = 0;
	while(1)
	{
		Key = Key_GetNum();
		if(Key == KEY_CONFIRM)
		{
			switch (Cursor)
			{
				case 0:
					vTaskResume(MenuTaskHandle);//恢复菜单任务，进入菜单界面
					vTaskSuspend(NULL);//挂起当前任务，等待下次进入MPU6050界面时再恢复
					break;
				case 1:
					TranAnime(TRAN_DIR_LEFT);//进入MPU6050角度界面，左移
					MPU6050();
					break;
				case 2:
					TranAnime(TRAN_DIR_LEFT);//进入电子水平仪界面，左移
					Gradienter();
					break;
			}
		}
		else if(Key == KEY_NEXT)
		{
			Cursor = (Cursor + 1) % 3; 
		}
		else if(Key == KEY_LAST)
		{
			Cursor = (Cursor - 1 + 3) % 3;
		}

		OLED_Clear();
		ShowUI_MPU6050();
		// 根据Cursor的值显示不同的内容
		switch (Cursor)
		{
			case 0:
				OLED_ReverseArea(0, 0, 16, 16);
				break;
			case 1:
				OLED_ReverseArea(0, 16, 72, 16);
				break;
			case 2:
				OLED_ReverseArea(0, 32, 96, 16);
				break;
		}
		OLED_Update();
		vTaskDelay(10);
	}
}
