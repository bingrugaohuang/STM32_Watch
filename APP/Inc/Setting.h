#ifndef _SETTING_H_
#define _SETTING_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "SetTime.h"
#include "ShowFrames.h"
#include "TranAnime.h"
#include "menu.h"
#include "Set_StackMonitor.h"
#include "ScreenBright.h"

extern TaskHandle_t StackMonitorTaskHandle;//栈监控任务句柄

void Setting(void);
	
#endif

