#include "common_macro.h"   /* 包含 I2C 错误码定义 */
#include "bsp_i2c1_hw.h"
#include "i2c.h"

#if !I2C1_SW_ENABLE

/* ====================== 函数声明 ====================== */
static uint8_t i2c1_hw_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
static uint8_t i2c1_hw_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);

/* ====================== 驱动实例 ====================== */
static I2C_Driver_t i2c1_hw_driver = {
    .delay_us = NULL,          // 硬件 I2C 不需要软件延时
    .init     = MX_I2C1_Init,  // 直接使用 HAL 库的初始化函数
    .read     = i2c1_hw_read,
    .write    = i2c1_hw_write,
};

/* ====================== 公开接口 ====================== */
const I2C_Driver_t * I2C1_HW_GetDriver(void)
{
    return &i2c1_hw_driver;
}

/* ====================== 传输接口实现 ====================== */
static uint8_t i2c1_hw_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Write_DMA(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len);
    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) return I2C_ERR_TIMEOUT;
        else return I2C_ERR_BUS;  // 或其他自定义错误码
    }
    return I2C_OK;
}

static uint8_t i2c1_hw_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, I2C1_TIMEOUT);
    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) return I2C_ERR_TIMEOUT;
        else return I2C_ERR_BUS;  // 或其他自定义错误码
    }
    return I2C_OK;
}

#endif /* !I2C1_SW_ENABLE */