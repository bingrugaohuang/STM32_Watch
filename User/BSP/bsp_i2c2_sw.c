#include "common_macro.h"

#if I2C2_SW_ENABLE

#include <stdint.h>
#include "bsp_i2c2.h"
#include "stm32f1xx_hal.h"  /* 寄存器定义，根据你的实际环境调整 */
#include "i2c_interface.h"

/* ====================== 硬件抽象宏定义 ====================== */
#define I2C2_SCL_PIN GPIO_PIN_10
#define I2C2_SDA_PIN GPIO_PIN_11

#define SCL_HIGH() (GPIOB->BSRR = I2C2_SCL_PIN)
#define SCL_LOW()  (GPIOB->BSRR = I2C2_SCL_PIN << 16)
#define SDA_HIGH() (GPIOB->BSRR = I2C2_SDA_PIN)
#define SDA_LOW()  (GPIOB->BSRR = I2C2_SDA_PIN << 16)
#define SDA_READ() ((GPIOB->IDR & I2C2_SDA_PIN) ? 1 : 0)

#include "protocol_swi2c.h" // 包含 I2C 时序函数和 DWT 延时函数

/* ====================== 初始化和接口函数实现 ====================== */
static void I2C2_SW_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = I2C2_SCL_PIN | I2C2_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    DWT_Init(); // 初始化 DWT 用于微秒级延时
}

/*
 * 软件 I2C2 写入函数
 * 参数：
 *   dev_addr: 从机地址
 *   reg:      寄存器地址
 *   data:     要写入的数据指针
 *   len:      要写入的数据长度
 * 返回值：     I2C_Status_t 类型的错误码
 */
static uint8_t I2C2_SW_Write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    SW_I2C_Start();

    SW_I2C_SendByte((dev_addr << 1) | 0x00); // 发送从机地址（写方向）
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }

    SW_I2C_SendByte(reg); // 发送寄存器地址
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_DATA; // 寄存器地址无应答
    }

    for(uint8_t i = 0; i < len; i++){
        SW_I2C_SendByte(data[i]); // 发送数据
        if(SW_I2C_WaitAck()){
            SW_I2C_Stop();
            return I2C_ERR_NACK_DATA; // 数据无应答
        }
    }

    SW_I2C_Stop();
    return I2C_OK; // 写入成功
}

/*
 * 软件 I2C2 读取函数
 * 参数：
 *   dev_addr: 从机地址
 *   reg:      寄存器地址
 *   data:     用于存储读取数据的指针
 *   len:      要读取的数据长度
 * 返回值：     I2C_Status_t 类型的错误码
 */
static uint8_t I2C2_SW_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    SW_I2C_Start();

    SW_I2C_SendByte((dev_addr << 1) | 0x00); // 发送从机地址（写方向）
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }
    SW_I2C_SendByte(reg); // 发送寄存器地址
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_DATA; // 寄存器地址无应答
    }

    SW_I2C_Start(); // 重复起始条件
    SW_I2C_SendByte((dev_addr << 1) | 0x01); // 发送从机地址（读方向）
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }
    for(uint8_t i = 0; i < len; i++){
        data[i] = SW_I2C_ReadByte(); // 读取数据
        if(i < len - 1){
            SW_I2C_SendAck(0); // 发送 ACK
        }
        else{
            SW_I2C_SendAck(1); // 最后一个字节发送 NACK
        }
    }
    SW_I2C_Stop();
    return I2C_OK; // 读取成功
}

/* ====================== I2C 驱动接口实现 ====================== */
static I2C_Driver_t i2c2_sw_driver = {
    .init = I2C2_SW_Init,
    .write = I2C2_SW_Write,
    .read = I2C2_SW_Read,
    .delay_us = SW_I2C_DelayUs,
    .dma_callback = NULL // 软件模拟 I2C 不使用 DMA 回调
};

const I2C_Driver_t* I2C2_SW_GetDriver(void)
{
    return &i2c2_sw_driver;
}

#endif /* I2C2_SW_ENABLE */
