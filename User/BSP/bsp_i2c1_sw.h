

/*
 * i2c1_sw.h
 * 软件 I2C1 的 BSP 头文件 — 公开接口声明与引脚定义
 *
 * 功能：使用 GPIO 模拟 I2C 时序，驱动 I2C1 总线（PB6-SCL, PB7-SDA）
 * 
 * I2C1 引脚分配（按 STM32F103C8T6 默认映射）：
 *   PB6 - SCL
 *   PB7 - SDA
 */
#include "common_macro.h"

#if I2C1_SW_ENABLE

#ifndef I2C1_SW_H
#define I2C1_SW_H

#include "i2c_interface.h"


/*********************** 公开接口声明 ***********************/
/*
 * 获取软件 I2C1 驱动实例的指针
 * 调用示例：
 *   I2C_Driver_t *i2c_drv = I2C1_SW_GetDriver();
 *   i2c_drv->init();
 *   i2c_drv->write(0x3C, 0x00, buf, 1);
 *
 * 返回值：指向静态全局 I2C_Driver_t 实例的指针
 */
const I2C_Driver_t * I2C1_SW_GetDriver(void);

#endif /* I2C1_SW_H */

#endif /* I2C1_SW_ENABLE */
