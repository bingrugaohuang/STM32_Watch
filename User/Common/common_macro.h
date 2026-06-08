#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H

/* ================================================================
 *  通用宏定义
 * 该头文件包含项目中所有模块通用使用的宏定义，如错误码、状态码等。
 * ================================================================ */
#define COMMON_ERR_OK           0   /* 成功 */
#define COMMON_ERR_PARAM       -1   /* 参数错误（空指针、长度非法等） */
#define COMMON_ERR_MEM         -2   /* 内存不足（分配失败） */
#define COMMON_ERR_QUEUE_FULL  -3   /* 队列/缓冲满，无法投递 */

/* I2C 相关错误码宏定义 */
#define I2C1_TIMEOUT           30

/* I2C 错误码定义 */
typedef enum{
    I2C_OK = 0,
    I2C_ERR_NACK_ADDR,
    I2C_ERR_NACK_DATA,
    I2C_ERR_TIMEOUT,
    I2C_ERR_BUS
}I2C_Status_t;

/* 条件编译相关宏定义 */
#define I2C1_SW_ENABLE         0   /* 1 = 使用软件模拟 I2C1，0 = 使用硬件 I2C1 */
#define I2C1_OLED_TEST_ENABLE  1   /* 1 = 使能 OLED I2C test, 0 = disable */


#endif