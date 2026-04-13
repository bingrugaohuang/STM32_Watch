#ifndef _DINO_H
#define _DINO_H

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "Key.h"
#include <math.h>
#include "timers.h"
#include <stdlib.h>
#include "ShowFrames.h"
#include "app_handles.h"

uint8_t Game_Dino(void);
void Dino_Tick(TimerHandle_t xTimer);

#endif
