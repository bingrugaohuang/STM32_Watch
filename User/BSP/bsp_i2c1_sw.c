/*
 * i2c1_sw.c
 * 软件 I2C1 的 BSP 实现 — 使用 GPIO 模拟 I2C 总线时序
 *
 * 芯片：STM32F103C8T6，系统主频 72MHz
 * 总线速度目标：标准模式 100kHz（如需快速模式 400kHz，可自行调整延时参数）
 * 引脚：PB6（SCL）、PB7（SDA）
 *
 * 关键硬件知识点（请查阅参考手册寄存器章节完成实现）：
 *   - GPIO 模式配置：CRL（PB6, PB7 在低 8 位，用 CRL）
 *   - 输出控制：BSRR 寄存器进行原子位操作（避免 ODR 的读-改-写竞态问题）
 *   - 输入读取：IDR 寄存器
 *   - 时钟使能：RCC_APB2ENR 的 IOPB 位
 *   - 延时实现：用 DWT_CYCCNT 或精确 NOP 循环（不可用 SysTick，与 FreeRTOS 冲突）
 *   - 引脚配置建议：开漏输出模式，以兼容 I2C 总线的线与特性并避免电平冲突
 *   - 确保外部硬件已连接 4.7kΩ 上拉电阻至 3.3V
 */

#include "bsp_i2c1_sw.h"
#include "stm32f1xx_hal.h"  /* 寄存器定义，根据你的实际环境调整 */

/* ====================== 硬件抽象宏定义 ====================== */
//引脚操作宏
#define SCL_HIGH()  (GPIOB->BSRR = I2C1_SCL_PIN)
#define SCL_LOW()   (GPIOB->BRR  = I2C1_SCL_PIN) 
#define SDA_HIGH()  (GPIOB->BSRR = I2C1_SDA_PIN)
#define SDA_LOW()   (GPIOB->BRR  = I2C1_SDA_PIN)
#define SDA_READ()  ((GPIOB->IDR & I2C1_SDA_PIN) ? 1 : 0)

//GPIO 模式配置相关宏
#define I2C1_SCL_PIN GPIO_PIN_6
#define I2C1_SDA_PIN GPIO_PIN_7

/* I2C 时序延时参数（72MHz 下，约 5us 半个周期得到约 100kHz 时钟） */
#define I2C_DELAY_HALF_CYCLE_US   5
#define CYCLES_PER_US (72)

/* ====================== 静态函数声明 ====================== */
/* 
 * 以下 4 个函数对应 I2C_Driver_t 接口。
 */
static void     I2C1_SW_Init(void);
static uint8_t  I2C1_SW_Write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
static uint8_t  I2C1_SW_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len);
static void     I2C1_SW_DelayUs(uint32_t us);

/* ====================== 传输辅助函数 ====================== */
/*
 * 函数：I2C_Start
 */
static void I2C_Start(void)
{
    SDA_HIGH();
    SCL_HIGH();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SDA_LOW();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_LOW();
    //I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);
}

/*
 * 函数：I2C_Stop
 */
static void I2C_Stop(void)
{
    SCL_LOW();
    SDA_LOW();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_HIGH();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SDA_HIGH();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);
}

/*
 * 函数：I2C_WaitAck
 */
static uint8_t I2C_WaitAck(void)
{
    SCL_LOW();
    SDA_HIGH(); // 释放 SDA 线
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_HIGH();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    uint8_t ack = SDA_READ(); // 读取 ACK 位

    SCL_LOW();
    ///I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    return ack;
}

/*
 * 函数：I2C_SendByte
 */
static void I2C_SendByte(uint8_t byte)
{
    for(int i = 0; i < 8; i++)
    {
        SCL_LOW();
        //I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

        if(byte & 0x80){ // 发送最高位
            SDA_HIGH();
        }
        else{
            SDA_LOW();
        }
        byte <<= 1; // 移出已发送的最高位
        I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

        SCL_HIGH();
        I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);
    }
    SCL_LOW();
}

/*
 * 函数：I2C_ReadByte
 */
static uint8_t I2C_ReadByte(void)
{
    uint8_t byte = 0;
    SDA_HIGH(); // 释放 SDA 线

    for(int i = 0; i < 8; i++)
    {
        SCL_LOW();
        I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

        SCL_HIGH();
        I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

        byte <<= 1; // 为新位腾出空间
        if(SDA_READ()){
            byte |= 0x01; // 读到 1
        }
    }
    SCL_LOW();
    return byte;
}

/*
 * 函数：I2C_SendAck
 */
static void I2C_SendAck(uint8_t ack)
{
    SCL_LOW();
    if(ack){
        SDA_LOW(); // 发送 ACK（0）
    }
    else{
        SDA_HIGH(); // 发送 NACK（1）
    }
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_HIGH();
    I2C1_SW_DelayUs(I2C_DELAY_HALF_CYCLE_US);

    SCL_LOW();
}

/* ====================== 驱动实例（全局） ====================== */
/*
 * 这是当前文件的核心数据结构。
 * 所有上层驱动都通过 I2C1_SW_GetDriver() 获取它的指针。
 * 它的四个函数指针指向本文件的静态函数。
 */
static I2C_Driver_t i2c1_sw_driver = {
    .init     = I2C1_SW_Init,
    .write    = I2C1_SW_Write,
    .read     = I2C1_SW_Read,
    .delay_us = I2C1_SW_DelayUs,
};

/* ====================== 公开接口 ====================== */
const I2C_Driver_t * I2C1_SW_GetDriver(void)
{
    return &i2c1_sw_driver;
}

/* ====================== 延时实现 ====================== */
/*
 * 函数：DWT初始化
 */
static void DWT_Init(void)
{
    if((!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 使能 DWT
    }
    DWT->CYCCNT = 0;                     // 清零计数器
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // 使能周期计数器
}

/*
 * 函数：I2C1_SW_DelayUs,通过 DWT 周期计数器实现微秒级延时
 * 说明: 如从机有超时限制，可以短暂屏蔽 SysTick 中断（不是全局中断） 
 */
static inline void I2C1_SW_DelayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * CYCLES_PER_US; // 计算需要的周期数
    while(DWT->CYCCNT - start < cycles);  // 等待直到达到所需周期数
    // for(int i = 0; i < 72 * us; i++){
    //     __nop;
    // }
}

/* ====================== 硬件初始化 ====================== */
/*
 * 函数：I2C1_SW_Init
 * 说明：配置 GPIO 引脚为开漏输出，初始化 DWT 用于微秒级延时
 *      使用条件编译
 */
static void I2C1_SW_Init(void)
{
    // TODO: 实现 —— 请查阅 RCC 和 GPIO 章节
    GPIO_InitTypeDef GPIO_Initstruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE(); // 使能 GPIOB 时钟

    GPIO_Initstruct.Pin  = I2C1_SCL_PIN | I2C1_SDA_PIN;
    GPIO_Initstruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Initstruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_Initstruct);
    DWT_Init(); // 初始化 DWT 用于微秒级延时
}

/* ====================== 传输接口实现 ====================== */
/*
 * 函数：I2C1_SW_Write
 * 注意：任何一步 ACK 失败，立即发送 STOP 并返回错误码
 */
static uint8_t I2C1_SW_Write(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{
    I2C_Start();

    I2C_SendByte((dev_addr << 1) | 0); // 发送从机地址（写方向）
    if(I2C_WaitAck()){
        I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }

    I2C_SendByte(reg); // 发送寄存器地址
    if(I2C_WaitAck()){
        I2C_Stop();
        return I2C_ERR_NACK_DATA; // 寄存器地址无应答
    }
    
    for(uint8_t i = 0; i < len; i++){
        I2C_SendByte(data[i]); // 发送数据字节
        if(I2C_WaitAck()){
            I2C_Stop();
            return I2C_ERR_NACK_DATA; // 数据字节无应答
        }
    }

    I2C_Stop();

    return I2C_OK; // 临时返回成功
}

/*
 * 函数：I2C1_SW_Read
 * 注意：与写操作不同，读操作需要发送 ACK/NACK 来结束数据传输
 */
static uint8_t I2C1_SW_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
{

    I2C_Start();

    I2C_SendByte((dev_addr << 1) | 0); // 发送从机地址（写方向）
    if(I2C_WaitAck()){
        I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }

    I2C_SendByte(reg); // 发送寄存器地址
    if(I2C_WaitAck()){
        I2C_Stop();
        return I2C_ERR_NACK_DATA; // 寄存器地址无应答
    }

    I2C_Start(); // 重复起始

    for(int i = 0; i < len; i++){
        data[i] = I2C_ReadByte(); // 读取数据字节
        I2C_SendAck(i < (len - 1)); // 最后一个字节发送 NACK
    }

    I2C_Stop();

    return 0; // 临时返回成功
}