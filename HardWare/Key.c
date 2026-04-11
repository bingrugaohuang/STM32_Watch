#include "Key.h"

#define KEY_LONGPRESS_TICKS 100U  // 长按判定阈值：10ms * 100 = 1s

#define KEY_REPEAT_TICKS    10U   // 长按后连发周期：10ms * 10 = 100ms

static volatile uint16_t Key_Cnt = 0;     // 当前按键持续按下计数，单位10ms
static volatile uint8_t Key_LongSent = 0; // 是否已发送过长按事件：0未发送，1已发送
static volatile uint8_t Key_Active = 0;   // 当前处于长按检测中的按键，0表示无
// static uint8_t KeyNum = 0; // 历史调试变量，保留注释

// 读取指定按键当前是否按下（低电平有效）
static uint8_t Key_IsPressed(uint8_t Key)
{
	switch(Key)
	{
		case KEY_CONFIRM:
			return HAL_GPIO_ReadPin(GPIOA, KEY_CONFIRM_Pin) == 0;
		case KEY_NEXT:
			return HAL_GPIO_ReadPin(GPIOA, KEY_NEXT_Pin) == 0;
		case KEY_LAST:
			return HAL_GPIO_ReadPin(GPIOB, KEY_LAST_Pin) == 0;
		default:
			return 0;
	}
}

// 将短按键值映射为对应的长按键值
static uint8_t Key_IsLongPressed(uint8_t Key)
{
	switch(Key)
	{
		case KEY_CONFIRM:
			return KEY_CONFIRM_LONG;
		case KEY_NEXT:
			return KEY_NEXT_LONG;
		case KEY_LAST:
			return KEY_LAST_LONG;
		default:
			return 0;
	}
}

// 从按键队列获取一个按键值（非阻塞）
uint8_t Key_GetNum(void)
{
  uint8_t Key_Num = 0;
  if(xQueueReceive(xKeyQueue, &Key_Num, 0) == pdPASS){
	SleepMgr_ReportActivity(); // 上报用户活动，刷新休眠计时
    return Key_Num;
  }else
    return 0;
}

// 初始化长按检测状态并启动10ms扫描定时器
static void LongPressed_Init(uint8_t Key)
{
	Key_Active = Key;
	Key_Cnt = 0;
	Key_LongSent = 0;
	xTimerStart(KeyTimerHandle, 0);
}

// 退出长按检测并停止扫描定时器
static void LongPressed_Exit(TimerHandle_t xTimer)
{
	Key_Cnt = 0;
	Key_LongSent = 0;
	Key_Active = 0;
	xTimerStop(xTimer, 0);
}

// 按键任务：等待中断通知并启动对应按键的长按检测
void Key(void)
{
	uint32_t KeyVal = 0;
	xKeyQueue = xQueueCreate(10, sizeof(uint8_t)); // 创建按键事件队列：长度10，元素类型uint8_t
	while(1)
	{
		xTaskNotifyWait(0x07, 0x07, &KeyVal, portMAX_DELAY); // 等待EXTI通知，按位记录到KeyVal
		vTaskDelay(pdMS_TO_TICKS(20)); // 简单消抖
		if(KeyVal & (1 << 0)) // 确认键
		{
			if(Key_IsPressed(KEY_CONFIRM)){
				if(Key_Active == 0){
					LongPressed_Init(KEY_CONFIRM);
				}
			}
		}
		else if(KeyVal & (1 << 1)) // 下一个键
		{
			if(Key_IsPressed(KEY_NEXT)){
				if(Key_Active == 0){
					LongPressed_Init(KEY_NEXT);
				}
			}
		}
		else if(KeyVal & (1 << 2)) // 上一个键
		{
			if(Key_IsPressed(KEY_LAST)){
				if(Key_Active == 0){
					LongPressed_Init(KEY_LAST);
				}		
			}	
		}
    } 
}

/* 注意：软件定时器回调运行在 Timer Service 任务中，禁止在此处调用阻塞式 API。 */
// 软件定时器回调：处理长按判定与连发上报
void Key_Tick(TimerHandle_t xTimer)
{
	uint8_t KeyNum = 0; // 待发送到队列的按键值
	uint8_t Key_Active_Copy = Key_Active; // 拷贝活动按键，避免并发修改影响本轮判断

	if(Key_Active == 0){
		xTimerStop(xTimer, 0);
		return;
	}

	// 按键仍处于按下状态：累计计数并在长按后按周期连发
	if(Key_IsPressed(Key_Active_Copy)){
		if(Key_Cnt < 0xFFFFU){
			Key_Cnt++;
		}
		// 达到长按阈值后，每100ms发送一次长按事件
		if((Key_Cnt >= KEY_LONGPRESS_TICKS) &&
		   (((Key_Cnt - KEY_LONGPRESS_TICKS) % KEY_REPEAT_TICKS) == 0U)){
			KeyNum = Key_IsLongPressed(Key_Active_Copy);
			xQueueSend(xKeyQueue, &KeyNum, 0);
			Key_LongSent = 1U;
		}
	}else{
		if(Key_LongSent == 0U){ // 未触发长按时，补发一次短按事件
		KeyNum = Key_Active_Copy;
		xQueueSend(xKeyQueue, &KeyNum, 0);
		}
		LongPressed_Exit(xTimer);
	}
}

// GPIO 外部中断回调：将按键事件通知给按键任务
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  uint32_t NotifyNum = 0;

  if(GPIO_Pin == KEY_CONFIRM_Pin){
    NotifyNum = 1 << 0;
  }else if(GPIO_Pin == KEY_NEXT_Pin){
    NotifyNum = 1 << 1;
  }else if(GPIO_Pin == KEY_LAST_Pin){
    NotifyNum = 1 << 2;
  }else if(GPIO_Pin == MPU_INT_Pin){
    SleepMgr_ReportWakeupFromISR(GPIO_Pin);
    return;
  }else{
	return; // 非按键相关中断，直接返回
  }
  SleepMgr_ReportWakeupFromISR(GPIO_Pin); // 上报唤醒事件，刷新休眠计时

	// 休眠期间用于唤醒的首个按键只负责唤醒，不进入按键队列
	if(SleepMgr_ConsumeWakeKeyDiscardFlagFromISR() != 0U){
		return;
	}

  if(KeyTaskHandle != NULL){
    xTaskNotifyFromISR(KeyTaskHandle, NotifyNum, eSetBits, &xHigherPriorityTaskWoken);
  }
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // 如唤醒更高优先级任务则立即切换
}
