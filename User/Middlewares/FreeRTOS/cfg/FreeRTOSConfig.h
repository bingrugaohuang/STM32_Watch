#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* 头文件 */
#include "stm32f1xx.h"
#include "main.h"  
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

extern uint32_t SystemCoreClock;

/* 内核配置 */
#define configUSE_PREEMPTION                            1                       /* 1: 抢占式调度器, 0: 协作式调度器, 无默认需设定 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1                       /* 1: 使用硬件计算下一个要运行的任务, 0: 使用软件算法计算下一个要运行的任务, 默认: 0 */
#define configUSE_TICKLESS_IDLE                         1                       /* 1: 使用tickless低功耗模式, 默认: 0 */
#define configCPU_CLOCK_HZ                              SystemCoreClock         /* 定义CPU主频, 单位: Hz, 无默认需设定 */
//#define configSYSTICK_CLOCK_HZ                          (configCPU_CLOCK_HZ / 8)/* 定义SysTick时钟频率，当SysTick时钟频率与内核时钟频率不同时才可以定义, 单位: Hz, 默认: 不定义 */
#define configTICK_RATE_HZ                              1000                    /* 定义系统时钟节拍频率, 单位: Hz, 无默认需设定 */
#define configMAX_PRIORITIES                            10                      /* 定义最大优先级数, 可用优先级=configMAX_PRIORITIES-1, 无默认需设定 */
#define configMINIMAL_STACK_SIZE                        128                     /* 定义空闲任务的栈空间大小, 单位: Word, 无默认需设定 */
#define configMAX_TASK_NAME_LEN                         16                      /* 定义任务名称最大字符串长度, 默认: 16 */
#define configUSE_16_BIT_TICKS                          0                       /* 1: 系统时钟节拍计数器变量定义为16位无符号数, 默认: 0 */
#define configIDLE_SHOULD_YIELD                         1                       /* 1: 启用抢占式调度时，同优先级任务抢占空闲任务, 默认: 1 */
#define configUSE_TASK_NOTIFICATIONS                    1                       /* 1: 启用任务直接消息传递，包含信号量、事件标志组和消息传递, 默认: 1 */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1                       /* 定义任务通知数组的大小, 默认: 1 */
#define configUSE_MUTEXES                               1                       /* 1: 启用互斥信号量, 默认: 0 */
#define configUSE_RECURSIVE_MUTEXES                     1                       /* 1: 启用递归互斥信号量, 默认: 0 */
#define configUSE_COUNTING_SEMAPHORES                   1                       /* 1: 启用计数信号量, 默认: 0 */
#define configUSE_ALTERNATIVE_API                       0                       /* 已弃用!!! */
#define configQUEUE_REGISTRY_SIZE                       8                       /* 定义可以注册的队列和信号量的最大个数, 默认: 0 */
#define configUSE_QUEUE_SETS                            1                       /* 1: 启用队列集, 默认: 0 */
#define configUSE_TIME_SLICING                          1                       /* 1: 启用时间片调度, 默认: 1 */
#define configUSE_NEWLIB_REENTRANT                      0                       /* 1: 任务创建时分配Newlib可重入结构, 默认: 0 */
#define configENABLE_BACKWARD_COMPATIBILITY             0                       /* 1: 启用兼容旧版本, 默认: 1 */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0                       /* 定义线程本地存储指针的个数, 默认: 0 */
#define configSTACK_DEPTH_TYPE                          uint16_t                /* 定义任务栈深度的数据类型, 默认: uint16_t */
#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t                  /* 定义消息缓冲区消息长度的数据类型, 默认: size_t */

/* 内存分配相关定义 */
#define configSUPPORT_STATIC_ALLOCATION                 0                       /* 1: 支持静态分配内存, 默认: 0 */
#define configSUPPORT_DYNAMIC_ALLOCATION                1                       /* 1: 支持动态分配内存, 默认: 1 */
#define configTOTAL_HEAP_SIZE                           ((size_t)(8 * 1024))    /* FreeRTOS堆中可用的RAM总量, 单位: Byte, 无默认需设定 */
#define configAPPLICATION_ALLOCATED_HEAP                0                       /* 1: 用户手动分配FreeRTOS内存堆(ucHeap), 默认: 0 */
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0                       /* 1: 用户自行实现任务创建时使用的内存分配和释放函数, 默认: 0 */

/* 钩子函数相关定义 */
#define configUSE_IDLE_HOOK                             0                       /* 1: 启用空闲任务钩子函数, 无默认需设定  */
#define configUSE_TICK_HOOK                             0                       /* 1: 启用系统时钟节拍中断钩子函数, 无默认需设定 */
#define configCHECK_FOR_STACK_OVERFLOW                  2                       /* 1: 使用栈溢出检测方法1, 2: 使用栈溢出检测方法2, 默认: 0 */
#define configUSE_MALLOC_FAILED_HOOK                    0                       /* 1: 启用动态内存分配失败钩子函数, 默认: 0 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK              0                       /* 1: 启用定时器服务任务首次执行前的钩子函数, 默认: 0 */

/* 运行时任务运行状态统计相关定义 */
#define configGENERATE_RUN_TIME_STATS                   0                       /* 1: 启用任务运行时间统计功能, 默认: 0 */
#if configGENERATE_RUN_TIME_STATS
#include "./BSP/TIMER/btim.h"
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()        ConfigureTimeForRunTimeStats()
extern uint32_t FreeRTOSRunTimeTicks;
#define portGET_RUN_TIME_COUNTER_VALUE()                FreeRTOSRunTimeTicks
#endif
#define configUSE_TRACE_FACILITY                        1                       /* 1: 启用可视化跟踪调试, 默认: 0 */
#define configUSE_STATS_FORMATTING_FUNCTIONS            1                       /* 1: configUSE_TRACE_FACILITY为1时可生成vTaskList()和vTaskGetRunTimeStats()函数, 默认: 0 */

/* 协程相关定义 */
#define configUSE_CO_ROUTINES                           0                       /* 1: 启用协程, 默认: 0 */
#define configMAX_CO_ROUTINE_PRIORITIES                 2                       /* 定义协程的最大优先级数, 可用优先级=configMAX_CO_ROUTINE_PRIORITIES-1, 需在configUSE_CO_ROUTINES为1时设定 */

/* 软件定时器相关定义 */
#define configUSE_TIMERS                                1                               /* 1: 启用软件定时器, 默认: 0 */
#define configTIMER_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )    /* 定义软件定时器服务任务的优先级, 需在configUSE_TIMERS为1时设定 */
#define configTIMER_QUEUE_LENGTH                        5                               /* 定义软件定时器命令队列的长度, 需在configUSE_TIMERS为1时设定 */
#define configTIMER_TASK_STACK_DEPTH                    ( configMINIMAL_STACK_SIZE * 2) /* 定义软件定时器服务任务的栈空间大小, 需在configUSE_TIMERS为1时设定 */

/* 可选函数, 1: 启用 */
#define INCLUDE_vTaskPrioritySet                        1                       /* 设置任务优先级 */
#define INCLUDE_uxTaskPriorityGet                       1                       /* 获取任务优先级 */
#define INCLUDE_vTaskDelete                             1                       /* 删除任务 */
#define INCLUDE_vTaskSuspend                            1                       /* 挂起任务 */
#define INCLUDE_xResumeFromISR                          1                       /* 恢复中断中挂起的任务 */
#define INCLUDE_vTaskDelayUntil                         1                       /* 任务延时到绝对时间 */
#define INCLUDE_vTaskDelay                              1                       /* 任务延时 */
#define INCLUDE_xTaskGetSchedulerState                  1                       /* 获取调度器状态 */
#define INCLUDE_xTaskGetCurrentTaskHandle               1                       /* 获取当前任务句柄 */
#define INCLUDE_uxTaskGetStackHighWaterMark             1                       /* 获取任务栈历史剩余最小值 */
#define INCLUDE_xTaskGetIdleTaskHandle                  1                       /* 获取空闲任务句柄 */
#define INCLUDE_eTaskGetState                           1                       /* 获取任务状态 */
#define INCLUDE_xEventGroupSetBitFromISR                1                       /* 在中断中设置事件标志位 */
#define INCLUDE_xTimerPendFunctionCall                  1                       /* 延迟执行挂起的定时器回调函数 */
#define INCLUDE_xTaskAbortDelay                         1                       /* 中止任务延时 */
#define INCLUDE_xTaskGetHandle                          1                       /* 通过任务名称获取句柄 */
#define INCLUDE_xTaskResumeFromISR                      1                       /* 恢复中断中挂起的任务 */

/* 中断嵌套行为配置 */
#ifdef __NVIC_PRIO_BITS
    /* 注意：此处将 configPRIO_BITS 直接定义为 4。
       因为 STM32 HAL 库中，__NVIC_PRIO_BITS 会被定义为 4U。
       ARMCC5 编译器可能无法识别带有 'U' 后缀的常量参数，会导致 port.c 编译错误。
       所以此处写为 4 并去掉 U 后缀。 */
    #define configPRIO_BITS 4
#else
    #define configPRIO_BITS 4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15                  /* 中断最低优先级 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5                   /* FreeRTOS可管理的中断最高优先级 */
#define configKERNEL_INTERRUPT_PRIORITY                 0xF0   /* 15 << (8 - 4) = 240. 用于ARMCC5内联汇编的字面值。 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            0x50   /* 5 << (8 - 4) = 80. 用于ARMCC5内联汇编的字面值 (port.c:424)。 */
#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY

/* FreeRTOS中断服务函数重映射 */
#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
#define xPortSysTickHandler SysTick_Handler


/* 断言 */
#define vAssertCalled(file, line) do{printf("Error: %s, %d\r\n", file, line);Error_Handler();}while(0)
#define configASSERT( x ) do{if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );}while(0)


#endif /* FREERTOS_CONFIG_H */