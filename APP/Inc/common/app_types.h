#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

#include "common/app_common.h"

typedef enum
{
ALARM_REPEAT_ONCE = 0,
ALARM_REPEAT_DAILY = 1
} AlarmRepeat_t;

typedef struct
{
uint8_t Enabled;
uint8_t Hour;
uint8_t Min;
uint8_t SnoozeMin;
AlarmRepeat_t Repeat;
} AlarmConfig_t;

typedef enum
{
ALARM_ACTION_STOP = 0,
ALARM_ACTION_SNOOZE = 1
} AlarmAction_t;

#endif
