#include "modules/ScreenBright.h"

//将亮度值限制在0~100范围内
static uint8_t BrightnessClampPercent(int16_t value)
{
	if(value < 0) return 0;
	if(value > 100) return 100;
	return (uint8_t)value;
}

//亮度百分比转换为OLED对比度原始值
static uint8_t BrightnessRawFromPercent(uint8_t percent)
{
	uint16_t span = SCREEN_BRIGHTNESS_RAW_MAX - SCREEN_BRIGHTNESS_RAW_MIN;
	return (uint8_t)(SCREEN_BRIGHTNESS_RAW_MIN + ((uint16_t)percent * span + 50U) / 100U);
}

//OLED对比度原始值转换为亮度百分比
static uint8_t BrightnessPercentFromRaw(uint8_t raw)
{
	uint16_t span = SCREEN_BRIGHTNESS_RAW_MAX - SCREEN_BRIGHTNESS_RAW_MIN;

	if(raw <= SCREEN_BRIGHTNESS_RAW_MIN) return 0;
	if(raw >= SCREEN_BRIGHTNESS_RAW_MAX) return 100;
	return (uint8_t)(((uint16_t)(raw - SCREEN_BRIGHTNESS_RAW_MIN) * 100U + (span / 2U)) / span);
}

//将亮度百分比应用到屏幕
static void ApplyScreenBrightnessPercent(uint8_t percent)
{
	OLED_SetContrast(BrightnessRawFromPercent(percent));
}

//显示亮度设置界面
static void ShowUI_ScreenBrightness(uint8_t percent)
{
	uint8_t fillWidth = (uint8_t)(((uint16_t)percent * 96U + 50U) / 100U);

	OLED_ShowImage(0, 0, 16, 16, Return);
    OLED_ReverseArea(0, 0, 16, 16);
    OLED_Printf(0, 16 , OLED_8X16, "Value:%3d%%", percent);
	OLED_DrawRectangle(16, 36, 96, 8, OLED_UNFILLED);
	if(fillWidth > 0)
	{
		uint8_t innerWidth = fillWidth;
		if(innerWidth > 94U) innerWidth = 94U;
		OLED_ReverseArea(17, 37, innerWidth, 6);
	}

}

//亮度设置界面
uint8_t Menu_Third_SetScreenBrightness(void)
{
	uint8_t key = 0;
	uint8_t lastPercent = 0xFF;
	uint8_t percent = BrightnessPercentFromRaw(OLED_GetContrast());

	while(1)
	{
		key = Key_GetNum();
		if(key == KEY_CONFIRM)
		{
			return 0;
		}
		else if(key == KEY_NEXT)
		{
			percent = BrightnessClampPercent((int16_t)percent + SCREEN_BRIGHTNESS_STEP_SHORT);
		}
		else if(key == KEY_LAST)
		{
			percent = BrightnessClampPercent((int16_t)percent - SCREEN_BRIGHTNESS_STEP_SHORT);
		}
		else if(key == KEY_NEXT_LONG)
		{
			percent = BrightnessClampPercent((int16_t)percent + SCREEN_BRIGHTNESS_STEP_LONG);
		}
		else if(key == KEY_LAST_LONG)
		{
			percent = BrightnessClampPercent((int16_t)percent - SCREEN_BRIGHTNESS_STEP_LONG);
		}

		if(percent != lastPercent)
		{
			ApplyScreenBrightnessPercent(percent);
			lastPercent = percent;
		}

		OLED_Clear();
		ShowUI_ScreenBrightness(percent);
		ShowFrames();
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
