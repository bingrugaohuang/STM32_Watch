#include "i2c_interface.h"
#include "bsp_i2c1_sw.h"
#include "osal.h"
#include "i2c_test.h"

#define OLED_ADDR 0x3C
#define OLED_CMD  0x00
#define OLED_DATA 0x40

#define I2C1_SW_ENABLE 1 /* 定义此宏以启用软件 I2C1 实现，注释掉以使用硬件 I2C1 */

/* 获取软件 I2C1 驱动实例的指针 */
static const I2C_Driver_t *i2c1 = NULL;

static osal_task_handle_t I2C_test_handle;

/* ========================== 私有函数 ========================== */
static void I2C_SW_test(void *pvParameters)
{
    (void)pvParameters; // 避免未使用参数的编译警告

     /* 发送测试：每秒发送一次 OLED 关闭显示命令 */
    while(1)
    {
        // uint8_t cmd = 0xAE;
        // i2c1->write(OLED_ADDR, OLED_CMD, &cmd, 1); // 发送 OLED 关闭显示命令
        osal_task_delay(1); // 延时 1ms
    }
  
}

/* ========================== 公共函数 ========================== */
void I2C_Init(void)
{
    i2c1 = I2C1_SW_GetDriver();

#if I2C1_SW_ENABLE
    i2c1->init();   // 初始化软件 I2C1
#else
    MX_I2C1_Init(); // 硬件 I2C1 初始化
#endif

    I2C_test_handle = osal_task_create("I2C_SW_test", I2C_SW_test, 256, NULL, 3);
}
