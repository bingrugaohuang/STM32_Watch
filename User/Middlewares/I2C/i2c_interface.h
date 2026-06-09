/*
 * i2c_interface.h
 * I2C 抽象接口定义
 * 
 * 设计目标：
 * 所有 I2C 设备驱动（OLED / MPU6050 等）依赖此接口，
 * 而不依赖具体的硬件实现（软件模拟 / 硬件I2C1 / 硬件I2C2）。
 * 更换底层实现时，上层代码无需任何修改。
 */

#ifndef I2C_INTERFACE_H
#define I2C_INTERFACE_H

#include <stdint.h>

typedef void (*I2C1_DMACplt_Callback_t)(void);

/*
 * I2C 驱动抽象结构体
 * 
 * 每个具体的 I2C 实现（软件模拟 / 硬件I2C）需要：
 *   1. 定义一个 static 的函数，匹配下面的函数指针类型
 *   2. 创建一个 I2C_Driver_t 实例，将函数指针指向这些 static 函数
 *   3. 在上层设备驱动初始化时，将该实例的地址传入
 */
typedef struct {
    void     (*init)(void);
    uint8_t  (*write)(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
    uint8_t  (*read)(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
    void     (*delay_us)(uint32_t us);
    void     (*dma_callback)(void);
//  I2C1_DMACplt_Callback_t dma_callback;  // 可选的 DMA 传输完成回调函数指针
} I2C_Driver_t;

#endif /* I2C_INTERFACE_H */