#include "common_macro.h"   /* 包含 I2C 错误码定义 */

#if !I2C1_SW_ENABLE

#include "bsp_i2c1_hw.h"
#include "i2c.h"


/* ====================== 函数声明 ====================== */
static uint8_t i2c1_hw_write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
static uint8_t i2c1_hw_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);

/* ====================== 驱动实例 ====================== */
static I2C_Driver_t i2c1_hw_driver = {
    .delay_us = NULL,          // 硬件 I2C 不需要软件延时
    .init     = MX_I2C1_Init,  // 直接使用 HAL 库的初始化函数
    .read     = i2c1_hw_read,
    .write    = i2c1_hw_write,
    .dma_callback = NULL,  // 需要在初始化时设置
};

/* ====================== 公开接口 ====================== */
/* 获取 I2C1 硬件驱动实例指针 */
const I2C_Driver_t * I2C1_HW_GetDriver(void)
{
    return &i2c1_hw_driver;
}

/* 传输完成回调注册函数 */
void I2C1_HW_SetDMATxCplt_Callback(I2C1_DMACplt_Callback_t cb)
{
    i2c1_hw_driver.dma_callback = cb;
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
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, dev_addr << 1, reg, I2C_MEMADD_SIZE_8BIT, data, len, I2C_TIMEOUT);
    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) return I2C_ERR_TIMEOUT;
        else return I2C_ERR_BUS;  // 或其他自定义错误码
    }
    return I2C_OK;
}

// I2C DMA 传输完成回调函数，HAL 库会在 DMA 传输完成后调用
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if(hi2c->Instance == I2C1)
    {
        if(i2c1_hw_driver.dma_callback)
        {
            i2c1_hw_driver.dma_callback();
        }
    }
}

#endif /* !I2C1_SW_ENABLE */