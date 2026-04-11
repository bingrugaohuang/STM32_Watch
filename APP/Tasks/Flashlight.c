#include "Flashlight.h"


static uint16_t CCR = 0;//PWM占空比值
static uint16_t CCR_Speed = 50;//PWM占空比调整速度
static int16_t CCR_Value = 0;//PWM占空比值
static float CCR_Filter = 0.0f;//PWM滤波状态，避免整数步进截断

//将占空比值限制在0-1000范围内
static int16_t ClampCCRValue(int16_t value)
{
	if(value < 0) return 0;
	if(value > 1000) return 1000;
	return value;
}

//根据按键更新目标占空比值
static void UpdateTargetByKey(uint8_t key)
{
	if(key == KEY_LAST)
	{
		CCR_Value -= CCR_Speed;
	}
	else if(key == KEY_NEXT)
	{
		CCR_Value += CCR_Speed;
	}
	else if(key == KEY_LAST_LONG)
	{
		CCR_Value -= 2 * CCR_Speed;
	}
	else if(key == KEY_NEXT_LONG)
	{
		CCR_Value += 2 * CCR_Speed;
	}

	CCR_Value = ClampCCRValue(CCR_Value);
}

//根据按键类型和差值大小计算滤波系数，差值越大追踪越快以降低体感延迟
static float GetFilterAlpha(uint8_t key, float absDiff)
{
	float alpha = 0.0f;

	if((key == KEY_NEXT_LONG) || (key == KEY_LAST_LONG))
	{
		alpha = 0.40f + absDiff * 0.00050f;
	}
	else
	{
		alpha = 0.26f + absDiff * 0.00042f;
	}

	if(alpha > 0.85f) alpha = 0.85f;
	return alpha;
}

//接近目标时提供最小步进，避免“快到目标却慢慢挪”的拖尾
static float GetMinStep(uint8_t key)
{
	if((key == KEY_NEXT_LONG) || (key == KEY_LAST_LONG)) return 4.0f;
	if((key == KEY_NEXT) || (key == KEY_LAST)) return 2.5f;
	return 2.0f;
}

//根据目标占空比和当前滤波状态更新实际占空比值，使用非线性步进以获得更好的用户体验
static void UpdateFilteredCCR(uint8_t key)
{
	float target = (float)CCR_Value;//目标占空比值
	float diff = target - CCR_Filter;//计算目标与当前滤波状态的差值
	float absDiff = (diff >= 0.0f) ? diff : -diff;//计算差值的绝对值

	// 与目标足够接近时直接吸附，避免浮点微小误差导致反复抖动
	if(absDiff > 0.5f)
	{
		float alpha = GetFilterAlpha(key, absDiff);
		float delta = diff * alpha;
		float minStep = GetMinStep(key);

		if((delta > 0.0f) && (delta < minStep)) delta = minStep;
		if((delta < 0.0f) && (delta > -minStep)) delta = -minStep;

		// 防止单次更新跨过目标值
		if((delta > 0.0f) && (delta > diff)) delta = diff;
		if((delta < 0.0f) && (delta < diff)) delta = diff;

		CCR_Filter += delta;
	}
	else
	{
		CCR_Filter = target;
	}

	// 将浮点滤波结果四舍五入到整数 PWM 比较值，而不是直接截断
	CCR = (uint16_t)(CCR_Filter + 0.5f);
}

//应用当前 CCR 值到 PWM 输出
static void ApplyFlashlightPWM(void)
{
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
	__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, CCR);
}

//显示手电筒界面
void ShowUI_Light(void)
{
	uint8_t CCR_Width = (float)CCR / 1000 * 100;
	
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ShowString(20, 40, "ON", OLED_8X16);
	OLED_ShowString(72, 40, "OFF", OLED_8X16);
	OLED_ReverseArea(24, 20, CCR_Width, 16);
	OLED_DrawRectangle(24, 20, 100, 16, OLED_UNFILLED);
}

//占空比设置
void CCR_Set(void)
{
	uint8_t Key = 0;

	CCR_Value = (int16_t)CCR;
	CCR_Filter = (float)CCR;
	while(1)
	{
		Key = Key_GetNum();
		if(Key == KEY_CONFIRM)
		{
			return;
		}

		UpdateTargetByKey(Key);//根据按键更新目标占空比值
		UpdateFilteredCCR(Key);//根据目标占空比和当前滤波状态更新实际占空比值

		OLED_Clear();
		ShowUI_Light();
		OLED_DrawTriangle(8, 20, 8, 36, 16, 28, OLED_FILLED);
		ShowFrames();
		OLED_Update();
		
		ApplyFlashlightPWM();//应用当前 CCR 值到 PWM 输出

		vTaskDelay(pdMS_TO_TICKS(5));//让出CPU，避免按键定时器服务任务饥饿
	}
}

//显示手电筒界面并处理按键
void Show_Flashlight(void)
{
	uint8_t KeyNum = 0;
	int8_t Cursor = 0;
	while(1)
	{
		KeyNum = Key_GetNum();
		if(KeyNum == KEY_CONFIRM)
		{
			switch(Cursor)
			{
				case 0:
					vTaskResume(MenuTaskHandle);//恢复UI任务
					vTaskSuspend(NULL);//挂起手电筒任务
					return;
				case 1:
					CCR_Set();
					break;
				case 2:
					CCR = CCR_Value;
					CCR_Filter = (float)CCR;
					ApplyFlashlightPWM();
					break;
				case 3:
					CCR = 0;
					CCR_Filter = 0.0f;
					HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
					break;
			}

		}
		else if(KeyNum == KEY_NEXT)
		{
			Cursor++;
			if(Cursor > 3) Cursor = 0;
		}
		else if(KeyNum == KEY_LAST)
		{
			Cursor--;
			if(Cursor < 0) Cursor = 3;
		}

		OLED_Clear();
		ShowUI_Light();
		switch(Cursor)
		{
			case 0:
				OLED_ReverseArea(0, 0, 16, 16);
				break;
			case 1:
				OLED_DrawTriangle(8, 20, 8, 36, 16, 28, OLED_UNFILLED);
				break;
			case 2:
				OLED_ReverseArea(20, 40, 16, 16);
				break;
			case 3:
				OLED_ReverseArea(72, 40, 24, 16);
				break;
			default:
				break;
		}
		ShowFrames();
		OLED_Update();
		vTaskDelay(10);//防止死循环占满CPU导致低优先级任务饿死
	}
}
