#include "StackMonitor_Task.h"
#include "osal.h"

#define STACK_REPORT_BUF_SIZE 640

//静态字符数组，用作报告缓冲区的全局共享内存，DMA发送期间不能改写同一缓冲区，忙则本周期跳过
static char gStackReportBuf[STACK_REPORT_BUF_SIZE];

//追加一行任务栈信息到报告缓冲区
static uint16_t StackMonitor_AppendLine(uint16_t used, const char *name, TaskHandle_t taskHandle)
{
  int len;

  if(used >= STACK_REPORT_BUF_SIZE)
  {
    return used;
  }
  //任务句柄为NULL表示任务未创建或已删除，直接报告句柄NULL
  if(taskHandle == NULL)
  {
    len = snprintf(&gStackReportBuf[used], STACK_REPORT_BUF_SIZE - used,
                   "[STACK] %-14s: handle NULL\r\n", name);
  }
  else//获取任务栈高水位，单位字，转换为字节
  {
    UBaseType_t watermarkWords = uxTaskGetStackHighWaterMark(taskHandle);
    uint32_t watermarkBytes = (uint32_t)watermarkWords * sizeof(StackType_t);
    len = snprintf(&gStackReportBuf[used], STACK_REPORT_BUF_SIZE - used,
                   "[STACK] %-14s: %lu words (%lu bytes)\r\n",
                   name,
                   (unsigned long)watermarkWords,
                   (unsigned long)watermarkBytes);
  }

  if(len <= 0)
  {
    return used;
  }

  if((uint16_t)len >= (STACK_REPORT_BUF_SIZE - used))
  {
    return STACK_REPORT_BUF_SIZE - 1;
  }

  return (uint16_t)(used + (uint16_t)len);
}

//串口整包DMA发送，避免阻塞占用CPU
static void StackMonitor_SendReport(uint16_t used)
{
  if(used == 0U)
  {
    return;
  }
  
  if(HAL_UART_Transmit_DMA(&huart1, (uint8_t *)gStackReportBuf, used) != HAL_OK)
  {
    return;
  }
}

//打印所有任务的栈高水位
static void StackMonitor_PrintAll(void)
{
  uint16_t used = 0;

  /* DMA发送期间不能改写同一缓冲区，忙则本周期跳过 */
  if(huart1.gState != HAL_UART_STATE_READY)
  {
    return;
  }

  int len = snprintf(gStackReportBuf, STACK_REPORT_BUF_SIZE,
                     "\r\n===== Stack HWM @%lu ms =====\r\n",
                     (unsigned long)HAL_GetTick());

  if(len > 0)
  {
    if((uint16_t)len >= STACK_REPORT_BUF_SIZE)
    {
      used = STACK_REPORT_BUF_SIZE - 1;
    }
    else
    {
      used = (uint16_t)len;
    }
  }

  used = StackMonitor_AppendLine(used, "Key_Task", KeyTaskHandle);
  used = StackMonitor_AppendLine(used, "UI_Task", UITaskHandle);
  used = StackMonitor_AppendLine(used, "Set_Task", SetTaskHandle);
  used = StackMonitor_AppendLine(used, "Menu_Task", MenuTaskHandle);
  used = StackMonitor_AppendLine(used, "Time_Task", TimeTaskHandle);
  used = StackMonitor_AppendLine(used, "Flashlight", FlashlightTaskHandle);
  used = StackMonitor_AppendLine(used, "MPU6050", MPU6050TaskHandle);
  used = StackMonitor_AppendLine(used, "Game", GameTaskHandle);
  used = StackMonitor_AppendLine(used, "SleepMgr", SleepManagerTaskHandle);
  used = StackMonitor_AppendLine(used, "StackMon", StackMonitorTaskHandle);

  StackMonitor_SendReport(used);
}

//栈监控任务，周期5s打印一次所有任务的栈高水位
void StackMonitor_Task(void *argument)
{
  (void)argument;
  while(1)
  {
    StackMonitor_PrintAll();
    OSAL_Delay(5000U);
  }
}
