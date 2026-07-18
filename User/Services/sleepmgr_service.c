#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "mpu6050_driver.h"
#include "common_macro.h"
#include "log.h"
#include "serial.h"

#if MPU_MOT_TEST_ENABLE
#include "gpio.h"
#endif

/* ========== 移植层宏（来自 port.c，覆盖 tickless 函数必需）========== */
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PEND_SYSTICK_SET_BIT         ( 1UL << 26UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT       ( 1UL << 25UL )

#define portMAX_24_BIT_NUMBER                 ( 0xffffffUL )
#define portMISSED_COUNTS_FACTOR              ( 94UL )

#ifndef configSYSTICK_CLOCK_HZ
    #define configSYSTICK_CLOCK_HZ             ( configCPU_CLOCK_HZ )
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( portNVIC_SYSTICK_CLK_BIT )
#else
    #define portNVIC_SYSTICK_CLK_BIT_CONFIG    ( 0 )
#endif

/* Tickless 计算变量（与 port.c 一致，但本地定义） */
static uint32_t ulTimerCountsForOneTick = 0;
static uint32_t xMaximumPossibleSuppressedTicks = 0;
static uint32_t ulStoppedTimerCompensation = 0;

/* 空闲任务钩子函数 — configUSE_IDLE_HOOK=1 时，每个空闲循环周期调用 */
void vApplicationIdleHook( void )
{
    //mot_test_gpio_togle_pin(); // 测试用，PB0 翻转，标记空闲循环
}

/* Tickless函数的覆盖实现 */
void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime ){

    /* 懒初始化：首次调用时计算（port.c 在 xPortStartScheduler 中初始化） */
    if( ulTimerCountsForOneTick == 0 )
    {
        ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
        xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
    }

#if MPU_MOT_TEST_ENABLE
    mot_test_gpio_set_high(); // 测试用，PB0 拉高，标记进入 tickless
    
#endif
    
    uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements, ulSysTickDecrementsLeft;
    TickType_t xModifiableIdleTime;

    /* 确保 SysTick 的重载值不会导致计数器溢出。 */
    if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
    {
        xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
    }

    /* 进入临界区，但不要使用 taskENTER_CRITICAL() 宏，
        * 因为该宏会屏蔽掉那些本应唤醒（退出睡眠模式）的中断。 */
    __disable_irq();
    __dsb( portSY_FULL_READ_WRITE );
    __isb( portSY_FULL_READ_WRITE );

    /* 如果当前已有上下文切换待处理，或者有任务在等待调度器恢复运行，
        * 就放弃进入低功耗。 */
    if( eTaskConfirmSleepModeStatus() == eAbortSleep )
    {
        /* 重新使能中断——参考上面对 __disable_irq() 调用的说明。 */
        __enable_irq();
    }
    else
    {
        /* 先暂时停止 SysTick。停止期间的时间会尽量被补偿，
            * 但使用 tickless 模式时，内核维护的时间相对于实际日历时间
            * 仍然不可避免会有一点点漂移。 */
        portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

        /* 使用 SysTick 当前值寄存器计算距离下一次 tick 中断还剩多少次递减。
            * 如果当前值寄存器为 0，并不代表真的没有剩余，而是还剩
            * ulTimerCountsForOneTick 次递减，因为 SysTick 会在从 1 递减到 0 时
            * 请求中断。 */
        ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;

        if( ulSysTickDecrementsLeft == 0 )
        {
            ulSysTickDecrementsLeft = ulTimerCountsForOneTick;
        }

        /* 计算等待 xExpectedIdleTime 个 tick 周期所需的重装载值。
            * 这里减 1，是因为这段代码通常是在第一个 tick 周期进行到一半时执行的。
            * 如果 SysTick 中断请求已经挂起，则清除该请求、屏蔽第一个 tick，
            * 并修正重装载值，以反映第二个 tick 周期已经开始。
            * 预计的空闲时间至少为两个 tick。 */
        ulReloadValue = ulSysTickDecrementsLeft + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );

        if( ( portNVIC_INT_CTRL_REG & portNVIC_PEND_SYSTICK_SET_BIT ) != 0 )
        {
            portNVIC_INT_CTRL_REG = portNVIC_PEND_SYSTICK_CLEAR_BIT;
            ulReloadValue -= ulTimerCountsForOneTick;
        }

        if( ulReloadValue > ulStoppedTimerCompensation )
        {
            ulReloadValue -= ulStoppedTimerCompensation;
        }

        /* 设置新的重装载值。 */
        portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

        /* 清除 SysTick 计数标志，并把计数值复位为 0。 */
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

        /* 重新启动 SysTick。 */
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;


        /* 进入睡眠，直到有事件发生。configPRE_SLEEP_PROCESSING() 可以把它的参数
            * 设为 0，表示其实现中已经包含了自己的 wait for interrupt 或
            * wait for event 指令，因此这里不应再次执行 wfi。
            * 不过原始的预计空闲时间变量不能被修改，所以这里先拷贝一份。 */
        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

        if( xModifiableIdleTime > 0 )
        {
            __dsb( portSY_FULL_READ_WRITE );
            __wfi();
            __isb( portSY_FULL_READ_WRITE );
        }

        configPOST_SLEEP_PROCESSING( xExpectedIdleTime );

        /* 重新使能中断，让把 MCU 从睡眠中唤醒的那个中断能够立即执行。
            * 参考上面对 __disable_irq() 调用的说明。 */
        __enable_irq();
        __dsb( portSY_FULL_READ_WRITE );
        __isb( portSY_FULL_READ_WRITE );

        /* 再次关闭中断，因为接下来时钟即将停止；如果在时钟停止期间还执行中断，
            * 会进一步增加 RTOS 维护时间与实际日历时间之间的偏差。 */
        __disable_irq();
        __dsb( portSY_FULL_READ_WRITE );
        __isb( portSY_FULL_READ_WRITE );

        /* 关闭 SysTick 时钟时不要去读取 portNVIC_SYSTICK_CTRL_REG 寄存器，
            * 这样可以确保如果 portNVIC_SYSTICK_COUNT_FLAG_BIT 已经置位，
            * 它不会被清除。再次说明，SysTick 停止期间的时间会尽量补偿，
            * 但使用 tickless 模式时，内核维护的时间相对实际日历时间仍会
            * 有一点点漂移。*/
        portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT );

        /* 判断 SysTick 是否已经计数到 0。 */
        if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
        {
            uint32_t ulCalculatedLoadValue;

            /* 是 tick 中断结束了睡眠（或者它现在已挂起），并且新的 tick 周期
                * 已经开始。用新 tick 周期剩余的部分重新设置
                * portNVIC_SYSTICK_LOAD_REG。 */
            ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

            /* 不允许出现过小的值，也不允许出现因为睡眠后钩子执行过久，
                * 或者 SysTick 当前值寄存器为 0 而导致的下溢值。 */
            if( ( ulCalculatedLoadValue <= ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
            {
                ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
            }

            portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

            /* 由于挂起的 tick 会在函数退出后立刻处理，所以 tick 维护的计数
                * 只需要比实际等待时间少一步。 */
            ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
        }
        else
        {
            /* 结束睡眠的并不是 tick 中断。 */

            /* 使用 SysTick 当前值寄存器计算，距离预计空闲时间结束还剩多少次
                * SysTick 递减。 */
            ulSysTickDecrementsLeft = portNVIC_SYSTICK_CURRENT_VALUE_REG;
            #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG != portNVIC_SYSTICK_CLK_BIT )
            {
                /* 如果 SysTick 没有使用内核时钟，这里当前值寄存器仍然可能为 0。
                    * 这种情况下，SysTick 并没有从重装载寄存器装载，
                    * 预计空闲时间里剩余的不是 0，而是 ulReloadValue 次递减。 */
                if( ulSysTickDecrementsLeft == 0 )
                {
                    ulSysTickDecrementsLeft = ulReloadValue;
                }
            }
            #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG */

            /* 计算本次睡眠持续了多久，结果按完整 tick 周期取整；
                * 这里不直接使用 ulReload 值，因为它已经考虑过半个 tick 的情况。 */
            ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - ulSysTickDecrementsLeft;

            /* 处理器等待期间经过了多少个完整的 tick 周期？ */
            ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

            /* 将重装载值设置为单个 tick 周期剩余的那一部分。 */
            portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
        }

        /* 重新启动 SysTick，使其再次从 portNVIC_SYSTICK_LOAD_REG 开始运行，
            * 然后把 portNVIC_SYSTICK_LOAD_REG 恢复为标准值。如果 SysTick 目前
            * 不是使用内核时钟，则临时切换为内核时钟。这样可以强制 SysTick 立即
            * 从 portNVIC_SYSTICK_LOAD_REG 装载，而不是等到另一时钟的下一个周期。
            * 随后 portNVIC_SYSTICK_LOAD_REG 就可以立刻写回标准值。 */
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
        portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
        #if ( portNVIC_SYSTICK_CLK_BIT_CONFIG == portNVIC_SYSTICK_CLK_BIT )
        {
            portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
        }
        #else
        {
            /* 如上所述，临时使用内核时钟的目的已经达到，现在恢复使用另一时钟。 */
            portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT;

            if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
            {
                /* 这段不完整的 tick 周期已经结束，确保 SysTick 只统计一次。 */
                portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
            }

            portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;
            portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT_CONFIG | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
        }
        #endif /* portNVIC_SYSTICK_CLK_BIT_CONFIG */

        /* 推进 tick 计数，以补偿已经经过的所有 tick 周期。 */
        vTaskStepTick( ulCompleteTickPeriods );

        /* 退出时保持中断使能。 */
        __enable_irq();


    
#if MPU_MOT_TEST_ENABLE
    mot_test_gpio_set_low(); // 测试用，PB0 拉低，标记进入 tickless
    // uint8_t buf[64];
    // sprintf(buf, "%lu, %lu, %lu\r\n", 
    //     xExpectedIdleTime, ulReloadValue, ulCompleteTickPeriods);
    // serial_send_blocking((uint8_t*)buf, strlen(buf));
#endif
    }

}