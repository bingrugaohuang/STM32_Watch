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
#include "common_macro.h"   /* 包含 I2C 错误码定义 */
#if I2C1_SW_ENABLE

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

#include "protocol_swi2c.h" // 包含 I2C 时序函数和 DWT 延时函数

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
        SW_I2C_SendByte(data[i]); // 发送数据字节
        if(SW_I2C_WaitAck()){
            SW_I2C_Stop();
            return I2C_ERR_NACK_DATA; // 数据字节无应答
        }
    }

    SW_I2C_Stop();

    return I2C_OK; // 临时返回成功
}

/*
 * 函数：I2C1_SW_Read
 * 注意：与写操作不同，读操作需要发送 ACK/NACK 来结束数据传输
 */
static uint8_t I2C1_SW_Read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint8_t len)
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

    SW_I2C_Start(); // 重复起始

    SW_I2C_SendByte((dev_addr << 1) | 0x01); // 发送从机地址（读方向）
    if(SW_I2C_WaitAck()){
        SW_I2C_Stop();
        return I2C_ERR_NACK_ADDR; // 从机无应答
    }
    for(int i = 0; i < len; i++){
        data[i] = SW_I2C_ReadByte(); // 读取数据字节
        SW_I2C_SendAck(i < (len - 1)); // 最后一个字节发送 NACK
    }

    SW_I2C_Stop();

    return I2C_OK; // 临时返回成功
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
    .delay_us = SW_I2C_DelayUs,  // 使用协议层的通用延时函数
    .dma_callback = NULL,  // 软件 I2C 不使用 DMA 回调
};

/* ====================== 公开接口 ====================== */
const I2C_Driver_t * I2C1_SW_GetDriver(void)
{
    return &i2c1_sw_driver;
}

#endif /* I2C1_SW_ENABLE */