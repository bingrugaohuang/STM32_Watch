#include "osal.h"
#include "main.h"   // 引入 Error_Handler 声明

/* 如果在 FreeRTOSConfig.h 中未开启 configASSERT，则定义为空 */
#ifndef configASSERT
#define configASSERT( x )
#endif

/**
  * 函    数：OSAL 层初始化
  * 说    明：当前无特殊操作，留作扩展。
  */
void osal_init(void)
{
    // 目前 FreeRTOS 不需要特殊的 OSAL 层初始化
}

/**
  * 函    数：创建任务
  * 说    明：调用 xTaskCreate，进行参数检查并处理可能的失败。
  */
osal_task_handle_t osal_task_create( const char * const pcName,
                                     void (*pxTaskCode)(void *),
                                     const uint16_t usStackDepth,
                                     void * const pvParameters,
                                     uint32_t uxPriority )
{
    configASSERT( pxTaskCode != NULL );
    configASSERT( usStackDepth > 0 );
    configASSERT( uxPriority < configMAX_PRIORITIES );
    
    TaskHandle_t xHandle = NULL;
    if (xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, &xHandle) != pdPASS)
    {
        Error_Handler();
    }
    return (osal_task_handle_t)xHandle;
}

/**
  * 函    数：挂起任务
  */
void osal_task_suspend( osal_task_handle_t xTask )
{
    vTaskSuspend( xTask );
}

/**
  * 函    数：恢复任务
  */
void osal_task_resume( osal_task_handle_t xTask )
{
    vTaskResume( xTask );
}

/**
  * 函    数：任务延时
  */
void osal_task_delay( const uint32_t ulTicks )
{
    vTaskDelay( ulTicks );
}

/**
  * 函    数：获取系统节拍
  */
uint32_t osal_get_tick(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/**
  * 函    数：创建软件定时器
  */
osal_timer_handle_t osal_timer_create( const char * const pcTimerName,
                                       const uint32_t ulPeriodInTicks,
                                       const uint8_t ucAutoReload,
                                       void * const pvTimerID,
                                       void (*pxCallbackFunction)( osal_timer_handle_t ) )
{
    configASSERT(pxCallbackFunction != NULL);
    
    TimerHandle_t xTimer = xTimerCreate(pcTimerName, 
                                        ulPeriodInTicks, 
                                        ucAutoReload ? pdTRUE : pdFALSE, 
                                        pvTimerID, 
                                        (TimerCallbackFunction_t)pxCallbackFunction);
    if (xTimer == NULL)
    {
        Error_Handler();
    }
    return (osal_timer_handle_t)xTimer;
}

/**
  * 函    数：启动定时器
  */
void osal_timer_start( osal_timer_handle_t xTimer, uint32_t ulBlockTime )
{
    configASSERT( xTimer != NULL );
    xTimerStart( xTimer, ulBlockTime );
}

/**
  * 函    数：从中断启动定时器
  */
void osal_timer_start_from_isr( osal_timer_handle_t xTimer, uint32_t ulBlockTime, BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xTimer != NULL );
    xTimerStartFromISR( xTimer, pxHigherPriorityTaskWoken);
}

/**
  * 函    数：延迟执行命令（从中断上下文调用）
  * 注：      需要自己定义一个 PendedFunction_t 类型的回调函数作为参数传入
  */
void osal_timer_deferred( PendedFunction_t pxFunction, BaseType_t *pxHigherPriorityTaskWoken )
{
    xTimerPendFunctionCallFromISR(pxFunction, 0, 0, pxHigherPriorityTaskWoken);
}

/**
  * 函    数：停止定时器
  */
void osal_timer_stop( osal_timer_handle_t xTimer, uint32_t ulBlockTime )
{
    configASSERT( xTimer != NULL );
    xTimerStop( xTimer, ulBlockTime );
}

/**
  * 函    数：创建队列
  */
osal_queue_handle_t osal_queue_create( uint32_t uxQueueLength, uint32_t uxItemSize )
{
    configASSERT( uxQueueLength > 0 );
    configASSERT( uxItemSize > 0 );
    
    QueueHandle_t xQueue = xQueueCreate(uxQueueLength, uxItemSize);
    if (xQueue == NULL)
    {
        Error_Handler();
    }
    return (osal_queue_handle_t)xQueue;
}

/**
  * 函    数：向队列发送数据（任务上下文）
  */
osal_status_t osal_queue_send( osal_queue_handle_t xQueue, const void *pvItemToQueue, uint32_t ulTicksToWait )
{
    configASSERT( xQueue != NULL );
    configASSERT( pvItemToQueue != NULL );
    
    return (xQueueSend(xQueue, pvItemToQueue, ulTicksToWait) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：从队列接收数据（任务上下文）
  */
osal_status_t osal_queue_receive( osal_queue_handle_t xQueue, void *pvBuffer, uint32_t ulTicksToWait )
{
    configASSERT( xQueue != NULL );
    configASSERT( pvBuffer != NULL );
    
    return (xQueueReceive(xQueue, pvBuffer, ulTicksToWait) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：从中断向队列发送数据
  */
osal_status_t osal_queue_send_from_isr( osal_queue_handle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xQueue != NULL );
    configASSERT( pvItemToQueue != NULL );
    
    return (xQueueSendFromISR(xQueue, pvItemToQueue, pxHigherPriorityTaskWoken) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：从中断接收队列数据
  */
osal_status_t osal_queue_receive_from_isr( osal_queue_handle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xQueue != NULL );
    configASSERT( pvBuffer != NULL );
    
    return (xQueueReceiveFromISR(xQueue, pvBuffer, pxHigherPriorityTaskWoken) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}
/**
  * 函    数：创建信号量（计数/二值 统一接口）
  */
osal_semaphore_handle_t osal_semaphore_create( uint32_t max_count, uint32_t initial_count )
{
    configASSERT( max_count > 0 );
    configASSERT( initial_count <= max_count );
    
    SemaphoreHandle_t xSemaphore = xSemaphoreCreateCounting( max_count, initial_count );
    if (xSemaphore == NULL)
    {
        Error_Handler();
    }
    return (osal_semaphore_handle_t)xSemaphore;
}

/**
  * 函    数：获取信号量（任务上下文）
  */
osal_status_t osal_semaphore_take( osal_semaphore_handle_t xSemaphore, uint32_t ulTicksToWait )
{
    configASSERT( xSemaphore != NULL );
    return (xSemaphoreTake(xSemaphore, ulTicksToWait) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：获取信号量（中断上下文）
  */
osal_status_t osal_semaphore_take_from_isr( osal_semaphore_handle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xSemaphore != NULL );
    return (xSemaphoreTakeFromISR(xSemaphore, pxHigherPriorityTaskWoken) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：释放信号量（任务上下文）
  */
osal_status_t osal_semaphore_give( osal_semaphore_handle_t xSemaphore )
{
    configASSERT( xSemaphore != NULL );
    return (xSemaphoreGive(xSemaphore) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：释放信号量（中断上下文）
  */
osal_status_t osal_semaphore_give_from_isr( osal_semaphore_handle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xSemaphore != NULL );
    return (xSemaphoreGiveFromISR(xSemaphore, pxHigherPriorityTaskWoken) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}
/**
  * 函    数：创建递归互斥锁
  */
osal_mutex_handle_t osal_recursive_mutex_create(void)
{
    SemaphoreHandle_t xMutex = xSemaphoreCreateRecursiveMutex();
    if (xMutex == NULL)
    {
        Error_Handler();
    }
    return (osal_mutex_handle_t)xMutex;
}

/**
  * 函    数：获取递归锁
  */
osal_status_t osal_recursive_mutex_take( osal_mutex_handle_t xMutex, uint32_t ulTicksToWait )
{
    configASSERT( xMutex != NULL );
    return (xSemaphoreTakeRecursive(xMutex, ulTicksToWait) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：释放递归锁
  */
osal_status_t osal_recursive_mutex_give( osal_mutex_handle_t xMutex )
{
    configASSERT( xMutex != NULL );
    return (xSemaphoreGiveRecursive(xMutex) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：任务通知 - 获取
  */
uint32_t osal_task_notify_take( uint8_t ucIndex, uint8_t ucClearOnExit, uint32_t ulTicksToWait )
{
    configASSERT( ucIndex < configTASK_NOTIFICATION_ARRAY_ENTRIES );
    return ulTaskNotifyTakeIndexed(ucIndex, ucClearOnExit ? pdTRUE : pdFALSE, ulTicksToWait);
}

/*
 * 函     数：任务通知 - 等待消息 
 */
osal_status_t osal_task_notify_wait( uint32_t ulBitsToClearOnEntry,
                                     uint32_t ulBitsToClearOnExit,
                                     uint32_t *pulNotificationValue,
                                     uint32_t ulTicksToWait )
{
    // FreeRTOS 的 xTaskNotifyWait 返回 pdTRUE(1) 或 pdFALSE(0)
    BaseType_t ret = xTaskNotifyWait( ulBitsToClearOnEntry,
                                      ulBitsToClearOnExit,
                                      pulNotificationValue,
                                      ulTicksToWait );
    return (ret == pdPASS) ? OSAL_OK : OSAL_FAIL;
}
/**
  * 函    数：任务通知 - 发送
  */
osal_status_t osal_task_notify( osal_task_handle_t xTaskToNotify, uint8_t ucIndex, uint32_t ulValue, uint32_t ulAction )
{
    configASSERT( xTaskToNotify != NULL );
    configASSERT( ucIndex < configTASK_NOTIFICATION_ARRAY_ENTRIES );
    
    eNotifyAction eAction;
    switch (ulAction)
    {
        case OSAL_NOTIFY_SET_BITS:                eAction = eSetBits; break;
        case OSAL_NOTIFY_INCREMENT:               eAction = eIncrement; break;
        case OSAL_NOTIFY_SET_VALUE_OVERWRITE:     eAction = eSetValueWithOverwrite; break;
        case OSAL_NOTIFY_SET_VALUE_NO_OVERWRITE:  eAction = eSetValueWithoutOverwrite; break;
        case OSAL_NOTIFY_NO_ACTION:               eAction = eNoAction; break;
        default: return OSAL_FAIL;
    }
    
    return (xTaskNotifyIndexed(xTaskToNotify, ucIndex, ulValue, eAction) == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：任务通知 - 从中断发送
  */
osal_status_t osal_task_notify_from_isr( osal_task_handle_t xTaskToNotify,
                                         uint8_t ucIndex,
                                         uint32_t ulValue,
                                         uint32_t ulAction,
                                         BaseType_t *pxHigherPriorityTaskWoken )
{
    configASSERT( xTaskToNotify != NULL );
    configASSERT( ucIndex < configTASK_NOTIFICATION_ARRAY_ENTRIES );
    configASSERT( pxHigherPriorityTaskWoken != NULL );

    eNotifyAction eAction;
    switch (ulAction)
    {
        case OSAL_NOTIFY_SET_BITS:                eAction = eSetBits; break;
        case OSAL_NOTIFY_INCREMENT:               eAction = eIncrement; break;
        case OSAL_NOTIFY_SET_VALUE_OVERWRITE:     eAction = eSetValueWithOverwrite; break;
        case OSAL_NOTIFY_SET_VALUE_NO_OVERWRITE:  eAction = eSetValueWithoutOverwrite; break;
        case OSAL_NOTIFY_NO_ACTION:               eAction = eNoAction; break;
        default: return OSAL_FAIL;
    }

    BaseType_t result = xTaskNotifyIndexedFromISR( xTaskToNotify, ucIndex, ulValue, eAction, pxHigherPriorityTaskWoken );
    return (result == pdPASS) ? OSAL_OK : OSAL_FAIL;
}

/**
  * 函    数：内存分配
  */
void *osal_malloc(size_t size)
{
    return pvPortMalloc(size);
}

/**
  * 函    数：内存释放
  */
void osal_free(void *ptr)
{
    vPortFree(ptr);
}

/**
  * 函    数：启动调度器
  * 说    明：调用 vTaskStartScheduler，若返回说明堆内存不足，进入 Error_Handler。
  */
void osal_start_scheduler(void)
{
    vTaskStartScheduler();
    
    /* 如果调度器返回，说明内存不足或发生了错误 */
    Error_Handler();
}

/**
  * 函    数：获取任务栈的高水位标记
  * 参    数：xTask - 任务句柄
  * 返 回 值：任务栈的高水位标记（剩余栈空间的最小值）
  * 说    明：封装 uxTaskGetStackHighWaterMark，返回剩余栈空间的最小值。
  */
osal_UBaseType_t osal_getstackhighwatermark(osal_task_handle_t xTask)
{
    configASSERT(xTask != NULL);
    return (osal_UBaseType_t)uxTaskGetStackHighWaterMark(xTask);
}