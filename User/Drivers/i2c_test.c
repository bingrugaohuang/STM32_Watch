#include "i2c_interface.h"
#include "bsp_i2c1_sw.h"
#include "oled_driver.h"
#include "oled_common.h"
#include "oled_strings_gbk.h"
#include "osal.h"
#include "log.h"
#include "i2c_test.h"

/* ================== 模块标签（用于日志系统） ================== */
#define TAG "TEST"

/* ================== 宏定义 ================== */
// #define I2C1_SW_ENABLE  1   /* 1 = software I2C1, 0 = hardware I2C1 */

/* ================== 静态变量 ================== */
// static const I2C_Driver_t *i2c1 = NULL;
static osal_task_handle_t   test_handle;

/* ================== OLED功能测试 ================== */

static void OLED_ComprehensiveTest(void)
{
    LOG_I(TAG, "==== OLED functional test start ====");

    // /* ---- Test 1: 全屏填充 ---- */
    // LOG_I(TAG, "[Test 1] Full screen fill -> clear -> refresh");
    // OLED_Clear();
    // OLED_DrawRectangle(0, 0, 128, 64, OLED_FILLED);
    // OLED_Update();
    // osal_task_delay(5000);
    // OLED_Clear();
    // OLED_Update();
    // osal_task_delay(500);

    // /* ---- Test 2: ASCII 文本 ---- */
    // LOG_I(TAG, "[Test 2] ASCII text display");
    // OLED_ShowString(0, 0,  "Hello World!",  OLED_8X16);
    // OLED_ShowString(0, 16, "STM32F103C8T6", OLED_6X8);
    // OLED_ShowString(0, 32, "0123456789",    OLED_8X16);
    // OLED_Update();
    // osal_task_delay(5000);

    /* ---- Test 3: 中文 (依赖于 GBK 字符集) ---- */
    // LOG_I(TAG, "[Test 3] Chinese text display");
    // OLED_Clear();
    // OLED_ShowString(0, 0,  (char *)str_hello,            OLED_8X16);
    // OLED_ShowString(0, 24, (char *)str_oled_driver,      OLED_8X16);
    // OLED_ShowString(0, 48, (char *)str_freertos_watch,   OLED_6X8);
    // OLED_Update();
    // osal_task_delay(20000);

    // /* ---- Test 4: 数字显示 ---- */
    // LOG_I(TAG, "[Test 4] Number display");
    // OLED_Clear();
    // OLED_ShowNum(0, 0,  12345, 5, OLED_8X16);
    // OLED_ShowSignedNum(0, 20, -1234, 4, OLED_8X16);
    // OLED_ShowHexNum(0, 40, 0xABCD, 4, OLED_8X16);
    // OLED_ShowBinNum(0, 56, 0xA5, 8, OLED_6X8);
    // OLED_Update();
    // osal_task_delay(5000);

    // /* ---- Test 5: 浮点数显示 ---- */
    // LOG_I(TAG, "[Test 5] Float number display");
    // OLED_Clear();
    // OLED_ShowFloatNum(0, 0,  3.14159, 1, 4, OLED_8X16);
    // OLED_ShowFloatNum(0, 20, -2.71828, 1, 4, OLED_8X16);
    // OLED_ShowFloatNum(0, 40,  0.00000, 1, 3, OLED_8X16);
    // OLED_Update();
    // osal_task_delay(5000);

    // /* ---- Test 6: 输出格式显示 ---- */
    // LOG_I(TAG, "[Test 6] OLED_Printf formatted output");
    // OLED_Clear();
    // OLED_Printf(0, 0,  OLED_8X16, "Tick: %lu",  osal_get_tick());
    // OLED_Printf(0, 20, OLED_6X8,  "Heap: %u",   (unsigned)xPortGetFreeHeapSize());
    // OLED_Printf(0, 32, OLED_6X8,  "Time: %dms", 12345);
    // OLED_Update();
    // osal_task_delay(5000);

    // /* ---- Test 7: 图形绘制 ---- */
    // LOG_I(TAG, "[Test 7] Shape drawing");
    // OLED_Clear();
    // OLED_DrawLine(0, 0, 127, 63);
    // OLED_DrawLine(0, 63, 127, 0);
    // OLED_DrawRectangle(10, 10, 30, 20, OLED_UNFILLED);
    // OLED_DrawRectangle(50, 10, 30, 20, OLED_FILLED);
    // OLED_DrawCircle(100, 20, 8, OLED_UNFILLED);
    // OLED_DrawCircle(100, 50, 8, OLED_FILLED);
    // OLED_DrawTriangle(10, 50, 40, 30, 70, 55, OLED_UNFILLED);
    // OLED_Update();
    // osal_task_delay(5000);

    // /* ---- Test 8: 进度条动画 ---- */
    // LOG_I(TAG, "[Test 8] Progress bar animation");
    // OLED_Clear();
    // for (uint8_t i = 0; i <= 100; i++)
    // {
    //     OLED_ClearArea(10, 25, 108, 14);
    //     OLED_DrawRectangle(10, 25, 108, 14, OLED_UNFILLED);
    //     OLED_DrawRectangle(14, 29, i, 6, OLED_FILLED);
    //     OLED_ShowNum(50, 45, i, 3, OLED_6X8);
    //     OLED_ShowString(68, 45, "%", OLED_6X8);
    //     OLED_UpdateArea(10, 25, 108, 30);
    //     osal_task_delay(20);
    // }
    // osal_task_delay(500);

    /* ---- Test complete ---- */
    LOG_I(TAG, "[Test 9] Final screen - test complete");
    OLED_Clear();
    OLED_ShowString(10, 0,  "OLED Test",                OLED_8X16);
    OLED_ShowString(10, 20, "All tests",                 OLED_6X8);
    //OLED_ShowString(10, 32, (char *)str_all_tests_passed, OLED_8X16);
    OLED_DrawLine(0, 63, 60, 63);
    OLED_Update();

    LOG_I(TAG, "==== OLED functional test complete ====");
    uint32_t tick = (uint32_t)osal_get_tick();
    LOG_I(TAG, "Test finished at tick: %lu (~%lu seconds)", tick, tick / 1000);
}

/* ================== Test task ================== */

static void I2C_SW_test(void *pvParameters)
{
    (void)pvParameters;
    LOG_I(TAG, "I2C test task started");

    OLED_Init();
    OLED_ComprehensiveTest();

    uint32_t counter = 0;
    while (1)
    {
        OLED_ClearArea(90, 56, 38, 8);
        OLED_ShowNum(90, 56, counter++, 4, OLED_6X8);
        OLED_UpdateArea(90, 56, 38, 8);
        LOG_D(TAG, "Heartbeat: %lu", counter);
        osal_task_delay(5000);
    }
}

/* ================== Public functions ================== */

void I2CTestTask_Init(void)
{
//     LOG_I(TAG, "Initializing I2C subsystem...");

//     i2c1 = (I2C_Driver_t *)I2C1_SW_GetDriver();
// #if I2C1_SW_ENABLE
//     i2c1->init();
//     LOG_I(TAG, "SW I2C1 initialized (PB6=SCL, PB7=SDA)");
// #else
//     MX_I2C1_Init();
//     LOG_I(TAG, "HW I2C1 initialized");
// #endif

    test_handle = osal_task_create("I2C_OLED_test", I2C_SW_test,
                                    256, NULL, 3);
    if (test_handle == NULL)
        LOG_E(TAG, "I2C test task creation failed!");
    else
        LOG_I(TAG, "I2C test task created (stack=256 words, priority=3)");
}
