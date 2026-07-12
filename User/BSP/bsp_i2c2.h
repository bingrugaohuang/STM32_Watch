#ifndef BSP_I2C2_H
#define BSP_I2C2_H

#include "common_macro.h"
#include "i2c_interface.h"

#if I2C2_SW_ENABLE

/*********************** 公开接口声明 ***********************/
/*
 * 获取软件 I2C2 驱动实例的指针
 * 调用示例：
 *   I2C_Driver_t *i2c_drv = I2C2_SW_GetDriver();
 *   i2c_drv->init();
 *   i2c_drv->write(0x3C, 0x00, buf, 1);
 *
 * 返回值：指向静态全局 I2C_Driver_t 实例的指针
 */
const I2C_Driver_t * I2C2_SW_GetDriver(void);

#else 

/*********************** 公开接口声明 ***********************/
/*
 * 获取硬件 I2C2 驱动实例的指针
 * 调用示例：
 *   I2C_Driver_t *i2c_drv = I2C2_HW_GetDriver();
 *   i2c_drv->init();
 *   i2c_drv->write(0x3C, 0x00, buf, 1);
 *
 * 返回值：指向静态全局 I2C_Driver_t 实例的指针
 */
const I2C_Driver_t * I2C2_HW_GetDriver(void);

#endif 

#endif 