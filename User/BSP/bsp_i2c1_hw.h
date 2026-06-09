#ifndef BSP_I2C1_HW_H
#define BSP_I2C1_HW_H

#include "i2c_interface.h"
#include "common_macro.h"

#if !I2C1_SW_ENABLE



/* ====================== 公开接口 ====================== */
/*
 * 获取硬件 I2C1 驱动实例的指针
 * 调用示例：
 *   I2C_Driver_t *i2c_drv = I2C1_HW_GetDriver();
 *   i2c_drv->init();
 *   i2c_drv->write(0x3C, 0x00, buf, 1);
 *
 * 返回值：指向静态全局 I2C_Driver_t 实例的指针
 */
const I2C_Driver_t * I2C1_HW_GetDriver(void);

/*
 * 设置 I2C1 DMA 传输完成回调函数
 * 参 数：cb - 回调函数指针
 * 说 明：在 I2C1 DMA 传输完成后被调用
 */
void I2C1_HW_SetDMATxCplt_Callback(I2C1_DMACplt_Callback_t cb);

#endif /* !I2C1_SW_ENABLE */

#endif /* BSP_I2C1_HW_H */

