#include "modules/dino.h"


static uint16_t Ground_Pos = 0;//地面数组中投射到屏幕上x为0的字节的下标，每20ms加一
static uint8_t Barrier_Pos = 0;
static uint8_t Cloud_Pos = 0;
static uint8_t Jump_Pos = 0;
static uint8_t barrier_flag = 0;//障碍物标志
static uint8_t Dino_Turn = 0;//前后腿切换
static uint8_t Dino_Jump_Flag = 0;
static uint8_t crash_flag = 0;
static uint8_t Game_flag = 0;
static uint16_t Score = 0;
static float pi = 3.1415926;

static uint16_t Jump_cnt;


typedef struct 
{
	uint8_t xmin;
	uint8_t xmax;
	uint8_t ymin;
	uint8_t ymax;
}Pos;


void ShowScore(void)
{
	OLED_ShowNum(98, 8, Score, 5, OLED_6X8);
}

//显示地面的移动
void ShowGround(void)
{
	if(Ground_Pos < 128)
	{
		for(uint8_t i = 0; i < 128; i++)
		{
			OLED_DisplayBuf[7][i] = Ground[Ground_Pos + i];
		}
	}
	else
	{
		for(uint8_t i = 0; i < 255 - Ground_Pos; i++)
		{
			OLED_DisplayBuf[7][i] = Ground[Ground_Pos + i];
		}
		for(uint8_t i= 255 - Ground_Pos; i < 128; i++)
		{
			OLED_DisplayBuf[7][i] = Ground[i - (255 - Ground_Pos)];
		}
	}
	
}

Pos barrier;
//显示障碍物移动
void ShowBarrier(void)
{
	if(Barrier_Pos >= 135)
	{
		barrier_flag = rand() % 3;
	}
	OLED_ShowImage(127 - Barrier_Pos, 45, 8, 16, Barrier[barrier_flag]);
	
	barrier.xmin = 127 - Barrier_Pos;
	barrier.xmax = 127 - Barrier_Pos + 8;
	barrier.ymin = 45;
	barrier.ymax = 45 +16;

}

void ShowCloud(void)
{
	OLED_ShowImage(127 - Cloud_Pos, 8, 32, 16, Cloud);//每50ms动一下
}

Pos dino;
//小恐龙显示
void ShowDino(void)
{
	if(!Dino_Jump_Flag)
	{
		OLED_ShowImage(0, 44, 16, 18, Dino[Dino_Turn]);
	}
	else 
	{
		Jump_Pos = 28 * sin((float)(pi * Jump_cnt / 100));
		OLED_ShowImage(0, 44 - Jump_Pos, 16, 18, Dino[2]);
	}
	
	dino.xmin = 0;
	dino.xmax = 15;
	dino.ymin = 44 - Jump_Pos;
	dino.ymax = 44 -Jump_Pos + 18;
}

//碰撞判断
void Crash_Judge(Pos *a, Pos *b)
{
	
	
	if(a->xmax >= b->xmin && a->xmin <= b->xmax && a->ymax >= b->ymin && a->ymin <= b->ymax)
	{
		crash_flag = 1;
	}
}

//显示游戏界面
void ShowUI_DinoGame(void)
{
	OLED_Clear();

	ShowGround();
	ShowDino();
	ShowBarrier();
	ShowCloud();
	ShowScore();
	ShowFrames();

	OLED_ShowImage(0, 0, 16, 16, Return);//显示返回图标
	OLED_ReverseArea(0, 0, 16, 16);

	OLED_Update();

	Crash_Judge(&dino, &barrier);//碰撞判断
}

void DinoGame_Init(void)
{
	xTimerStart(DinoTimerHandle, 0);//启动小恐龙游戏定时器
	Ground_Pos = Barrier_Pos = Cloud_Pos = Jump_Pos = Score = 0;
}

//游戏任务
uint8_t Game_Dino(void)
{
	DinoGame_Init();
	uint8_t Key = 0;
	while(1)
	{
		Key = Key_GetNum();
		if(Key == KEY_NEXT)
		{
			Dino_Jump_Flag = 1;
		}
		else if(Key == KEY_CONFIRM)
		{
			return 0;
		}
		ShowUI_DinoGame();
		if(crash_flag)
		{
			OLED_Clear();
			OLED_ShowString(24, 24, "Game Over", OLED_8X16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(1000));
			crash_flag = 0;
			OLED_Clear();
			ShowFrames();
			OLED_Update();
			xTimerStop(DinoTimerHandle, 0);//停止小恐龙游戏定时器
			return 0;
		}
		if(Game_flag)
		{
			OLED_Clear();
			OLED_ShowString(24, 24, "Game Pass", OLED_8X16);
			ShowFrames();
			OLED_Update();
			vTaskDelay(pdMS_TO_TICKS(1000));
			Game_flag = 0;
			OLED_Clear();
			ShowFrames();
			OLED_Update();
			xTimerStop(DinoTimerHandle, 0);//停止小恐龙游戏定时器
			return 0;
		}

		vTaskDelay(pdMS_TO_TICKS(10)); // 让出CPU，避免饿死软件定时器服务任务
	}
}

//每10ms调用一次，用于定时，延时
void Dino_Tick(TimerHandle_t xTimer)
{
	static uint8_t Ground_cnt, Cloud_cnt, Score_cnt;
	Ground_cnt++;
	Cloud_cnt++;
	Score_cnt++;
	if(Score_cnt >= 10)
	{
		Score_cnt = 0;
		Score++;
		if(Score >= 65535)
		{
			Score = 0;
			Game_flag = 1;
		}
	}
	if(Ground_cnt >= 2)
	{
		Ground_cnt = 0;
		Ground_Pos++;
		Barrier_Pos++;
		if(Ground_Pos >= 256)
		{
			Ground_Pos = 0;
		}
		if(Barrier_Pos >= 136)
		{
			Barrier_Pos = 0;
		}
	}
	if(Cloud_cnt >= 20)
	{
		Cloud_cnt = 0;
		Cloud_Pos++;
		if(Cloud_Pos > 160)
		{
			Cloud_Pos = 0;
		}
		if(Cloud_Pos % 2 == 0)
		{
			Dino_Turn = 0;
		}
		else
		{
			Dino_Turn = 1;
		}
	}
	if(Dino_Jump_Flag)
	{
		Jump_cnt++;
		if(Jump_cnt > 100)
		{
			Jump_cnt = 0;
			Dino_Jump_Flag = 0;
		}
	}
}
