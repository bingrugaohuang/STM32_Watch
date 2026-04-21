#include "AlarmService.h"

extern void OLED_I2C_Lock(void);
extern void OLED_I2C_Unlock(void);
extern void SleepMgr_ReportActivity(void);
extern AlarmAction_t AlarmUI_ShowRingingPage(const AlarmConfig_t *cfg);

static AlarmConfig_t gAlarmCfg = {0U, 0U, 0U, 1U, ALARM_REPEAT_DAILY};

static volatile uint8_t gAlarmPendingIRQ = 0U;
static volatile uint8_t gSnoozeArmed = 0U;
static volatile uint8_t gAlarmRinging = 0U;
static uint32_t mark = 0U;


/*
 * 函数功能：约束配置项到合法范围
 * 入口参数：cfg 闹钟配置指针
 * 返回值  ：无
 */
static void Alarm_ClampConfig(AlarmConfig_t *cfg)
{
	if(cfg == NULL) return;

	if(cfg->Hour > 23U) cfg->Hour = 23U;
	if(cfg->Min > 59U) cfg->Min = 59U;
	if(cfg->SnoozeMin < 1U) cfg->SnoozeMin = 5U;
	if(cfg->SnoozeMin > 30U) cfg->SnoozeMin = 30U;
	cfg->Enabled = (cfg->Enabled != 0U) ? 1U : 0U;
	cfg->Repeat = (cfg->Repeat == ALARM_REPEAT_DAILY) ? ALARM_REPEAT_DAILY : ALARM_REPEAT_ONCE;
}

/*
 * 函数功能：设置硬件闹钟
 * 入口参数：hour 小时
 *          minute 分钟
 * 返回值  ：无
 */
static void Alarm_SetHardwareAlarm(uint8_t hour, uint8_t minute)
{
	RTC_AlarmTypeDef alarm = {0};

	alarm.Alarm = RTC_ALARM_A;
	alarm.AlarmTime.Hours = hour;
	alarm.AlarmTime.Minutes = minute;
	alarm.AlarmTime.Seconds = 0;

	HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
	if(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}
}

/*
 * 函数功能：计算下一个触发时间
 * 入口参数：addMin 需要追加的分钟数
		  	outHour 输出目标小时指针
		  	outMin 输出目标分钟指针
 * 返回值  ：无
 */
static void Alarm_NextTriggerCaculation(uint8_t addMin, uint8_t* outHour, uint8_t* outMin)
{
	RTC_TimeTypeDef nowTime;
	RTC_DateTypeDef nowDate;
	uint32_t total = 0U;

	HAL_RTC_GetTime(&hrtc, &nowTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &nowDate, RTC_FORMAT_BIN);

	total = (uint32_t)nowTime.Hours * 60U + (uint32_t)nowTime.Minutes + (uint32_t)addMin;
	*outHour = (uint8_t)((total / 60U) % 24U);
	*outMin = (uint8_t)(total % 60U);
}

/*
 * 函数功能：设置下一个触发时间
 * 入口参数：无
 * 返回值  ：无
 */
static void Alarm_NextTrigger(void)
{
	uint8_t targetHour = 0U;
	uint8_t targetMinute = 0U;

	//ONCE模式下，按下stop或者闹钟未启用，关闭闹钟模块
	if(gAlarmCfg.Enabled == 0U)
	{
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
		return;
	}
	else if(gAlarmCfg.Enabled == 1U)
	{
		//若按下贪睡，则以当前时间为基准重新计算下一次触发时间
		if(gSnoozeArmed == 1U)
		{
			gSnoozeArmed = 0U;
			Alarm_NextTriggerCaculation(gAlarmCfg.SnoozeMin, &targetHour, &targetMinute);
			Alarm_SetHardwareAlarm(targetHour, targetMinute);
			return;
		}
		//Repeat模式下，按下stop直接配置下一次触发时间，无需计算
		Alarm_SetHardwareAlarm(gAlarmCfg.Hour, gAlarmCfg.Min);
	}
}


/*
 * 函数功能：应用闹钟动作
 * 入口参数：action 动作类型
 * 返回值  ：无
 */
static void Alarm_ApplyAction(AlarmAction_t action)
{
	if(action == ALARM_ACTION_SNOOZE)
	{
		//贪睡可叠加
		gSnoozeArmed = 1U;
		Alarm_NextTrigger();
		return;
	}
	//ONCE模式下按下stop关闭闹钟模块，其余流程一致
	//先清0标志位,再重排下一次触发
	else if(action == ALARM_ACTION_STOP)
	{
		if(gAlarmCfg.Repeat == ALARM_REPEAT_ONCE)
		{
			gAlarmCfg.Enabled = 0U;
		}
		gSnoozeArmed = 0U;
		//gAlarmSnzCfg = gAlarmCfg;
		Alarm_NextTrigger();
		return;
	}
}

/*
 * 函数功能：设置闹钟配置
 * 入口参数：cfg 闹钟配置指针
 * 返回值  ：无
 */
void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg)
{
	configASSERT(cfg != NULL);
	gAlarmCfg = *cfg;
	Alarm_ClampConfig(&gAlarmCfg);
	//配置变更后重置贪睡状态，确保新配置生效时不受旧贪睡状态影响
	gSnoozeArmed = 0U;
	Alarm_SetHardwareAlarm(gAlarmCfg.Hour, gAlarmCfg.Min);
}

/*
 * 函数功能：闹钟服务初始化（加载配置并排程）
 * 入口参数：无
 * 返回值  ：无
 */
void Alarm_ServiceInit(void)
{
	gAlarmPendingIRQ = 0U;
	gAlarmRinging = 0U;
	gSnoozeArmed = 0U;
	Alarm_NextTrigger();
}

/*
 * 函数功能：获取当前闹钟配置
 * 入口参数：cfg 输出配置指针
 * 返回值  ：无
 */
void Alarm_ServiceGetConfig(AlarmConfig_t *cfg)
{
	configASSERT(cfg != NULL);
	*cfg = gAlarmCfg;
}

/*
 * 函数功能：查询当前是否处于响铃态
 * 入口参数：无
 * 返回值  ：1响铃中，0非响铃
 */
uint8_t Alarm_ServiceIsRinging(void)
{
	return gAlarmRinging;
}

/*
 * 函数功能：调试接口，主动触发一次提醒
 * 入口参数：无
 * 返回值  ：无
 */
void Alarm_ServiceDebugForceWake(void)
{
	gAlarmPendingIRQ = 1U;
	if(AlarmTaskHandle != NULL)
	{
		xTaskNotify(AlarmTaskHandle, 1U, eSetBits);
	}
}

/*
 * 函数功能：在外部修改mark值
 * 入口参数：marktemp
 * 返回值  ：无
 */
void Alarm_SetMark(uint32_t marktemp)
{
	mark = marktemp;
}

/*
 *函数功能：闹钟任务，等待中断通知后拉起最小提醒页
 *入口参数：argument 任务参数（未使用）
 *返回值  ：无
 */
void Alarm_ServiceTask(void *argument)
{
	(void)argument;
	uint32_t notifyValue = 0U;
	while(1)
	{
		xTaskNotifyWait(0xFFFFFFFFU, 0xFFFFFFFFU, &notifyValue, portMAX_DELAY);
		if(gAlarmPendingIRQ == 0U) continue;
		gAlarmPendingIRQ = 0U;

		if(gAlarmCfg.Enabled == 0U) continue;

		SleepMgr_ReportActivity();

		gAlarmRinging = 1U;
		
		mark = SuspendForegroundTasks();
		AlarmAction_t action = AlarmUI_ShowRingingPage(&gAlarmCfg);
		Alarm_ApplyAction(action);
		ResumeForegroundTasks(mark);
		gAlarmRinging = 0U;
	}
}

/*
 * 函数功能：RTC_ALarm_IRQHandler中断回调，标记待处理状态并通知任务
 * 入口参数：hrtc RTC句柄
 * 返回值  ：无
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	(void)hrtc;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	gAlarmPendingIRQ = 1U;
	if(AlarmTaskHandle != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
	{
		xTaskNotifyFromISR(AlarmTaskHandle, 1U, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
