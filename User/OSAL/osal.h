#ifndef _OSAL_H
#define _OSAL_H

#include <stdint.h>/* 包含标准整数类型定义 */

/* 包含 FreeRTOS 头文件，具体实现依赖，但接口对外统一 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ---------- 类型重定义，隔离RTOS具体类型 ---------- */
typedef TaskHandle_t          osal_task_handle_t;      /**< 任务句柄 */
typedef QueueHandle_t         osal_queue_handle_t;     /**< 队列句柄 */
typedef TimerHandle_t         osal_timer_handle_t;     /**< 软件定时器句柄 */
typedef SemaphoreHandle_t osal_semaphore_handle_t;     /**< 信号量句柄 */
typedef SemaphoreHandle_t     osal_mutex_handle_t;     /**< 互斥锁句柄（递归锁也用此类型） */
typedef BaseType_t            osal_status_t;           /**< 操作状态 */

/* ---------- 通用返回值定义 ---------- */
#define OSAL_OK                 pdPASS
#define OSAL_FAIL               pdFAIL
#define OSAL_WAIT_FOREVER        portMAX_DELAY
#define OSAL_NO_WAIT            0

/* ---------- 任务通知动作定义 ---------- */
#define OSAL_NOTIFY_NO_ACTION               0
#define OSAL_NOTIFY_SET_BITS                1
#define OSAL_NOTIFY_INCREMENT               2
#define OSAL_NOTIFY_SET_VALUE_OVERWRITE     3
#define OSAL_NOTIFY_SET_VALUE_NO_OVERWRITE  4

/* ======================== 函数声明 ======================== */

/**
  * 函    数：OSAL 层初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：若 RTOS 有特殊初始化需求可在此完成，FreeRTOS 通常无需操作。
  */
void osal_init(void);

/**
  * 函    数：创建任务
  * 参    数：pcName          - 任务名称字符串
  *           pxTaskCode      - 任务函数指针
  *           usStackDepth    - 任务栈大小（字）
  *           pvParameters    - 传递给任务函数的参数
  *           uxPriority      - 任务优先级（0 最低）
  * 返 回 值：任务句柄，NULL表示创建失败
  * 说    明：封装 xTaskCreate，使用动态内存分配。
  */
osal_task_handle_t osal_task_create( const char * const pcName,
                                     void (*pxTaskCode)(void *),
                                     const uint16_t usStackDepth,
                                     void * const pvParameters,
                                     uint32_t uxPriority );

/**
  * 函    数：挂起任务
  * 参    数：xTask - 任务句柄
  * 返 回 值：无
  * 说    明：挂起指定任务，使其暂停运行。
  */
void osal_task_suspend( osal_task_handle_t xTask );

/**
  * 函    数：恢复任务
  * 参    数：xTask - 任务句柄
  * 返 回 值：无
  * 说    明：恢复先前挂起的任务。
  */
void osal_task_resume( osal_task_handle_t xTask );

/**
  * 函    数：任务延时
  * 参    数：ulTicks - 延时节拍数
  * 返 回 值：无
  * 说    明：当前任务进入阻塞状态指定时间。
  */
void osal_task_delay( const uint32_t ulTicks );

/**
  * 函    数：获取系统节拍
  * 参    数：无
  * 返 回 值：当前系统节拍计数值
  * 说    明：返回自启动以来的节拍数，可用于时间测量。
  */
uint32_t osal_get_tick(void);

/**
  * 函    数：创建软件定时器
  * 参    数：pcTimerName        - 定时器名称字符串
  *           ulPeriodInTicks    - 周期（节拍数）
  *           ucAutoReload       - 1：自动重载（周期定时器），0：一次性
  *           pvTimerID          - 定时器ID（可选参数）
  *           pxCallbackFunction - 定时器回调函数，原型 void func(osal_timer_handle_t)
  * 返 回 值：定时器句柄，NULL表示创建失败
  * 说    明：封装 xTimerCreate。
  */
osal_timer_handle_t osal_timer_create( const char * const pcTimerName,
                                       const uint32_t ulPeriodInTicks,
                                       const uint8_t ucAutoReload,
                                       void * const pvTimerID,
                                       void (*pxCallbackFunction)( osal_timer_handle_t ) );

/**
  * 函    数：启动定时器
  * 参    数：xTimer     - 定时器句柄
  *           ulBlockTime - 阻塞等待时间（通常设为 OSAL_NO_WAIT）
  * 返 回 值：无
  * 说    明：封装 xTimerStart，若定时器已运行则重启。
  */
void osal_timer_start( osal_timer_handle_t xTimer, uint32_t ulBlockTime );

/**
  * 函    数：停止定时器
  * 参    数：xTimer     - 定时器句柄
  *           ulBlockTime - 阻塞等待时间（通常设为 OSAL_NO_WAIT）
  * 返 回 值：无
  * 说    明：封装 xTimerStop，停止后定时器不再触发。
  */
void osal_timer_stop( osal_timer_handle_t xTimer, uint32_t ulBlockTime );

/**
  * 函    数：创建队列
  * 参    数：uxQueueLength - 队列长度（最多可存储的消息数）
  *           uxItemSize    - 每项消息的大小（字节）
  * 返 回 值：队列句柄，NULL表示创建失败
  * 说    明：封装 xQueueCreate，用于任务间数据传递。
  */
osal_queue_handle_t osal_queue_create( uint32_t uxQueueLength, uint32_t uxItemSize );

/**
  * 函    数：向队列发送数据（任务上下文）
  * 参    数：xQueue         - 队列句柄
  *           pvItemToQueue  - 待发送数据的指针
  *           ulTicksToWait  - 超时等待时间
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 失败或超时
  * 说    明：若队列满则阻塞指定时间。
  */
osal_status_t osal_queue_send( osal_queue_handle_t xQueue, const void *pvItemToQueue, uint32_t ulTicksToWait );

/**
  * 函    数：从队列接收数据（任务上下文）
  * 参    数：xQueue         - 队列句柄
  *           pvBuffer       - 接收缓冲区指针
  *           ulTicksToWait  - 超时等待时间
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 失败或超时
  * 说    明：若队列空则阻塞指定时间。
  */
osal_status_t osal_queue_receive( osal_queue_handle_t xQueue, void *pvBuffer, uint32_t ulTicksToWait );

/**
  * 函    数：从中断向队列发送数据
  * 参    数：xQueue                    - 队列句柄
  *           pvItemToQueue             - 数据指针
  *           pxHigherPriorityTaskWoken - 输出参数，标记是否需要任务切换
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 队列满
  * 说    明：中断服务函数中调用，可能唤醒等待该队列的高优先级任务。
  */
osal_status_t osal_queue_send_from_isr( osal_queue_handle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken );

/**
  * 函    数：从中断接收队列数据
  * 参    数：xQueue                    - 队列句柄
  *           pvBuffer                  - 接收缓冲区
  *           pxHigherPriorityTaskWoken - 输出参数，标记是否需要任务切换
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 队列空
  * 说    明：中断服务函数中调用，不会阻塞。
  */
osal_status_t osal_queue_receive_from_isr( osal_queue_handle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken );
/**
  * 函    数：创建信号量（计数/二值 统一接口）
  * 参    数：max_count     - 最大计数值（1 即二值信号量）
  *           initial_count - 初始计数值
  * 返 回 值：信号量句柄，NULL 表示创建失败
  * 说    明：若需二值信号量，调用 osal_semaphore_create(1, 0) 即可。
  */
osal_semaphore_handle_t osal_semaphore_create( uint32_t max_count, uint32_t initial_count );

/**
  * 函    数：获取信号量（任务上下文）
  * 参    数：xSemaphore   - 信号量句柄
  *           ulTicksToWait - 超时等待时间
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 超时或参数无效
  * 说    明：信号量计数值 > 0 则立即返回并减一，否则阻塞。
  */
osal_status_t osal_semaphore_take( osal_semaphore_handle_t xSemaphore, uint32_t ulTicksToWait );

/**
  * 函    数：获取信号量（中断上下文）
  * 参    数：xSemaphore                - 信号量句柄
  *           pxHigherPriorityTaskWoken  - 标记是否需要任务切换
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 信号量不可用
  * 说    明：不阻塞，仅当计数值 > 0 时成功并减一。
  */
osal_status_t osal_semaphore_take_from_isr( osal_semaphore_handle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );

/**
  * 函    数：释放信号量（任务上下文）
  * 参    数：xSemaphore - 信号量句柄
  * 返 回 值：OSAL_OK 成功
  * 说    明：计数值加一，如有等待任务则唤醒。
  */
osal_status_t osal_semaphore_give( osal_semaphore_handle_t xSemaphore );

/**
  * 函    数：释放信号量（中断上下文）
  * 参    数：xSemaphore                - 信号量句柄
  *           pxHigherPriorityTaskWoken  - 标记是否需要任务切换
  * 返 回 值：OSAL_OK 成功
  * 说    明：中断安全版本。
  */
osal_status_t osal_semaphore_give_from_isr( osal_semaphore_handle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken );
/**
  * 函    数：创建递归互斥锁
  * 参    数：无
  * 返 回 值：锁句柄，NULL表示失败
  * 说    明：允许同一任务多次获取而不死锁，需与 osal_recursive_mutex_give 配对释放。
  */
osal_mutex_handle_t osal_recursive_mutex_create(void);

/**
  * 函    数：获取递归锁
  * 参    数：xMutex        - 锁句柄
  *           ulTicksToWait - 最大等待时间
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 超时
  * 说    明：若未被占用则获取；若已被当前任务占用，计数加一。
  */
osal_status_t osal_recursive_mutex_take( osal_mutex_handle_t xMutex, uint32_t ulTicksToWait );

/**
  * 函    数：释放递归锁
  * 参    数：xMutex - 锁句柄
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 失败（非持有者释放）
  * 说    明：减少持有计数，当计数归零时真正释放锁。
  */
osal_status_t osal_recursive_mutex_give( osal_mutex_handle_t xMutex );

/**
  * 函    数：任务通知 - 获取（类似信号量）
  * 参    数：ucIndex        - 通知索引（通常为0，除非多个通知）
  *           ucClearOnExit  - 1：获取后清零，0：减一
  *           ulTicksToWait  - 最大等待时间
  * 返 回 值：获取前的通知值
  * 说    明：若通知值为0则任务阻塞等待。封装 ulTaskNotifyTake。
  */
uint32_t osal_task_notify_take( uint8_t ucIndex, uint8_t ucClearOnExit, uint32_t ulTicksToWait );

/**
  * 函    数：任务通知 - 等待消息（携带 32 位值）
  * 参    数：ulBitsToClearOnEntry - 进入前清除哪些位
  *           ulBitsToClearOnExit  - 退出前清除哪些位
  *           pulNotificationValue - 输出：接收到的通知值
  *           ulTicksToWait        - 等待时间
  * 返 回 值：OSAL_OK 成功获取，OSAL_FAIL 超时
  * 说    明：封装 xTaskNotifyWait。
  */
osal_status_t osal_task_notify_wait( uint32_t ulBitsToClearOnEntry,
                                     uint32_t ulBitsToClearOnExit,
                                     uint32_t *pulNotificationValue,
                                     uint32_t ulTicksToWait );

/**
  * 函    数：任务通知 - 发送（任务上下文）
  * 参    数：xTaskToNotify - 目标任务句柄
  *           ucIndex       - 通知索引
  *           ulValue       - 通知值/位
  *           ulAction      - 动作类型：OSAL_NOTIFY_... 宏
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 失败（如动作非法）
  * 说    明：封装 xTaskNotify，根据 ulAction 选择 eNotifyAction。
  */
osal_status_t osal_task_notify( osal_task_handle_t xTaskToNotify, uint8_t ucIndex, uint32_t ulValue, uint32_t ulAction );

/**
  * 函    数：任务通知 - 从中断发送
  * 参    数：xTaskToNotify - 目标任务句柄
  *           ucIndex       - 通知索引
  *           ulValue       - 通知值/位
  *           ulAction      - 动作类型（OSAL_NOTIFY_... 宏）
  *           pxHigherPriorityTaskWoken - 输出：是否需要进行任务切换
  * 返 回 值：OSAL_OK 成功，OSAL_FAIL 失败
  * 说    明：中断安全版本，调用者需在 ISR 末尾统一检查 pxHigherPriorityTaskWoken。
  */
osal_status_t osal_task_notify_from_isr( osal_task_handle_t xTaskToNotify,
                                         uint8_t ucIndex,
                                         uint32_t ulValue,
                                         uint32_t ulAction,
                                         BaseType_t *pxHigherPriorityTaskWoken );

/**
  * 函    数：内存分配
  * 参    数：size - 需要分配的字节数
  * 返 回 值：指向分配内存的指针，NULL表示分配失败
  * 说    明：封装 pvPortMalloc，使用 RTOS 的内存管理机制。
  */
void *osal_malloc(size_t size);

/**
  * 函    数：内存释放
  * 参    数：ptr - 待释放内存的指针
  * 返 回 值：无
  * 说    明：封装 vPortFree，释放之前分配的内存。
  */
void  osal_free(void *ptr);

/**
  * 函    数：启动 OS 调度器
  * 参    数：无
  * 返 回 值：无（正常情况不会返回）
  * 说    明：调用 vTaskStartScheduler，之后由RTOS接管，不再返回。
  */
void osal_start_scheduler(void);

#endif /* _OSAL_H */