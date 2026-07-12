#ifndef TASK_SERVICE_H
#define TASK_SERVICE_H

#include "osal.h"
#include "common_macro.h"

/**
 * @brief 初始化显示任务
 *
 * 该函数创建并初始化显示任务，负责处理显示相关的操作。
 * 如果任务创建失败，将调用 Error_Handler() 进行错误处理。
 */
void displaytask_init(void);

/**
 * @brief 初始化 MPU6050 处理任务
 *
 * 该函数创建并初始化 MPU6050 处理任务，负责处理 MPU6050 传感器的数据。
 * 如果任务创建失败，将调用 Error_Handler() 进行错误处理。
 */
void mpu6050task_init(void);

/**
 * @brief 初始化 MPU6050 队列
 *
 * 该函数创建并初始化用于 MPU6050 数据传输的队列。
 * 如果队列创建失败，将调用 Error_Handler() 进行错误处理。
 */
void mpu6050_queue_init(void);

#if STACK_MONITOR_ENABLE
/**
 * @brief 栈高水位监控函数
 *
 * 该函数用于打印指定任务的栈高水位标记，帮助开发者监控任务的栈使用情况。
 * @param xTask 任务句柄
 * @param taskName 任务名称字符串，用于日志输出
 */
void stackmonitor(osal_task_handle_t xTask, const char *taskName);
#else
#define stackmonitor(xTask, taskName)  /* 空实现，当 STACK_MONITOR_ENABLE 为 0 时不执行任何操作 */
#endif

#endif