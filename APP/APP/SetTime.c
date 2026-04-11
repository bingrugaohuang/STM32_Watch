#include "modules/SetTime.h"


extern RTC_DateTypeDef RTC_Date;
extern RTC_TimeTypeDef RTC_Time;

uint8_t *MyRTC_Time[6];//此处建一个数组是因为原标准库用到了，
											 //移植过来改用RTC结构体要改很多东西，不如这样方便

#define INDEX_YEAR 0
#define INDEX_MON  1
#define INDEX_DAY  2
#define INDEX_HOUR 3
#define INDEX_MIN  4
#define INDEX_SEC  5

//显示第一页：年、月、日
void ShowUI1()
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_Printf(0, 16, OLED_8X16, "年:%04d", RTC_Date.Year);
	OLED_ShowNum(24, 16, 20, 2, OLED_8X16);
	OLED_Printf(0, 32, OLED_8X16, "月:%2d", RTC_Date.Month);
	OLED_Printf(0, 48, OLED_8X16, "日:%2d", RTC_Date.Date);
}

//显示第二页：时、分、秒
void ShowUI2()
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_Printf(0, 16, OLED_8X16, "时:%2d", RTC_Time.Hours);
	OLED_Printf(0, 32, OLED_8X16, "分:%2d", RTC_Time.Minutes);
	OLED_Printf(0, 48, OLED_8X16, "秒:%2d", RTC_Time.Seconds);
}

void SetTime(uint8_t Key, uint8_t Index)
{
	int16_t value = *MyRTC_Time[Index];
	if(Key == KEY_LAST) value--;
	else if(Key == KEY_NEXT) value++;
	
	switch (Index)
	{
		case INDEX_YEAR:
			break;
		case INDEX_MON:
			if(value > 12) value = 1;
			else if(value < 1) value = 12;
			break;
		case INDEX_DAY:
			if(value > 31) value = 1;
			else if(value < 1) value = 31;
			break;
		case INDEX_HOUR:
			if(value > 23) value = 0;
			else if(value < 0) value = 23;
			break;
		case INDEX_MIN:
			if(value > 59) value = 0;
			else if(value < 0) value = 59;
			break;
		case INDEX_SEC:
			if(value > 59) value = 0;
			else if(value < 0) value = 59;
			break;
	}
	*MyRTC_Time[Index] = (uint8_t)value;
	HAL_RTC_SetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
	HAL_RTC_SetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
	RTC_SaveDateToBackup(&RTC_Date);
}

uint8_t SetYear(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_YEAR);
			ShowUI1();
			OLED_ReverseArea(24, 16, 32, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

uint8_t SetMonth(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_MON);
			ShowUI1();
			OLED_ReverseArea(24, 32, 16, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

uint8_t SetDay(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_DAY);
			ShowUI1();
			OLED_ReverseArea(24, 48, 16, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

uint8_t SetHour(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_HOUR);
			ShowUI2();
			OLED_ReverseArea(24, 16, 16, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

uint8_t SetMin(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_MIN);
			ShowUI2();
			OLED_ReverseArea(24, 32, 16, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

uint8_t SetSec(void)
{
	uint8_t Key;

	while(1)
	{
		Key = Key_GetNum();
		
		if(Key != KEY_CONFIRM) 
		{
			SetTime(Key, INDEX_SEC);
			ShowUI2();
			OLED_ReverseArea(24, 48, 16, 16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}	

		else if(Key == KEY_CONFIRM) return 0;
	}
}

//三级菜单——设置界面
uint8_t Menu_Third_SetTime(void)
{
	static uint8_t Key;
	static uint8_t cursor = 1;
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    MyRTC_Time[INDEX_YEAR] = &RTC_Date.Year;
    MyRTC_Time[INDEX_MON] = &RTC_Date.Month;
    MyRTC_Time[INDEX_DAY] = &RTC_Date.Date;
    MyRTC_Time[INDEX_HOUR] = &RTC_Time.Hours;
    MyRTC_Time[INDEX_MIN] = &RTC_Time.Minutes;
    MyRTC_Time[INDEX_SEC] = &RTC_Time.Seconds;
	
	while(1)
	{
		Key = Key_GetNum();
		//判断按键键值
		if(Key == KEY_LAST)//Last
		{
			cursor--;
			if(cursor < 1)
			{
				cursor = 7;
			}
		}
		else if(Key == KEY_NEXT)//Next
		{
			cursor++;
			if(cursor > 7)
			{
				cursor = 1;
			}
		}
		else if(Key == KEY_CONFIRM)//Enter
		{
			
			//OLED_Clear();
			//OLED_Update();
			switch (cursor)
			{
				case 1:
					//OLED_Update();
					TranAnime(TRAN_DIR_RIGHT);
					return 0;
				case 2:					
					SetYear();
					break;
				case 3:
					SetMonth();
					break;
				case 4:
					SetDay();
					break;
				case 5:
					SetHour();
					break;
				case 6:
					SetMin();
					break;
				case 7:
					SetSec();
					break;
			}
		}
		
		//光标在指定位置闪烁
		if(cursor <= 4 )//第一页
		{
			OLED_Clear();
			OLED_ShowNum(64, 0, cursor - 1, 1, OLED_6X8);//显示光标位置
			ShowUI1();
			switch (cursor)
			{
				case 1:
					OLED_ReverseArea(0, 0, 16, 16);
					break;
				case 2:
					OLED_ReverseArea(0, 16, 16, 16);
					break;
				case 3:
					OLED_ReverseArea(0, 32, 16, 16);
					break;
				case 4:
					OLED_ReverseArea(0, 48, 16, 16);
					break;
			}
				ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}
		else if(cursor > 4)//第二页
		{
			OLED_Clear();
			OLED_ShowNum(64, 0, cursor - 1, 1, OLED_6X8);//显示光标位置
			ShowUI2();
			switch (cursor)
			{
				case 5:
					OLED_ReverseArea(0, 16, 16, 16);
					break;
				case 6:
					OLED_ReverseArea(0, 32, 16, 16);
					break;
				case 7:
					OLED_ReverseArea(0, 48, 16, 16);
					break;
			}	
				ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(20));
		}
		
	}


}
