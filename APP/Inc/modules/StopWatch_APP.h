#ifndef __STOPWATCH_APP_H
#define __STOPWATCH_APP_H

#include "app_handles.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Key.h"
#include "ShowFrames.h"

uint8_t Show_StopWatch(void);
void StopWatch_Tick(TimerHandle_t xTimer);


#endif
