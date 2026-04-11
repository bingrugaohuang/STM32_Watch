#include "StopWatch_APP.h"

static uint8_t hour, min, sec;

//显示秒表界面
void ShowUI_StopWatch(void)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_Printf(32, 20, OLED_8X16, "%02d:%02d:%02d", hour, min, sec);
	OLED_ShowString(8, 40, "开始", OLED_8X16);
	OLED_ShowString(48, 40, "暂停", OLED_8X16);
	OLED_ShowString(88, 40, "清除", OLED_8X16);
}

//秒表软件定时器回调函数
void StopWatch_Tick(TimerHandle_t xTimer)
{
	(void)xTimer; // 防止未使用参数报警
	sec++;
	if(sec >= 60)
	{
		sec = 0;
		min++;
		if(min >= 60)
		{
			min = 0;
			hour++;
			if(hour >= 100)
			{
				hour = 0;
			}
		}
	}
	//vTaskDelay(pdMS_TO_TICKS(5));//验证软件定时器回调函数的补跑机制
}

//秒表控制函数，根据Control参数控制秒表的开始、暂停和清除
void StopWatchControl(uint8_t Control)
{
	static uint8_t Running = 0;//秒表运行状态
	switch(Control)
	{
		case 1://开始
			if(Running == 0){
				xTimerStart(StopWatchTimerHandle, 0);//启动秒表定时器
				Running = 1;
			}
			break;
		case 2://暂停
			if(Running == 1){
				xTimerStop(StopWatchTimerHandle, 0);//停止秒表定时器
				Running = 0;
			}
			break;
		case 3://清除
			Running = 0;
			hour = min = sec = 0;
			xTimerStop(StopWatchTimerHandle, 0);//停止秒表定时器
			break;
		default:
			break;
	}
}

//秒表任务处理函数
uint8_t Show_StopWatch(void)
{
	uint8_t KeyNum = 0;
	int8_t Cursor = 0;
	while(1)
	{
		KeyNum = Key_GetNum();
		switch (KeyNum)
		{
			case KEY_CONFIRM:
				if(Cursor == 0){
					return 0;
				}else{
					StopWatchControl(Cursor);
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
			default:
				break;
		}
		OLED_Clear();
		OLED_ShowNum(64, 0, Cursor, 1, OLED_6X8);//显示光标位置
		ShowUI_StopWatch();
		switch(Cursor)
		{
			case 0:
				OLED_ReverseArea(0, 0, 16, 16);
				break;
			case 1:
				OLED_ReverseArea(8, 40, 32, 16);
				break;
			case 2:
				OLED_ReverseArea(48, 40, 32, 16);
				break;
			case 3:
				OLED_ReverseArea(88, 40, 32, 16);
				break;
			default:
				break;
		}
		ShowFrames();
		OLED_Update();
		
		vTaskDelay(pdMS_TO_TICKS(10)); // 防止死循环占满CPU导致低优先级任务饿死
	}
}
