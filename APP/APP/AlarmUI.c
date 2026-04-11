#include "AlarmUI.h"

/*
 * 函数功能：显示闹钟设置页的静态文本
 * 入口参数：cfg 当前闹钟配置
 * 返回值  ：无
 */
static void AlarmUI_ShowSettingBase(const AlarmConfig_t *cfg)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_Printf(16, 0, OLED_8X16, "ON:%s", cfg->Enabled ? "Y" : "N");
	OLED_Printf(0, 16, OLED_8X16, "1.TIME %02d:%02d", cfg->Hour, cfg->Min);
	OLED_Printf(0, 32, OLED_8X16, "2.REPEAT %s", (cfg->Repeat == ALARM_REPEAT_DAILY) ? "DAILY" : "ONCE");
	OLED_Printf(0, 48, OLED_8X16, "3.SNOOZE %2dM", cfg->SnoozeMin);
	ShowFrames();
}

/*
 * 函数功能：根据光标位置显示反色选中效果
 * 入口参数：cursor 光标位置，0表示返回
 * 返回值  ：无
 */
static void AlarmUI_ShowSettingSelection(int8_t cursor, const AlarmConfig_t *cfg)
{
	switch(cursor)
	{
		case 0:
			OLED_ReverseArea(0, 0, 16, 16);
			break;
        case 1:
			OLED_ReverseArea(16, 0, 32, 16);
			break;
		case 2:
			OLED_ReverseArea(56, 16, 40, 16);
			break;
		case 3:
            if(cfg->Repeat == ALARM_REPEAT_DAILY) OLED_ReverseArea(72, 32, 40, 16);
            else OLED_ReverseArea(72, 32, 32, 16);
			break;
		case 4:
            if(cfg->SnoozeMin < 10) OLED_ReverseArea(80, 48, 16, 16);
            else OLED_ReverseArea(72, 48, 24, 16);
			break;
		default:
			break;
	}
}

/*
 * 函数功能：编辑时间（时/分）
 * 入口参数：cfg 待修改配置
 * 返回值  ：无
 */
static void AlarmUI_EditTime(AlarmConfig_t *cfg)
{
	int8_t subCursor = 0;
	while(1)
	{
		uint8_t key = Key_GetNum();
		if(key == KEY_CONFIRM)
		{
			if(subCursor == 1)
			{
				return;
			}
			subCursor = (subCursor + 1) % 2;
		}
		else if(key == KEY_NEXT)
		{
			if(subCursor == 0)
			{
				cfg->Hour = (cfg->Hour + 1) % 24;
			}
			else if(subCursor == 1)
			{
				cfg->Min = (cfg->Min + 1) % 60;
			}
		}
		else if(key == KEY_LAST)
		{
			if(subCursor == 0)
			{
				cfg->Hour = (cfg->Hour + 23) % 24;
			}
			else if(subCursor == 1)
			{
				cfg->Min = (cfg->Min + 59) % 60;
			}
		}

		OLED_Clear();
		
        AlarmUI_ShowSettingBase(cfg);
		if(subCursor == 0) OLED_ReverseArea(56, 16, 16, 16);
		if(subCursor == 1) OLED_ReverseArea(80, 16, 16, 16);
		
		ShowFrames();
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/*
 * 函数功能：闹钟设置页面（最小可用版）
 * 入口参数：无
 * 返回值  ：0 表示返回上一级页面
 */
uint8_t AlarmUI_ShowSettingPage(void)
{
    uint8_t key = 0;
    int8_t ForthCursor = 0;
    AlarmConfig_t cfg;
    int8_t cursor = 0;
    Alarm_ServiceGetConfig(&cfg);
    while(1)
    {
        key = Key_GetNum();
        switch(key)
        {
            case KEY_CONFIRM:
                if(cursor == 0)
                {
                    Alarm_ServiceSetConfig(&cfg);
                    return 0;
                }
                else if(cursor == 1)
                {
                     cfg.Enabled = (uint8_t)!cfg.Enabled;
                }
                else if(cursor == 2)
                {
                    AlarmUI_EditTime(&cfg);
                }
                else if(cursor == 3)
                {
                    cfg.Repeat = (cfg.Repeat == ALARM_REPEAT_DAILY) ? ALARM_REPEAT_ONCE : ALARM_REPEAT_DAILY;
                }
                else if(cursor == 4)
                {
                    ForthCursor = !ForthCursor;
                }
                break;
            case KEY_NEXT:
                if(cursor == 4 && ForthCursor == 1)
                {
                    if(cfg.SnoozeMin < 30) cfg.SnoozeMin++;
                }
                else
                {
                    cursor = (cursor + 1) % 5;
                }
                break;
            case KEY_LAST:
                if(cursor == 4 && ForthCursor == 1)
                {
                    if(cfg.SnoozeMin > 1) cfg.SnoozeMin--;
                }
                else
                {
                    cursor = (cursor - 1 + 5) % 5;
                }
                break;
            default:
                break;
        }
        OLED_Clear();
        AlarmUI_ShowSettingBase(&cfg);
        AlarmUI_ShowSettingSelection(cursor, &cfg);
        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }    
}
/*
 * 函数功能：显示提醒页静态内容
 * 入口参数：cfg 当前闹钟配置
 *            blink 闪烁状态：0普通，1反色
 * 返回值  ：无
 */
static void AlarmUI_DrawRinging(const AlarmConfig_t *cfg, uint8_t blink)
{
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ShowString(20, 0, "ALARM", OLED_8X16);
	OLED_Printf(8, 16, OLED_12X24, "%02d:%02d", cfg->Hour, cfg->Min);
	OLED_ShowString(0, 48, "OK:STOP", OLED_8X16);
	OLED_ShowString(72, 48, "NEXT:SNZ", OLED_8X16);
	if(blink)
	{
		OLED_ReverseArea(0, 0, 128, 16);
	}
	ShowFrames();
}

/*
 * 函数功能：最小提醒页（OLED闪烁 + 停止 + 贪睡）
 * 入口参数：cfg 当前闹钟配置
 * 返回值  ：AlarmAction_t 用户动作
 */
AlarmAction_t AlarmUI_ShowRingingPage(const AlarmConfig_t *cfg)
{
	uint32_t lastBlinkTick = HAL_GetTick();
	uint8_t blink = 0;

	while(1)
	{
		uint8_t key = Key_GetNum();
		SleepMgr_ReportActivity(); // 响铃期间持续保活，避免被睡眠状态机打断
		if(key == KEY_CONFIRM)
		{
			return ALARM_ACTION_STOP;
		}
		if(key == KEY_NEXT)
		{
			return ALARM_ACTION_SNOOZE;
		}

		if(HAL_GetTick() - lastBlinkTick >= 300U)
		{
			lastBlinkTick = HAL_GetTick();
			blink = (uint8_t)!blink;
		}

		OLED_Clear();
		AlarmUI_DrawRinging(cfg, blink);
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

