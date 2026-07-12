#ifndef HARDFAULT_DEBUG_H
#define HARDFAULT_DEBUG_H

#include <stdint.h>

/* ================================================================
 *  条件编译配置（在此修改以选择 HardFault 输出行为）
 * ================================================================
 *    HARDFAULT_CFG_SERIAL_ENABLE  - HardFault 时实时串口输出
 *    HARDFAULT_CFG_FLASH_SAVE     - HardFault 时保存到 FLASH
 *
 *  组合：
 *    SERIAL=1, FLASH=0 → 仅串口（原有行为）
 *    SERIAL=0, FLASH=1 → 仅 FLASH（崩溃时静默保存到 FLASH，复位后输出）
 *    SERIAL=1, FLASH=1 → 两者同时启用
 */
#define HARDFAULT_CFG_SERIAL_ENABLE      1
#define HARDFAULT_CFG_FLASH_SAVE         1
#define HARDFAULT_CFG_DIAGNOSTIC         1   /* 启动时若无崩溃日志，输出 FLASH 诊断信息（调试用，发布时置 0） */
#define HARDFAULT_CFG_FLASH_MODE         1   /* 0=简化版(无错误检查+重试), 1=带错误检查版 */
#define HARDFAULT_CFG_FLASH_DEBUG_TRACE  1   /* 写 FLASH 时输出进度字符（调试用，发布时置 0） */

/**
  * HardFault 调试模块初始化函数 
  */
void hardfault_debug_init(void);

/**
  * HardFault 崩溃诊断转储。
  * 在 HardFault_Handler 的 naked 入口中获取异常栈帧指针后调用。
  * 根据 HARDFAULT_CFG_SERIAL_ENABLE / HARDFAULT_CFG_FLASH_SAVE 宏决定输出方式。
  *
  * 参    数：stack_frame - 指向 CPU 自动压栈的 8 字异常帧
  *           (R0, R1, R2, R3, R12, LR, PC, xPSR)
  */
void hardfault_dump(uint32_t *stack_frame);

/**
  * 启动时检查 FLASH 中是否有上次 HardFault 留下的崩溃日志。
  * 若有，通过串口轮询方式输出，然后擦除该 FLASH 页。
  * 应在系统初始化早期（串口硬件就绪后）调用。
  */
void check_crash_log_on_startup(void);

#endif
