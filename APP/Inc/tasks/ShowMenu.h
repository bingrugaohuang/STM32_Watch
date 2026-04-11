#ifndef _SHOWMENU_H_
#define _SHOWMENU_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "Key.h"
#include "modules/ShowFrames.h"
#include "common/app_handles.h"

#define ANIME_SPEED 6//动画速度，数值越大动画越快


void ShowMenu(void);

#endif
