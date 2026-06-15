#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

extern void Error_Handler(void);   /* 错误处理函数，定义在 main.c 中 */

/* ================================================================
 *  通用宏定义
 * 该头文件包含项目中所有模块通用使用的宏定义，如错误码、状态码等。
 * ================================================================ */
#define COMMON_ERR_OK           0   /* 成功 */
#define COMMON_ERR_PARAM       -1   /* 参数错误（空指针、长度非法等） */
#define COMMON_ERR_MEM         -2   /* 内存不足（分配失败） */
#define COMMON_ERR_QUEUE_FULL  -3   /* 队列/缓冲满，无法投递 */

/* 条件编译相关宏定义 */
#define I2C1_SW_ENABLE         0   /* 1 = 使用软件模拟 I2C1，0 = 使用硬件 I2C1 */
#define I2C1_OLED_TEST_ENABLE  0   /* 1 = 使能 OLED I2C test, 0 = disable */

/* 断言*/
#define G_ASSERT( x )    do { if (!(x)) { /* 可选：添加错误处理或日志输出 */Error_Handler();/* return;*/ } } while (0)

/* 任务相关定义*/
//栈分配
#define DISPLAYTASK_STACK      1024

//优先级分配
#define DISPLAYTASK_PRIORITY   4



#endif