#ifndef _SHOWMENU_H_
#define _SHOWMENU_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "ShowFrames.h"

#define ANIME_SPEED 6//动画速度，数值越大动画越快

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t UITaskHandle;
extern TaskHandle_t TimeTaskHandle;
extern TaskHandle_t FlashlightTaskHandle;
extern TaskHandle_t MPU6050TaskHandle;
extern TaskHandle_t GameTaskHandle;


void ShowMenu(void);

#endif
