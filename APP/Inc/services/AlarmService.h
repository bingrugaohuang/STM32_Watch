#ifndef _ALARM_SERVICE_H_
#define _ALARM_SERVICE_H_

#include "main.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_handles.h"
#include "app_types.h"

void Alarm_ServiceInit(void);
void Alarm_ServiceGetConfig(AlarmConfig_t *cfg);
uint8_t Alarm_ServiceIsRinging(void);
void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg);
void Alarm_ServiceTask(void *argument);

// typedef enum
// {
// 	ALARM_REPEAT_ONCE = 0,   // ��������
// 	ALARM_REPEAT_DAILY = 1   // ÿ���ظ�
// } AlarmRepeat_t;

// typedef struct
// {
// 	uint8_t Enabled;         // ����ʹ�ܣ�0�رգ�1����
// 	uint8_t Hour;            // ����Сʱ��0~23
// 	uint8_t Minute;          // ���ӷ��ӣ�0~59
// 	uint8_t SnoozeMinutes;   // ̰˯���ӣ�1~30
// 	AlarmRepeat_t Repeat;    // �ظ�ģʽ������/ÿ��
// } AlarmConfig_t;

// typedef enum
// {
// 	ALARM_ACTION_STOP = 0,   // ֹͣ����
// 	ALARM_ACTION_SNOOZE = 1  // ̰˯����
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
