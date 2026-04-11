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
#include "common/app_handles.h"

uint8_t Game_Dino(void);
void Dino_Tick(TimerHandle_t xTimer);

#endif
