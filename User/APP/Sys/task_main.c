/*
 * task_main.c
 * FreeRTOS 应用入口文件
 * 负责创建初始任务、IPC 对象并启动调度器
 */
#include "osal.h"           /* 包含 OSAL 抽象层 */
#include "log.h"            /* 包含日志模块 */
#include "serial.h"
#include "button_service.h" /* 包含按钮服务模块 */
#include "hardfault_debug.h"

//测试
#include "i2c_test.h"

/* ---------- 私有函数声明 ---------- */
static void prvCreateObjects(void);   /* 创建 IPC 对象 */
static void prvCreateTasks(void);     /* 创建系统任务 */

/**
  * 函    数：系统底层总初始化函数
  * 参    数：无
  * 返 回 值：无
  * 说    明：在 APP_Init 中被调用，负责完成所有底层初始化工作。
  *           包括但不限于：
  *             - OSAL 层初始化（如 osal_init()）
  *             - 硬件外设初始化（如 I2C、OLED、传感器等）
  *             - 驱动层初始化（如串口、按键等）
  *           注意：此函数应在创建 IPC 对象和任务之前调用，以确保底层资源准备就绪。
  */
static void total_init(void)
{
  check_crash_log_on_startup();  /* 最先检查上次 HardFault 是否有 FLASH 崩溃日志 */
  serial_init();           // Driver 层（创建串口锁）
  log_init();              // Service 层（创建日志锁、队列，设置后端）
  button_service_init();   // Service 层（创建按键事件队列，配置按键控制块）
  I2C_Init();              // Driver 层（初始化 I2C 硬件或软件实现）
  hardfault_debug_init();  // MiddleWares/Debug（配置相关寄存器，准备 HardFault 调试）
  LOG_I("MAIN", "System boot...");
}

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
    /* 步骤1：驱动等底层初始化 */
    //osal_init();
    total_init(); /* 包含 osal_init 和其他底层初始化，如 I2C、OLED、传感器等 */

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

/**
 * 函   数： FreeRTOS 栈溢出钩子函数
 * 参   数： 发生溢出的任务句柄
 *           发生溢出的任务名称
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* 1. 打印是哪个任务炸了栈（非常关键的信息！） */
    printf("FATAL ERROR: Stack Overflow in task: %s\r\n", pcTaskName);

    /* 2. 既然已经栈溢出了，系统状态已经不可控，直接死循环 */
    /* 配合你之前写的硬错误处理，这里也可以触发一个断言或者直接禁用中断 */
    __disable_irq(); 
    while (1) 
    {
        // 停在这里，你可以用调试器查看 pcTaskName 的值
    }
}