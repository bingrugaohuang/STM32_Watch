#include "Gradienter.h" 


void ShowUI_Level(void)
{
	Angle angle = MPU6050_Cauculate();
	
	OLED_Clear();
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ReverseArea(0, 0, 16, 16);
	OLED_DrawCircle(64, 32, 30, OLED_UNFILLED);
	OLED_DrawCircle(64 - angle.Pitch * 0.38, 32 + angle.Roll * 0.38, 3, OLED_FILLED);
	ShowFrames();
	OLED_Update();
}

uint8_t Gradienter(void)
{
	uint8_t Key;
	while(1)
	{
		Key = Key_GetNum();
		if(Key == KEY_CONFIRM)
		{
			TranAnime(TRAN_DIR_RIGHT);//退出电子水平仪界面，右移
			return 0;
		}
		ShowUI_Level();
		vTaskDelay(5);//让出CPU，避免饿死软件定时器服务任务
	}
}
