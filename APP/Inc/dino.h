#ifndef _DINO_H
#define _DINO_H

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "math.h"
#include "Timers.h"
#include "stdlib.h"
#include "ShowFrames.h"

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t GameTaskHandle;
extern TimerHandle_t DinoTimerHandle;//软件定时器句柄

uint8_t Game_Dino(void);
void Dino_Tick(TimerHandle_t xTimer);

#endif
