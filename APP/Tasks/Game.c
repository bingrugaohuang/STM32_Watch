#include "Game.h"

static const TaskMgrSwitchPlan_t gGameSwitchPlan = {
	TASKMGR_TASK_MENU, TASKMGR_TASK_GAME
};

void Game(void)
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
					TaskMgr_ApplySwitchPlan(&gGameSwitchPlan);//恢复菜单任务，挂起游戏任务
				}else if(Cursor == 1){
					TranAnime(TRAN_DIR_LEFT);//进入小恐龙游戏，左移
					Game_Dino();//进入小恐龙游戏
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
		OLED_ShowNum(64, 0, Cursor, 1, OLED_6X8);//显示光标位置
		OLED_ShowImage(0, 0, 16, 16, Return);
		OLED_ShowString(0, 16, "小恐龙游戏", OLED_8X16);
		switch (Cursor)
		{
			case 0:
				OLED_ReverseArea(0, 0, 16, 16);
				break;
			case 1:
				OLED_ReverseArea(0, 16, 80, 16);
				break;
			default:
				break;
		}
		ShowFrames();
		OLED_Update();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}
