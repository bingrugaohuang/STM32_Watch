#include "ShowMenu.h"


uint8_t Pre_Select = 0;//上一个中心图标
uint8_t Target_Select = 0;//目标中心图标
uint8_t Direction = 0;//动画方向
uint8_t Moving = 0;//动画进行标志
int8_t x_pre = 48;//动画过程中中心图标的x坐标
uint8_t anime_speed = ANIME_SPEED;//动画速度

//显示动画的一帧	
void ShowAnimeFrame(void)
{
	OLED_Clear();
	OLED_ShowNum(64, 0, Pre_Select, 1, OLED_6X8);//显示光标位置
	OLED_ShowImage(42, 10, 44, 44, Frame);//显示选择框

	OLED_ShowImage(x_pre - 96, 16, 32, 32, Menu_Graph[(Pre_Select - 2 + 6) % 6]);//显示左侧-1图标
	OLED_ShowImage(x_pre - 48, 16, 32, 32, Menu_Graph[(Pre_Select - 1 + 6) % 6]);//显示左侧图标
	OLED_ShowImage(x_pre, 16, 32, 32, Menu_Graph[Pre_Select]);//显示原中心图标
	OLED_ShowImage(x_pre + 48, 16, 32, 32, Menu_Graph[(Pre_Select + 1) % 6]);//显示右侧图标
	OLED_ShowImage(x_pre + 96, 16, 32, 32, Menu_Graph[(Pre_Select + 2) % 6]);//显示右侧+1图标

	ShowFrames();
	OLED_Update();
}

//显示动画
void ShowAnime(void)
{
	if(Direction == KEY_LAST)  // 向左滑动，图标右移
	{
		x_pre += anime_speed;
		if(x_pre >= 96)   // 动画完成
		{
			Pre_Select = Target_Select;
			x_pre = 48;
			Moving = 0;
			Direction = 0;
		}
	}
	else if(Direction == KEY_NEXT)  // 向右滑动，图标左移
	{
		x_pre -= anime_speed;
		if(x_pre <= 0)
		{
			Pre_Select = Target_Select;
			x_pre = 48;
			Moving = 0;
			Direction = 0;
		}
	}
	ShowAnimeFrame();
}

//调度函数，根据中心图标进入对应任务
void Dispatch(void)
{
	switch (Pre_Select)
	{
	case 0:
		vTaskResume(UITaskHandle);//恢复UI任务
		vTaskSuspend(MenuTaskHandle);//挂起菜单任务
		break;
	case 1:
		vTaskResume(TimeTaskHandle);//恢复时间任务
		vTaskSuspend(MenuTaskHandle);//挂起菜单任务
		break;
	case 2:
		vTaskResume(FlashlightTaskHandle);//恢复手电筒任务
		vTaskSuspend(MenuTaskHandle);//挂起菜单任务
		break;
	case 3:
		vTaskResume(MPU6050TaskHandle);//恢复MPU6050任务
		vTaskSuspend(MenuTaskHandle);//挂起菜单任务
		break;
	case 4:
		vTaskResume(GameTaskHandle);//恢复小恐龙游戏任务
		vTaskSuspend(MenuTaskHandle);//挂起菜单任务
		break;
	case 5:
	
		break;
	default:
		break;
	}
}

//显示菜单界面
void ShowMenu(void)
{
	uint8_t KeyNum = 0;
	int8_t KeyCnt = 0;
	uint8_t Confirm_flag = 0;//确认键按下标志
	ShowAnimeFrame();
	while(1)
	{
		KeyNum = Key_GetNum();//获取键值与键值处理不能放在if(!Moving)里,
							  //否则动画过程中按键不能得到及时响应
			//按键处理
		if(KeyNum != 0)
		{
			switch(KeyNum)
			{
				case KEY_CONFIRM:
					KeyCnt = 0;
					Confirm_flag = 1;//不能将调度函数放在这里，否则会在动画过程中切换任务，
									 //导致动画无法完成,且返回菜单时会继续未完成的动画
					break;
				case KEY_NEXT:
					KeyCnt++;
					anime_speed = ANIME_SPEED;//正常速度
					break;
				case KEY_LAST:
					KeyCnt--;
					anime_speed = ANIME_SPEED;//正常速度
					break;
				case KEY_NEXT_LONG:
					KeyCnt++;
					anime_speed = 2 * ANIME_SPEED;//加速
					break;
				case KEY_LAST_LONG:
					KeyCnt--;
					anime_speed = 2 * ANIME_SPEED;//加速
					break;
				default:
					break;
			}
		}
		if(!Moving)
		{
			if(Confirm_flag == 1)
			{
				Confirm_flag = 0;
				Dispatch();
				ShowAnimeFrame(); // 从其他任务恢复后，立刻刷新一次画面
			}
			if(KeyCnt < 0)
			{
				KeyCnt++;
				Target_Select = (Pre_Select - 1 + 6) % 6;//向左滑动
				Direction = KEY_LAST;
				Moving = 1;
			}
			else if(KeyCnt > 0)
			{
				KeyCnt--;
				Target_Select = (Pre_Select + 1) % 6;//向右滑动
				Direction = KEY_NEXT;
				Moving = 1;
			}
		}
		else
		{
			ShowAnime();
		}
		
		vTaskDelay(pdMS_TO_TICKS(5));//让出cpu，同时时间尽量短以保证动画流畅和按键响应及时
	}
  
}
