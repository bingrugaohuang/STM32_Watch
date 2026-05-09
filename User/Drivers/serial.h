#ifndef DRIVER_SERIAL_H
#define DRIVER_SERIAL_H

#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "portmacro.h"

/**
  * 函    数：初始化串口抽象层
  * 参    数：无
  * 返 回 值：无
  * 说    明：创建发送互斥锁、发送队列、DMA 相关信号量，创建发送任务 "tx_task"。
  *           任务优先级中等（如 5），栈大小 256。
  */
void serial_init(void);

/**
  * 函    数：异步串口发送（任务上下文）
  * 参    数：pData - 数据指针
  *           size  - 字节数
  * 返 回 值：0 成功，-1 参数错误，-2 内存不足, -3队列满
  * 说    明：将数据复制到动态分配的缓冲区，放入发送队列，由发送任务通过 DMA 发出。
  */
int serial_send_async(const uint8_t *pData, size_t size);

/**
  * 函    数：异步串口发送（中断上下文）
  * 参    数：pData - 数据
  *           size  - 长度
  *           pxHigherPriorityTaskWoken - 标记是否需要任务切换
  * 返 回 值：0 成功，-1 参数错误，-2 内存不足, -3队列满
  * 说    明：内部调用队列 FromISR 版本投递，唤醒串口发送任务。
  */
int serial_send_async_from_isr(const uint8_t *pData, size_t size, BaseType_t *pxHigherPriorityTaskWoken);

/**
  * 函    数：紧急轮询发送
  * 参    数：pData - 数据
  *           size  - 长度
  * 返 回 值：无
  * 说    明：直接操作 UART 数据寄存器逐字节发送，不依赖中断、DMA 或任务。
  *           仅用于 Error_Handler 等致命错误场景。
  */
void serial_send_blocking(const uint8_t *pData, size_t size);

#endif