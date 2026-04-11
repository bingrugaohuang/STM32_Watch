#ifndef _ALARM_SERVICE_H_
#define _ALARM_SERVICE_H_

#include "main.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"

extern TaskHandle_t AlarmTaskHandle;

typedef enum
{
	ALARM_REPEAT_ONCE = 0,
	ALARM_REPEAT_DAILY = 1
} AlarmRepeat_t;

typedef struct 
{
	uint8_t Enabled;        // 闹钟使能：0关闭，1开启
	uint8_t Hour;			// 闹钟小时：0~23
	uint8_t Min;            // 闹钟分钟：0~59
	uint8_t SnoozeMin;      // 贪睡分钟：1~30
	AlarmRepeat_t Repeat;   // 重复模式：单次/每日
}AlarmConfig_t;

typedef enum
{
	ALARM_ACTION_STOP = 0,
	ALARM_ACTION_SNOOZE = 1
}AlarmAction_t;

void Alarm_ServiceInit(void);
void Alarm_ServiceGetConfig(AlarmConfig_t *cfg);
uint8_t Alarm_ServiceIsRinging(void);
void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg);
void Alarm_ServiceTask(void *argument);

// typedef enum
// {
// 	ALARM_REPEAT_ONCE = 0,   // 单次提醒
// 	ALARM_REPEAT_DAILY = 1   // 每日重复
// } AlarmRepeat_t;

// typedef struct
// {
// 	uint8_t Enabled;         // 闹钟使能：0关闭，1开启
// 	uint8_t Hour;            // 闹钟小时：0~23
// 	uint8_t Minute;          // 闹钟分钟：0~59
// 	uint8_t SnoozeMinutes;   // 贪睡分钟：1~30
// 	AlarmRepeat_t Repeat;    // 重复模式：单次/每日
// } AlarmConfig_t;

// typedef enum
// {
// 	ALARM_ACTION_STOP = 0,   // 停止提醒
// 	ALARM_ACTION_SNOOZE = 1  // 贪睡提醒
// } AlarmAction_t;

// void Alarm_ServiceInit(void);
// void Alarm_ServiceGetConfig(AlarmConfig_t *cfg);
// void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg);

// void Alarm_ServiceEnable(uint8_t enable);
// uint8_t Alarm_ServiceIsEnabled(void);

// void Alarm_ServiceTask(void *argument);
// void Alarm_ServiceOnRtcAlarmFromISR(void);

// void Alarm_ServiceApplyAction(AlarmAction_t action);
// uint8_t Alarm_ServiceIsRinging(void);

// void Alarm_ServiceDebugForceWake(void);


#endif /* _ALARM_SERVICE_H_ */
