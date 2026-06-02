#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
 
/* ͷ�ļ� */
#include "stm32f1xx.h"
#include "main.h"  
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
 
extern uint32_t SystemCoreClock;
 
/* ���������� */
#define configUSE_PREEMPTION                            1                       /* 1: ��ռʽ������, 0: Э��ʽ������, ��Ĭ���趨�� */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION         1                       /* 1: ʹ��Ӳ��������һ��Ҫ���е�����, 0: ʹ�������㷨������һ��Ҫ���е�����, Ĭ��: 0 */
#define configUSE_TICKLESS_IDLE                         0                       /* 1: ʹ��tickless�͹���ģʽ, Ĭ��: 0 */
#define configCPU_CLOCK_HZ                              SystemCoreClock         /* ����CPU��Ƶ, ��λ: Hz, ��Ĭ���趨�� */
//#define configSYSTICK_CLOCK_HZ                          (configCPU_CLOCK_HZ / 8)/* ����SysTickʱ��Ƶ�ʣ���SysTickʱ��Ƶ�����ں�ʱ��Ƶ�ʲ�ͬʱ�ſ��Զ���, ��λ: Hz, Ĭ��: ������ */
#define configTICK_RATE_HZ                              1000                    /* ����ϵͳʱ�ӽ���Ƶ��, ��λ: Hz, ��Ĭ���趨�� */
#define configMAX_PRIORITIES                            10                      /* ����������ȼ���, ������ȼ�=configMAX_PRIORITIES-1, ��Ĭ���趨�� */
#define configMINIMAL_STACK_SIZE                        128                     /* ������������ջ�ռ��С, ��λ: Word, ��Ĭ���趨�� */
#define configMAX_TASK_NAME_LEN                         16                      /* ��������������ַ���, Ĭ��: 16 */
#define configUSE_16_BIT_TICKS                          0                       /* 1: ����ϵͳʱ�ӽ��ļ���������������Ϊ16λ�޷�����, ��Ĭ���趨�� */
#define configIDLE_SHOULD_YIELD                         1                       /* 1: ʹ������ռʽ������,ͬ���ȼ�����������ռ��������, Ĭ��: 1 */
#define configUSE_TASK_NOTIFICATIONS                    1                       /* 1: ʹ�������ֱ�ӵ���Ϣ����,�����ź������¼���־�����Ϣ����, Ĭ��: 1 */
#define configTASK_NOTIFICATION_ARRAY_ENTRIES           1                       /* ��������֪ͨ����Ĵ�С, Ĭ��: 1 */
#define configUSE_MUTEXES                               1                       /* 1: ʹ�ܻ����ź���, Ĭ��: 0 */
#define configUSE_RECURSIVE_MUTEXES                     1                       /* 1: ʹ�ܵݹ黥���ź���, Ĭ��: 0 */
#define configUSE_COUNTING_SEMAPHORES                   1                       /* 1: ʹ�ܼ����ź���, Ĭ��: 0 */
#define configUSE_ALTERNATIVE_API                       0                       /* ������!!! */
#define configQUEUE_REGISTRY_SIZE                       8                       /* �������ע����ź�������Ϣ���еĸ���, Ĭ��: 0 */
#define configUSE_QUEUE_SETS                            1                       /* 1: ʹ�ܶ��м�, Ĭ��: 0 */
#define configUSE_TIME_SLICING                          1                       /* 1: ʹ��ʱ��Ƭ����, Ĭ��: 1 */
#define configUSE_NEWLIB_REENTRANT                      0                       /* 1: ���񴴽�ʱ����Newlib������ṹ��, Ĭ��: 0 */  
#define configENABLE_BACKWARD_COMPATIBILITY             0                       /* 1: ʹ�ܼ����ϰ汾, Ĭ��: 1 */
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS         0                       /* �����̱߳��ش洢ָ��ĸ���, Ĭ��: 0 */
#define configSTACK_DEPTH_TYPE                          uint16_t                /* ���������ջ��ȵ���������, Ĭ��: uint16_t */
#define configMESSAGE_BUFFER_LENGTH_TYPE                size_t                  /* ������Ϣ����������Ϣ���ȵ���������, Ĭ��: size_t */
 
/* �ڴ������ض��� */
#define configSUPPORT_STATIC_ALLOCATION                 0                       /* 1: ֧�־�̬�����ڴ�, Ĭ��: 0 */
#define configSUPPORT_DYNAMIC_ALLOCATION                1                       /* 1: ֧�ֶ�̬�����ڴ�, Ĭ��: 1 */
#define configTOTAL_HEAP_SIZE                           ((size_t)(10 * 1024))   /* FreeRTOS���п��õ�RAM����, ��λ: Byte, ��Ĭ���趨�� */
#define configAPPLICATION_ALLOCATED_HEAP                0                       /* 1: �û��ֶ�����FreeRTOS�ڴ��(ucHeap), Ĭ��: 0 */
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP       0                       /* 1: �û�����ʵ�����񴴽�ʱʹ�õ��ڴ��������ͷź���, Ĭ��: 0 */
 
/* ���Ӻ�����ض��� */
#define configUSE_IDLE_HOOK                             0                       /* 1: ʹ�ܿ��������Ӻ���, ��Ĭ���趨��  */
#define configUSE_TICK_HOOK                             0                       /* 1: ʹ��ϵͳʱ�ӽ����жϹ��Ӻ���, ��Ĭ���趨�� */
#define configCHECK_FOR_STACK_OVERFLOW                  2                       /* 1: ʹ��ջ�����ⷽ��1, 2: ʹ��ջ�����ⷽ��2, Ĭ��: 0 */
#define configUSE_MALLOC_FAILED_HOOK                    0                       /* 1: ʹ�ܶ�̬�ڴ�����ʧ�ܹ��Ӻ���, Ĭ��: 0 */
#define configUSE_DAEMON_TASK_STARTUP_HOOK              0                       /* 1: ʹ�ܶ�ʱ�����������״�ִ��ǰ�Ĺ��Ӻ���, Ĭ��: 0 */
 
/* ����ʱ�������״̬ͳ����ض��� */
#define configGENERATE_RUN_TIME_STATS                   0                       /* 1: ʹ����������ʱ��ͳ�ƹ���, Ĭ��: 0 */
#if configGENERATE_RUN_TIME_STATS
#include "./BSP/TIMER/btim.h"
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()        ConfigureTimeForRunTimeStats()
extern uint32_t FreeRTOSRunTimeTicks;
#define portGET_RUN_TIME_COUNTER_VALUE()                FreeRTOSRunTimeTicks
#endif
#define configUSE_TRACE_FACILITY                        1                       /* 1: ʹ�ܿ��ӻ����ٵ���, Ĭ��: 0 */
#define configUSE_STATS_FORMATTING_FUNCTIONS            1                       /* 1: configUSE_TRACE_FACILITYΪ1ʱ�������vTaskList()��vTaskGetRunTimeStats()����, Ĭ��: 0 */
 
/* Э����ض��� */
#define configUSE_CO_ROUTINES                           0                       /* 1: ����Э��, Ĭ��: 0 */
#define configMAX_CO_ROUTINE_PRIORITIES                 2                       /* ����Э�̵�������ȼ�, ������ȼ�=configMAX_CO_ROUTINE_PRIORITIES-1, ��Ĭ��configUSE_CO_ROUTINESΪ1ʱ�趨�� */
 
/* ������ʱ����ض��� */
#define configUSE_TIMERS                                1                               /* 1: ʹ��������ʱ��, Ĭ��: 0 */
#define configTIMER_TASK_PRIORITY                       ( configMAX_PRIORITIES - 1 )    /* ����������ʱ����������ȼ�, ��Ĭ��configUSE_TIMERSΪ1ʱ�趨�� */
#define configTIMER_QUEUE_LENGTH                        5                               /* ����������ʱ��������еĳ���, ��Ĭ��configUSE_TIMERSΪ1ʱ�趨�� */
#define configTIMER_TASK_STACK_DEPTH                    ( configMINIMAL_STACK_SIZE * 2) /* ����������ʱ�������ջ�ռ��С, ��Ĭ��configUSE_TIMERSΪ1ʱ�趨�� */
 
/* ��ѡ����, 1: ʹ�� */
#define INCLUDE_vTaskPrioritySet                        1                       /* �����������ȼ� */
#define INCLUDE_uxTaskPriorityGet                       1                       /* ��ȡ�������ȼ� */
#define INCLUDE_vTaskDelete                             1                       /* ɾ������ */
#define INCLUDE_vTaskSuspend                            1                       /* �������� */
#define INCLUDE_xResumeFromISR                          1                       /* �ָ����ж��й�������� */
#define INCLUDE_vTaskDelayUntil                         1                       /* ���������ʱ */
#define INCLUDE_vTaskDelay                              1                       /* ������ʱ */
#define INCLUDE_xTaskGetSchedulerState                  1                       /* ��ȡ���������״̬ */
#define INCLUDE_xTaskGetCurrentTaskHandle               1                       /* ��ȡ��ǰ����������� */
#define INCLUDE_uxTaskGetStackHighWaterMark             1                       /* ��ȡ�����ջ��ʷʣ����Сֵ */
#define INCLUDE_xTaskGetIdleTaskHandle                  1                       /* ��ȡ��������������� */
#define INCLUDE_eTaskGetState                           1                       /* ��ȡ����״̬ */
#define INCLUDE_xEventGroupSetBitFromISR                1                       /* ���ж��������¼���־λ */
#define INCLUDE_xTimerPendFunctionCall                  1                       /* ��������ִ�йҵ���ʱ���������� */
#define INCLUDE_xTaskAbortDelay                         1                       /* �ж�������ʱ */
#define INCLUDE_xTaskGetHandle                          1                       /* ͨ����������ȡ������ */
#define INCLUDE_xTaskResumeFromISR                      1                       /* �ָ����ж��й�������� */
 
/* �ж�Ƕ����Ϊ���� */
#ifdef __NVIC_PRIO_BITS
    /* ע�⣺�˴��� configPRIO_BITS ����ֱ�ӵ��� __NVIC_PRIO_BITS��
       ��Ϊ STM32 HAL ���У�__NVIC_PRIO_BITS �ᱻ����Ϊ 4U��
       ARMCC5 ������������޷�ʶ����� 'U' ��׺�Ĳ���������ᵼ�� port.c ��౨����
       �������д��Ϊ 4 ����ȥ�� U ��׺�� */
    #define configPRIO_BITS 4
#else
    #define configPRIO_BITS 4
#endif
 
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15                  /* �ж�������ȼ� */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5                   /* FreeRTOS�ɹ���������ж����ȼ� */
#define configKERNEL_INTERRUPT_PRIORITY                 0xF0   /* 15 << (8 - 4) = 240. Literal required for ARMCC5 inline asm compatibility. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY            0x50   /* 5 << (8 - 4) = 80. Literal required for ARMCC5 inline asm compatibility (port.c:424). */
#define configMAX_API_CALL_INTERRUPT_PRIORITY           configMAX_SYSCALL_INTERRUPT_PRIORITY
 
/* FreeRTOS�жϷ�������ض��� */
#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler
#define xPortSysTickHandler SysTick_Handler
 
 
/* ���� */
#define vAssertCalled(file, line) do{printf("Error: %s, %d\r\n", file, line);Error_Handler();}while(0)
#define configASSERT( x ) do{if( ( x ) == 0 ) vAssertCalled( __FILE__, __LINE__ );}while(0)
 
 
#endif /* FREERTOS_CONFIG_H */
