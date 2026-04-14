#ifndef _SHOWMENU_H_
#define _SHOWMENU_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "Key.h"
#include "ShowFrames.h"
#include "TaskMgr.h"

#define ANIME_SPEED 8//动画速度，数值越大动画越快
#define ANIME_NUM 5  //动画图标数量

void ShowMenu(void);

#endif
