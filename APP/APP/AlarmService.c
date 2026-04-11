// #include "AlarmService.h"

// #include "AlarmUI.h"
// #include "menu.h"
// #include "SleepManager.h"
// #include <string.h>

// #define ALARM_BKP_MAGIC       0xA55AU
// #define ALARM_BKP_MAGIC_DR    RTC_BKP_DR2
// #define ALARM_BKP_CFG0_DR     RTC_BKP_DR3
// #define ALARM_BKP_CFG1_DR     RTC_BKP_DR4
// #define ALARM_BKP_CRC_DR      RTC_BKP_DR5

// static AlarmConfig_t gAlarmCfg = {1U, 7U, 0U, 5U, ALARM_REPEAT_DAILY};

// static volatile uint8_t gAlarmRinging = 0U;
// static volatile uint8_t gAlarmPendingIrq = 0U;
// static uint8_t gSnoozeArmed = 0U;

// /*
//  * 函数功能：约束配置项到合法范围
//  * 入口参数：cfg 闹钟配置指针
//  * 返回值  ：无
//  */
// static void Alarm_ClampConfig(AlarmConfig_t *cfg)
// {
// 	if(cfg == NULL) return;

// 	if(cfg->Hour > 23U) cfg->Hour = 23U;
// 	if(cfg->Minute > 59U) cfg->Minute = 59U;
// 	if(cfg->SnoozeMinutes < 1U) cfg->SnoozeMinutes = 5U;
// 	if(cfg->SnoozeMinutes > 30U) cfg->SnoozeMinutes = 30U;
// 	cfg->Enabled = (cfg->Enabled != 0U) ? 1U : 0U;
// 	cfg->Repeat = (cfg->Repeat == ALARM_REPEAT_DAILY) ? ALARM_REPEAT_DAILY : ALARM_REPEAT_ONCE;
// }

// /*
//  * 函数功能：将配置打包到BKP寄存器字段
//  * 入口参数：cfg 闹钟配置
//  * 返回值  ：cfg0/cfg1 两个16位打包值
//  */
// static void Alarm_PackConfig(const AlarmConfig_t *cfg, uint16_t *cfg0, uint16_t *cfg1)
// {
// 	if(cfg0 != NULL)
// 	{
// 		*cfg0 = (uint16_t)(cfg->Enabled & 0x01U);
// 		*cfg0 |= (uint16_t)(((uint16_t)cfg->Repeat & 0x01U) << 1);
// 		*cfg0 |= (uint16_t)(((uint16_t)cfg->SnoozeMinutes & 0x3FU) << 2);
// 	}
// 	if(cfg1 != NULL)
// 	{
// 		*cfg1 = (uint16_t)cfg->Hour;
// 		*cfg1 |= (uint16_t)(((uint16_t)cfg->Minute) << 8);
// 	}
// }

// /*
//  * 函数功能：从BKP打包字段解码配置
//  * 入口参数：cfg0/cfg1 两个16位打包值
//  * 返回值  ：解码后的AlarmConfig_t
//  */
// static AlarmConfig_t Alarm_UnpackConfig(uint16_t cfg0, uint16_t cfg1)
// {
// 	AlarmConfig_t cfg;
// 	cfg.Enabled = (uint8_t)(cfg0 & 0x01U);
// 	cfg.Repeat = (AlarmRepeat_t)((cfg0 >> 1) & 0x01U);
// 	cfg.SnoozeMinutes = (uint8_t)((cfg0 >> 2) & 0x3FU);
// 	cfg.Hour = (uint8_t)(cfg1 & 0x00FFU);
// 	cfg.Minute = (uint8_t)((cfg1 >> 8) & 0x00FFU);
// 	Alarm_ClampConfig(&cfg);
// 	return cfg;
// }

// /*
//  * 函数功能：保存闹钟配置到备份寄存器
//  * 入口参数：无（使用全局配置）
//  * 返回值  ：无
//  */
// static void Alarm_SaveToBackup(void)
// {
// 	uint16_t cfg0 = 0U;
// 	uint16_t cfg1 = 0U;
// 	uint16_t crc = 0U;

// 	Alarm_PackConfig(&gAlarmCfg, &cfg0, &cfg1);
// 	crc = (uint16_t)(cfg0 ^ cfg1 ^ 0x5A5AU);

// 	HAL_RTCEx_BKUPWrite(&hrtc, ALARM_BKP_MAGIC_DR, ALARM_BKP_MAGIC);
// 	HAL_RTCEx_BKUPWrite(&hrtc, ALARM_BKP_CFG0_DR, cfg0);
// 	HAL_RTCEx_BKUPWrite(&hrtc, ALARM_BKP_CFG1_DR, cfg1);
// 	HAL_RTCEx_BKUPWrite(&hrtc, ALARM_BKP_CRC_DR, crc);
// }

// /*
//  * 函数功能：从备份寄存器加载闹钟配置
//  * 入口参数：无
//  * 返回值  ：1 成功，0 无有效配置
//  */
// static uint8_t Alarm_LoadFromBackup(void)
// {
// 	uint16_t magic = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, ALARM_BKP_MAGIC_DR);
// 	uint16_t cfg0 = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, ALARM_BKP_CFG0_DR);
// 	uint16_t cfg1 = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, ALARM_BKP_CFG1_DR);
// 	uint16_t crc = (uint16_t)HAL_RTCEx_BKUPRead(&hrtc, ALARM_BKP_CRC_DR);

// 	if(magic != ALARM_BKP_MAGIC) return 0U;
// 	if((uint16_t)(cfg0 ^ cfg1 ^ 0x5A5AU) != crc) return 0U;

// 	gAlarmCfg = Alarm_UnpackConfig(cfg0, cfg1);
// 	return 1U;
// }

// /*
//  * 函数功能：根据“当前时分 + 指定分钟”计算未来时间
//  * 入口参数：addMinutes 需要追加的分钟数
//  * 返回值  ：outHour/outMinute 计算结果
//  */
// static void Alarm_CalcAfterMinutes(uint16_t addMinutes, uint8_t *outHour, uint8_t *outMinute)
// {
// 	RTC_TimeTypeDef nowTime;
// 	RTC_DateTypeDef nowDate;
// 	uint32_t total = 0U;

// 	HAL_RTC_GetTime(&hrtc, &nowTime, RTC_FORMAT_BIN);
// 	HAL_RTC_GetDate(&hrtc, &nowDate, RTC_FORMAT_BIN);

// 	total = (uint32_t)nowTime.Hours * 60U + (uint32_t)nowTime.Minutes + (uint32_t)addMinutes;
// 	if(outHour != NULL) *outHour = (uint8_t)((total / 60U) % 24U);
// 	if(outMinute != NULL) *outMinute = (uint8_t)(total % 60U);
// }

// /*
//  * 函数功能：将闹钟时间写入RTC硬件（中断模式）
//  * 入口参数：hour/minute 目标触发时分
//  * 返回值  ：无
//  */
// static void Alarm_SetHardwareAlarm(uint8_t hour, uint8_t minute)
// {
// 	RTC_AlarmTypeDef alarm;

// 	memset(&alarm, 0, sizeof(alarm));
//	alarm.Alarm = RTC_ALARM_A;
// 	alarm.AlarmTime.Hours = hour;
// 	alarm.AlarmTime.Minutes = minute;
// 	alarm.AlarmTime.Seconds = 0;

// 	HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
// 	if(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
// 	{
// 		Error_Handler();
// 	}
// }

// /*
//  * 函数功能：按照当前配置重排下一次闹钟触发
//  * 入口参数：无（使用全局配置及贪睡状态）
//  * 返回值  ：无
//  */
// static void Alarm_RebuildNextTrigger(void)
// {
// 	uint8_t targetHour = 0U;
// 	uint8_t targetMinute = 0U;

// 	if(gAlarmCfg.Enabled == 0U)
// 	{
// 		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
// 		return;
// 	}

// 	if(gSnoozeArmed != 0U)
// 	{
// 		Alarm_CalcAfterMinutes(gAlarmCfg.SnoozeMinutes, &targetHour, &targetMinute);
// 		Alarm_SetHardwareAlarm(targetHour, targetMinute);
// 		return;
// 	}
    
// 	targetHour = gAlarmCfg.Hour;
// 	targetMinute = gAlarmCfg.Minute;
// 	Alarm_SetHardwareAlarm(targetHour, targetMinute);
// }

// /*
//  * 函数功能：挂起前台任务并记录掩码，提醒结束后恢复
//  * 入口参数：无
//  * 返回值  ：挂起掩码
//  */
// static uint32_t Alarm_SuspendForegroundTasks(void)
// {
// 	uint32_t mask = 0U;

// 	/*
// 	 * 关键步骤：先拿OLED递归锁，再挂起前台任务。
// 	 * 目的：避免任务在持有OLED锁时被挂起，导致闹钟页后续刷新永久阻塞。
// 	 */
// 	OLED_I2C_Lock();

// 	if(UITaskHandle != NULL && eTaskGetState(UITaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(UITaskHandle);
// 		mask |= (1U << 0);
// 	}
// 	if(MenuTaskHandle != NULL && eTaskGetState(MenuTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(MenuTaskHandle);
// 		mask |= (1U << 1);
// 	}
// 	if(SetTaskHandle != NULL && eTaskGetState(SetTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(SetTaskHandle);
// 		mask |= (1U << 2);
// 	}
// 	if(TimeTaskHandle != NULL && eTaskGetState(TimeTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(TimeTaskHandle);
// 		mask |= (1U << 3);
// 	}
// 	if(FlashlightTaskHandle != NULL && eTaskGetState(FlashlightTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(FlashlightTaskHandle);
// 		mask |= (1U << 4);
// 	}
// 	if(MPU6050TaskHandle != NULL && eTaskGetState(MPU6050TaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(MPU6050TaskHandle);
// 		mask |= (1U << 5);
// 	}
// 	if(GameTaskHandle != NULL && eTaskGetState(GameTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(GameTaskHandle);
// 		mask |= (1U << 6);
// 	}
// 	if(StackMonitorTaskHandle != NULL && eTaskGetState(StackMonitorTaskHandle) != eSuspended)
// 	{
// 		vTaskSuspend(StackMonitorTaskHandle);
// 		mask |= (1U << 7);
// 	}

// 	/*
// 	 * 到这里前台显示任务都已挂起，释放OLED锁。
// 	 * 后续Alarm任务可正常调用OLED接口显示提醒页。
// 	 */
// 	OLED_I2C_Unlock();

// 	return mask;
// }

// /*
//  * 函数功能：按掩码恢复前台任务
//  * 入口参数：mask 挂起掩码
//  * 返回值  ：无
//  */
// static void Alarm_ResumeForegroundTasks(uint32_t mask)
// {
// 	if((mask & (1U << 0)) && UITaskHandle != NULL) vTaskResume(UITaskHandle);
// 	if((mask & (1U << 1)) && MenuTaskHandle != NULL) vTaskResume(MenuTaskHandle);
// 	if((mask & (1U << 2)) && SetTaskHandle != NULL) vTaskResume(SetTaskHandle);
// 	if((mask & (1U << 3)) && TimeTaskHandle != NULL) vTaskResume(TimeTaskHandle);
// 	if((mask & (1U << 4)) && FlashlightTaskHandle != NULL) vTaskResume(FlashlightTaskHandle);
// 	if((mask & (1U << 5)) && MPU6050TaskHandle != NULL) vTaskResume(MPU6050TaskHandle);
// 	if((mask & (1U << 6)) && GameTaskHandle != NULL) vTaskResume(GameTaskHandle);
// 	if((mask & (1U << 7)) && StackMonitorTaskHandle != NULL) vTaskResume(StackMonitorTaskHandle);

// 	if(mask == 0U && UITaskHandle != NULL)
// 	{
// 		vTaskResume(UITaskHandle);
// 	}
// }

// /*
//  * 函数功能：闹钟服务初始化（加载配置并排程）
//  * 入口参数：无
//  * 返回值  ：无
//  */
// void Alarm_ServiceInit(void)
// {
// 	if(Alarm_LoadFromBackup() == 0U)
// 	{
// 		Alarm_ClampConfig(&gAlarmCfg);
// 		Alarm_SaveToBackup();
// 	}

// 	gAlarmPendingIrq = 0U;
// 	gAlarmRinging = 0U;
// 	gSnoozeArmed = 0U;
// 	Alarm_RebuildNextTrigger();
// }

// /*
//  * 函数功能：获取当前闹钟配置
//  * 入口参数：cfg 输出配置指针
//  * 返回值  ：无
//  */
// void Alarm_ServiceGetConfig(AlarmConfig_t *cfg)
// {
// 	if(cfg == NULL) return;
// 	*cfg = gAlarmCfg;
// }

// /*
//  * 函数功能：设置闹钟配置并立即生效
//  * 入口参数：cfg 输入配置指针
//  * 返回值  ：无
//  */
// void Alarm_ServiceSetConfig(const AlarmConfig_t *cfg)
// {
// 	if(cfg == NULL) return;
// 	gAlarmCfg = *cfg;
// 	Alarm_ClampConfig(&gAlarmCfg);
// 	gSnoozeArmed = 0U;
// 	Alarm_SaveToBackup();
// 	Alarm_RebuildNextTrigger();
// }

// /*
//  * 函数功能：开关闹钟
//  * 入口参数：enable 0关闭，1开启
//  * 返回值  ：无
//  */
// void Alarm_ServiceEnable(uint8_t enable)
// {
// 	gAlarmCfg.Enabled = (enable != 0U) ? 1U : 0U;
// 	gSnoozeArmed = 0U;
// 	Alarm_SaveToBackup();
// 	Alarm_RebuildNextTrigger();
// }

// /*
//  * 函数功能：获取闹钟是否开启
//  * 入口参数：无
//  * 返回值  ：1开启，0关闭
//  */
// uint8_t Alarm_ServiceIsEnabled(void)
// {
// 	return gAlarmCfg.Enabled;
// }

// /*
//  * 函数功能：RTC Alarm中断入口（供ISR回调调用）
//  * 入口参数：无
//  * 返回值  ：无
//  */
// void Alarm_ServiceOnRtcAlarmFromISR(void)
// {
// 	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
// 	gAlarmPendingIrq = 1U;
// 	SleepMgr_ReportWakeupFromISR(0xFFFFU);

// 	/*
// 	 * 说明：仅在调度器运行后才使用FromISR通知，
// 	 * 防止闹钟在系统启动早期触发导致内核状态异常。
// 	 */
// 	if((AlarmTaskHandle != NULL) && (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING))
// 	{
// 		xTaskNotifyFromISR(AlarmTaskHandle, 1U, eSetBits, &xHigherPriorityTaskWoken);
// 		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// 	}
// }

// /*
//  * 函数功能：闹钟动作处理（停止/贪睡）
//  * 入口参数：action 用户动作
//  * 返回值  ：无
//  */
// void Alarm_ServiceApplyAction(AlarmAction_t action)
// {
// 	if(action == ALARM_ACTION_SNOOZE)
// 	{
// 		gSnoozeArmed = 1U;
// 		Alarm_RebuildNextTrigger();
// 		return;
// 	}
//     else
// 	{   /*
//         * 每日重复：
//         * 1) 普通提醒被停止时，不重新下发，保持硬件下一日匹配；
//         * 2) 贪睡提醒被停止时，需要恢复到常规闹钟时间并重排。
//         */
//         if(gAlarmCfg.Repeat == ALARM_REPEAT_ONCE)
//         {
//             gAlarmCfg.Enabled = 0U;
//             Alarm_SaveToBackup();
//             Alarm_RebuildNextTrigger();
//             return;
//         }

//         if(gSnoozeArmed != 0U)
//         {
//             gSnoozeArmed = 0U;
//             Alarm_RebuildNextTrigger();
//         }
// 	}
// }

// /*
//  * 函数功能：查询当前是否处于响铃态
//  * 入口参数：无
//  * 返回值  ：1响铃中，0非响铃
//  */
// uint8_t Alarm_ServiceIsRinging(void)
// {
// 	return gAlarmRinging;
// }

// /*
//  * 函数功能：调试接口，主动触发一次提醒
//  * 入口参数：无
//  * 返回值  ：无
//  */
// void Alarm_ServiceDebugForceWake(void)
// {
// 	gAlarmPendingIrq = 1U;
// 	if(AlarmTaskHandle != NULL)
// 	{
// 		xTaskNotify(AlarmTaskHandle, 1U, eSetBits);
// 	}
// }

// /*
//  * 函数功能：闹钟任务，等待中断通知后拉起最小提醒页
//  * 入口参数：argument 任务参数（未使用）
//  * 返回值  ：无
//  */
// void Alarm_ServiceTask(void *argument)
// {
// 	uint32_t notifyValue = 0U;
// 	(void)argument;

// 	while(1)
// 	{
//		xTaskNotifyWait(0xFFFFFFFFU, 0xFFFFFFFFU, &notifyValue, portMAX_DELAY);
        
// 		if(gAlarmPendingIrq == 0U) continue;
// 		gAlarmPendingIrq = 0U;

// 		if(gAlarmCfg.Enabled == 0U) continue;

// 		SleepMgr_ReportActivity();
// 		gAlarmRinging = 1U;

		
// 		uint32_t mask = Alarm_SuspendForegroundTasks();
// 		AlarmAction_t action = AlarmUI_ShowRingingPage(&gAlarmCfg);
// 		Alarm_ServiceApplyAction(action);
// 		Alarm_ResumeForegroundTasks(mask);
		

// 		gAlarmRinging = 0U;
// 	}
// }

// /*
//  * 函数功能：RTC Alarm HAL弱回调重写，统一转发到服务层
//  * 入口参数：hrtc RTC句柄
//  * 返回值  ：无
//  */
// void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
// {
// 	(void)hrtc;
// 	Alarm_ServiceOnRtcAlarmFromISR();
// }

/*
 * 函数功能：闹钟任务，等待中断通知后拉起最小提醒页
 * 入口参数：argument 任务参数（未使用）
 * 返回值  ：无
 * */
#include "AlarmService.h"
#include "AlarmUI.h"
#include "common/app_handles.h"
#include "SleepManager.h"
#include <string.h>

static AlarmConfig_t gAlarmCfg = {0U, 0U, 0U, 1U, ALARM_REPEAT_DAILY};
//static AlarmConfig_t gAlarmSnzCfg = {0U};

static volatile uint8_t gAlarmPendingIRQ = 0U;
static volatile uint8_t gSnoozeArmed = 0U;
static volatile uint8_t gAlarmRinging = 0U;

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
 * 函数功能：如果任务句柄有效且未挂起，则挂起任务
 * 入口参数：xTaskHandle 任务句柄
 * 返回值  ：1U 已挂起，0U 无需挂起
 */
static uint8_t SuspendIfNeeded(TaskHandle_t xTaskHandle)
{
	if(xTaskHandle != NULL && eTaskGetState(xTaskHandle) != eSuspended)
	{
		OLED_I2C_Lock(); // 先拿OLED递归锁，避免挂起时持有锁导致死锁
		vTaskSuspend(xTaskHandle);
		OLED_I2C_Unlock();
		return 1U;
	}
	return 0U;
}

/*
 * 函数功能：如果任务句柄有效且处于挂起状态，则恢复任务
 * 入口参数：xTaskHandle 任务句柄
 * 返回值  ：无
 */
static void ResumeIfNeeded(TaskHandle_t xTaskHandle)
{
	if(xTaskHandle != NULL && eTaskGetState(xTaskHandle) == eSuspended)
	{
		vTaskResume(xTaskHandle);
	}
}

/*
 * 函数功能：挂起前台任务
 * 入口参数：无
 * 返回值  ：挂起任务的掩码
 */
static uint32_t Alarm_SuspendForegroundTasks(void)
{
	uint32_t mark = 0U;

	if(SuspendIfNeeded(UITaskHandle))           mark |= (1U << 0);
	if(SuspendIfNeeded(MenuTaskHandle))  	    mark |= (1U << 1);
	if(SuspendIfNeeded(SetTaskHandle))          mark |= (1U << 2);
	if(SuspendIfNeeded(TimeTaskHandle))         mark |= (1U << 3);
	if(SuspendIfNeeded(FlashlightTaskHandle))   mark |= (1U << 4);
	if(SuspendIfNeeded(MPU6050TaskHandle))      mark |= (1U << 5);
	if(SuspendIfNeeded(GameTaskHandle))         mark |= (1U << 6);
	if(SuspendIfNeeded(StackMonitorTaskHandle)) mark |= (1U << 7);
	return mark;
}

/*
 * 函数功能：按掩码恢复前台任务
 * 入口参数：mask 挂起掩码
 * 返回值  ：无
 */
static void Alarm_ResumeForegroundTasks(uint32_t mark)
{
	if(mark & (1U << 0)) ResumeIfNeeded(UITaskHandle);
	if(mark & (1U << 1)) ResumeIfNeeded(MenuTaskHandle);
	if(mark & (1U << 2)) ResumeIfNeeded(SetTaskHandle);
	if(mark & (1U << 3)) ResumeIfNeeded(TimeTaskHandle);
	if(mark & (1U << 4)) ResumeIfNeeded(FlashlightTaskHandle);
	if(mark & (1U << 5)) ResumeIfNeeded(MPU6050TaskHandle);
	if(mark & (1U << 6)) ResumeIfNeeded(GameTaskHandle);
	if(mark & (1U << 7)) ResumeIfNeeded(StackMonitorTaskHandle);

	if(mark == 0U)       ResumeIfNeeded(UITaskHandle);
}

/*
 * 函数功能：设置硬件闹钟
 * 入口参数：hour 小时
 *          minute 分钟
 * 返回值  ：无
 */
static void Alarm_SetHardwareAlarm(uint8_t hour, uint8_t minute)
{
	RTC_AlarmTypeDef alarm;
	memset(&alarm, 0, sizeof(alarm));

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
	if(cfg == NULL) return;
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
	if(cfg == NULL) return;
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
		uint32_t mark = Alarm_SuspendForegroundTasks();
		AlarmAction_t action = AlarmUI_ShowRingingPage(&gAlarmCfg);
		Alarm_ApplyAction(action);
		Alarm_ResumeForegroundTasks(mark);
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
