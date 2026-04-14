#include "UI.h"

RTC_DateTypeDef RTC_Date;
RTC_TimeTypeDef RTC_Time;

static const TaskMgrSwitchPlan_t gUI_SwitchPlan[] = {
	{TASKMGR_TASK_MENU, TASKMGR_TASK_UI},
	{TASKMGR_TASK_SET, TASKMGR_TASK_UI}
};

//显示首页时钟
void ShowClock(void)
{
	HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
	HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);

	OLED_Printf(12, 0, OLED_6X8, "%02d-%d-%d", 
		RTC_Date.Year, RTC_Date.Month, RTC_Date.Date);
	OLED_ShowNum(0, 0, 20, 2, OLED_6X8);
	OLED_Printf(16, 16, OLED_12X24, "%02d:%02d:%02d", 
		RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
	OLED_ShowString(0, 48, "菜单", OLED_8X16);
	OLED_ShowString(96, 48, "设置", OLED_8X16);
}

void UI(void)
{
	uint8_t KeyNum = 0;
	int8_t Cursor = 0;

	uint8_t LastKeyNum = 0;

	while(1)
	{
		KeyNum = Key_GetNum();
		switch(KeyNum)
		{
			case KEY_CONFIRM:
				if(Cursor == 0){
					OLED_Clear();
					TaskMgr_ApplySwitchPlan(&gUI_SwitchPlan[0]);//恢复菜单任务，挂起UI任务
				}else if(Cursor == 1){
					TranAnime(TRAN_DIR_LEFT);//进入设置界面，左移
					TaskMgr_ApplySwitchPlan(&gUI_SwitchPlan[1]);//恢复设置任务，挂起UI任务
				}
				break;
			case KEY_NEXT:
				Cursor++;
				if(Cursor > 1) Cursor = 0;
				break;
			case KEY_LAST:
				Cursor--;
				if(Cursor < 0) Cursor = 1;
				break;
			default:
				break;
	}
	OLED_Clear();

	//测试代码，显示按键值和光标位置
	if(KeyNum != 0 && KeyNum != LastKeyNum)
	{
		LastKeyNum = KeyNum;
	}
	OLED_ShowNum(64, 0, Cursor, 1, OLED_6X8);//显示光标位置
	OLED_ShowNum(70, 0, LastKeyNum, 1, OLED_6X8);//显示按键值
	//测试代码，显示按键值和光标位置

	ShowClock();
	switch (Cursor)
	{
		case 0:
			OLED_ReverseArea(0, 48, 32, 16);
			break;
		case 1:
			OLED_ReverseArea(96, 48, 32, 16);
			break;
		default:
			break;
	}
	ShowFrames();
	OLED_Update();
	vTaskDelay(pdMS_TO_TICKS(10));
	}
}
