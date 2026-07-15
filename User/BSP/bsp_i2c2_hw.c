#include "common_macro.h"

#if !I2C2_SW_ENABLE

#include "bsp_i2c2.h"
#include "i2c.h"
#include <stdint.h>

/* ====================== 静态函数声明 ====================== */
static uint8_t i2c2_hw_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
static uint8_t i2c2_hw_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);

/* ====================== 驱动实例 ====================== */
static I2C_Driver_t i2c2_hw_driver = {
    .init = MX_I2C2_Init,   // 初始化函数，调用 CubeMX 生成的 I2C2 初始化函数
    .write = i2c2_hw_write, // 写入函数，调用 HAL 库的 I2C 写入函数
    .read = i2c2_hw_read,   // 读取函数，调用 HAL 库的 I2C 读取函数
    .delay_us = NULL,       // 硬件 I2C 不需要软件延时
    .dma_callback = NULL,   // 需要在初始化时设置
};

/* ====================== 公开接口 ====================== */
/* 获取 I2C2 硬件驱动实例指针 */
const I2C_Driver_t * I2C2_HW_GetDriver(void) {
    return &i2c2_hw_driver;
}

static uint8_t i2c2_hw_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len) {
    // 使用 HAL 库的 I2C 写入函数
    if (HAL_I2C_Mem_Write(&hi2c2, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, 10) == HAL_OK) {
        return 0; // 成功
    } else {
        return 1; // 失败
    }
}

static uint8_t i2c2_hw_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len) {
    // 使用 HAL 库的 I2C 读取函数
    if (HAL_I2C_Mem_Read(&hi2c2, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, 10) == HAL_OK) {
        return 0; // 成功
    } else {
        return 1; // 失败
    }
}

#endif