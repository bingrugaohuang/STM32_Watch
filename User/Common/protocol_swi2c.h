#include "common_macro.h"
#if I2C1_SW_ENABLE || I2C2_SW_ENABLE

#ifndef PROTOCOL_SWI2C_H
#define PROTOCOL_SWI2C_H

#include <stdint.h>
#include "stm32f1xx_hal.h"  /* 寄存器定义，根据你的实际环境调整 */

/* 
 * 注意：使用本文件前，调用者（bsp_i2cX_sw.c）必须在 #include 本文件之前，
 * 定义以下 5 个宏和 1 个延时函数指针（或函数）：
 *   - SCL_HIGH()、SCL_LOW()
 *   - SDA_HIGH()、SDA_LOW()
 *   - SDA_READ()
 *   - SW_DELAY_US(us)   (例如 #define SW_DELAY_US  I2C1_SW_DelayUs)
 */

/* I2C 时序延时参数（72MHz 下，约 5us 半个周期得到约 100kHz 时钟） */
#define I2C_DELAY_HALF_CYCLE_US   5
#define CYCLES_PER_US             (72)

/* ====================== 延时实现 ====================== */
//DWT 初始化
static inline void DWT_Init(void)
{
    if((!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 使能 DWT
    }
    DWT->CYCCNT = 0;                     // 清零计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // 使能周期计数器
}

// 通过 DWT 周期计数器实现微秒级延时
static inline void SW_I2C_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * CYCLES_PER_US; // 计算需要的周期数
    while(DWT->CYCCNT - start < cycles);  // 等待直到达到所需周期数
}

/* ====================== 传输辅助函数 ====================== */
static inline void SW_I2C_Start(void) 
{
    SDA_HIGH();
    SCL_HIGH();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SDA_LOW();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    SCL_LOW();
}

static inline void SW_I2C_Stop(void) 
{
    SCL_LOW();
    SDA_LOW();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_HIGH();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    SDA_HIGH();
}

static inline void SW_I2C_SendByte(uint8_t byte) 
{
    for(int i = 0; i < 8; i++) {
        SCL_LOW();
        if(byte & 0x80) SDA_HIGH(); else SDA_LOW();
        byte <<= 1;
        SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
        SCL_HIGH();
        SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    }
    SCL_LOW();
}

static inline uint8_t SW_I2C_ReadByte(void) 
{
    uint8_t byte = 0;
    SDA_HIGH(); // 释放总线
    for(int i = 0; i < 8; i++) {
        SCL_LOW();
        SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
        SCL_HIGH();
        SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
        byte <<= 1;
        if(SDA_READ()) byte |= 0x01;
    }
    SCL_LOW();
    return byte;
}

static inline uint8_t SW_I2C_WaitAck(void) 
{
    uint8_t ack;
    SCL_LOW();
    SDA_HIGH();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    SCL_HIGH();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    ack = SDA_READ();
    SCL_LOW();
    return ack;
}

static inline void SW_I2C_SendAck(uint8_t ack) 
{
    SCL_LOW();
    //ack = 0 发送 ACK，ack = 1 发送 NACK
    if(ack) SDA_HIGH(); else SDA_LOW();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    SCL_HIGH();
    SW_I2C_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    SCL_LOW();
}

#endif /* PROTOCOL_SWI2C_H */

#endif /* I2C1_SW_ENABLE || I2C2_SW_ENABLE */