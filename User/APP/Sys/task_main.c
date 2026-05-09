/*
 * task_main.c
 * FreeRTOS 应用入口文件
 * 负责创建初始任务、IPC 对象并启动调度器
 */
#include "osal.h"
#include "log.h"
//#include "app_tasks.h"   /* 用户自定义的应用任务头文件 */

/* ---------- 可能用到的全局对象句柄（若需要跨文件可声明 extern） ---------- */
static osal_queue_handle_t  g_xxx_queue;    /* 示例队列 */
static osal_mutex_handle_t  g_xxx_mutex;    /* 示例递归锁 */

/* ---------- 私有函数声明 ---------- */
static void prvCreateObjects(void);   /* 创建 IPC 对象 */
static void prvCreateTasks(void);     /* 创建系统任务 */

/**
  * 函    数：应用初始化入口
  * 参    数：无
  * 返 回 值：无
  * 说    明：在 main.c 硬件初始化完成后被调用。
  *           流程：1. OSAL 层初始化
  *                 2. 创建内核对象（队列、锁等）
  *                 3. 创建各个应用任务
  *                 4. 启动调度器（此后不再返回）
  */
void APP_Init(void)
{
    /* 步骤1：OSAL 初始化（目前可省） */
    osal_init();

    /* 步骤2：创建系统所需的 IPC 对象 */
    prvCreateObjects();

    /* 步骤3：创建各个任务（如传感器采集、通信处理、UI 等） */
    prvCreateTasks();

    /* 步骤4：启动调度器，正常情况不会返回 */
    osal_start_scheduler();

    /* 若返回说明启动失败，卡死在错误处理 */
    while(1) {
        /* 可加入错误指示 */
    }
}

/**
  * 函    数：创建全局 IPC 对象
  * 参    数：无
  * 返 回 值：无
  * 说    明：在此函数中创建所有任务间通信所需的队列、信号量、互斥锁等。
  *           示例：
  *             g_xxx_queue = osal_queue_create( 10, sizeof(MyMsgType) );
  *             g_xxx_mutex = osal_recursive_mutex_create();
  *           注意：需检查句柄有效性，若创建失败可进入错误处理。
  */
static void prvCreateObjects(void)
{
    /* 创建队列、递归锁等 */
}

/**
  * 函    数：创建应用任务
  * 参    数：无
  * 返 回 值：无
  * 说    明：调用 osal_task_create 逐个创建任务。
  *           每个任务应具有明确的优先级和合适的栈大小。
  *           示例：
  *             osal_task_create( "Sensor", vTaskSensor, 256, NULL, 2 );
  *             osal_task_create( "Comm",   vTaskComm,   512, NULL, 3 );
  */
static void prvCreateTasks(void)
{
    /* 创建日志后台任务，该任务负责异步输出所有日志 */

    /* 创建其他应用任务 */
    // osal_task_create( "Sensor", vTask_Sensor, 256, NULL, 2 );
    // ...

}