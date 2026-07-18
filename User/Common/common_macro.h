#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

extern void Error_Handler(void);   /* 错误处理函数，定义在 main.c 中 */

/* ================================================================
 *  通用宏定义
 * 该头文件包含项目中所有模块通用使用的宏定义，如错误码、状态码等。
 * ================================================================ */
#define COMMON_ERR_OK           0   /* 成功 */
#define COMMON_ERR_PARAM        1   /* 参数错误（空指针、长度非法等） */
#define COMMON_ERR_MEM          2   /* 内存不足（分配失败） */
#define COMMON_ERR_QUEUE_FULL   3   /* 队列/缓冲满，无法投递 */
#define COMMON_ERR_TIMEOUT      4   /* 操作超时（如等待队列时） */
#define COMMON_ERR_NULL         5   /* 空指针错误 */

/* 条件编译相关宏定义 */
#define I2C1_SW_ENABLE         0   /* 1 = 使用软件模拟 I2C1，0 = 使用硬件 I2C1 */
#define I2C1_OLED_TEST_ENABLE  0   /* 1 = 使能 OLED I2C test, 0 = disable */
#define I2C2_SW_ENABLE         0   /* 1 = 使用软件模拟 I2C2，0 = 使用硬件 I2C2 */   

#define STACK_MONITOR_ENABLE   1   /* 1 = 使能任务栈高水位监控，0 = 禁用 */

#define USE_HARDFAULT_HANDLER  1   /* 1 = 使用自定义硬件故障处理，0 = 不使用 */

#define MPU_MOT_TEST_ENABLE    1   /* 1 = 使能 MPU6050 运动检测耗时测试，0 = 禁用 */

/* 断言*/
#define G_ASSERT( x )    do { if (!(x)) { /* 可选：添加错误处理或日志输出 */Error_Handler();/* return;*/ } } while (0)

/* 任务相关定义*/
//栈分配
#define DISPLAYTASK_STACK      1 * 128 + 64
#define MPU6050TASK_STACK      1 * 128 + 64
//优先级分配
#define DISPLAYTASK_PRIORITY   3
#define MPU6050TASK_PRIORITY   4
/* I2C 相关错误码宏定义 */
#define I2C_TIMEOUT           30

/* I2C 错误码定义 */
typedef enum{
    I2C_OK = 0,
    I2C_ERR_NACK_ADDR,
    I2C_ERR_NACK_DATA,
    I2C_ERR_TIMEOUT,
    I2C_ERR_BUS
}I2C_Status_t;



#endif