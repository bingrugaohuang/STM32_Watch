#include "tasks/Setting.h"

//显示设置界面
static void ShowUI_Setting(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ShowString(0, 16, "1.设置时间", OLED_8X16);
	OLED_ShowString(0, 32, "2.显示栈高水位",OLED_8X16);
	OLED_ShowString(0, 48, "3.设置屏幕亮度", OLED_8X16);

}


static void DrawSettingPage(int8_t Cursor)
{
	ShowUI_Setting();
	OLED_ShowNum(64, 0, Cursor, 1, OLED_6X8);//显示光标位置

	switch (Cursor)
	{
		case 0:
			OLED_ReverseArea(0, 0, 16, 16);
			break;
		case 1:
			OLED_ReverseArea(16, 16, 64, 16);
			break;
		case 2:
			OLED_ReverseArea(16, 32, 96, 16);
			break;
		case 3:
			OLED_ReverseArea(16, 48, 96, 16);
			break;
		default:
			break;
	}

	ShowFrames();
}



//设置界面调度
void Setting(void)
{
	uint8_t KeyNum = 0;
	int8_t Cursor = 0;
	while (1)
	{
		KeyNum = Key_GetNum();
		switch (KeyNum)
		{
			case KEY_CONFIRM:
				if(Cursor == 0){
					TranAnime(TRAN_DIR_RIGHT);//退出设置，右移
					vTaskResume(UITaskHandle);//恢复UI任务
					vTaskSuspend(NULL);//挂起设置任务
				}else if(Cursor == 1){
					TranAnime(TRAN_DIR_LEFT);//进入应用，左移
					Menu_Third_SetTime();//进入设置时间界面
				}else if(Cursor == 2){
					Menu_Third_ShowStackWatermark();//进入控制显示栈高水位界面
				}else if(Cursor == 3){
					TranAnime(TRAN_DIR_LEFT);//进入应用，左移
					Menu_Third_SetScreenBrightness();//进入屏幕亮度设置界面
				}
				break;
			case KEY_NEXT:
				Cursor++;
				if(Cursor > 3) Cursor = 0;
				break;
			case KEY_LAST:
				Cursor--;
				if(Cursor < 0) Cursor = 3;
				break;
			case KEY_LAST_LONG:
				Cursor = (Cursor - 1 + 4) % 4;
				break;
			case KEY_NEXT_LONG:
				Cursor = (Cursor + 1) % 4;
				break;
			default:
				break;
		}
		OLED_Clear();
		DrawSettingPage(Cursor);
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
