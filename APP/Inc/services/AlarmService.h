#ifndef _ALARM_SERVICE_H_
#define _ALARM_SERVICE_H_

#include "main.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_handles.h"
#include "app_types.h"
#include "TaskMgr.h"

void Alarm_ServiceInit(void);
void Alarm_ServiceGetConfig(AlarmConfig_t *cfg);
uint8_t Alarm_ServiceIsRinging(void);
void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg);
void Alarm_ServiceTask(void *argument);
void Alarm_SetMark(uint32_t marktemp);

#endif /* _ALARM_SERVICE_H_ */
