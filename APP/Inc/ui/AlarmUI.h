#ifndef _ALARMUI_H
#define _ALARMUI_H

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "Key.h"
#include "modules/ShowFrames.h"
#include "FreeRTOS.h"
#include "services/AlarmService.h"
#include "services/SleepManager.h"

uint8_t AlarmUI_ShowSettingPage(void);
AlarmAction_t AlarmUI_ShowRingingPage(const AlarmConfig_t *cfg);


#endif /* _ALARMUI_H */
