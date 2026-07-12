/**
 * ================================================================
 *  log_tags.h — 日志模块标签统一管理
 *
 *  所有模块日志标签集中定义在此文件，使用 TAG_xxx 宏
 *  代替裸字符串，实现编译期拼写检查 + 单一维护点。
 *
 *  使用方式：
 *    LOG_I(TAG_MAIN, "System boot...");
 *    LOG_E(TAG_I2C, "I2C transfer error");
 * ================================================================
 */

#ifndef __LOG_TAGS_H
#define __LOG_TAGS_H

/* ========== APP 层 — 应用菜单页面 ========== */
#define TAG_MAIN    "MAIN"      /* 系统入口 / 任务创建              */
#define TAG_MENU    "MENU"      /* 菜单通用（过渡期，逐步细化）      */
#define TAG_HOME    "HOME"      /* 主页时钟                        */
#define TAG_APPLIST "APPS"      /* 应用列表 (cover-flow)           */
#define TAG_STPW    "STPW"      /* 秒表                            */
#define TAG_FLSH    "FLSH"      /* 手电筒                          */

/* ========== Services 层 — 应用服务 ========== */
#define TAG_BTN     "BTN"       /* 按钮服务                        */
#define TAG_DISP    "DISP"      /* 显示服务                        */
#define TAG_RTC     "RTC"       /* RTC 时钟服务                    */
#define TAG_LOG     "LOG"       /* 日志模块自身                    */
#define TAG_MONITOR "MONITOR"   /* 堆栈监控服务                        */

/* ========== Drivers 层 — 设备驱动 ========== */
#define TAG_OLED    "OLED"      /* OLED 显示驱动                   */
#define TAG_SER     "SER"       /* 串口驱动                        */
#define TAG_MPU     "MPU6050"   /* MPU6050 传感器驱动              */

/* ========== BSP 层 — 板级支持包 ========== */
#define TAG_I2C     "I2C"       /* I2C 总线 (hw/sw)                */
#define TAG_USART   "USART"     /* USART 外设                      */

/* ========== Middlewares 层 — 中间件 ========== */
#define TAG_MENUE   "MENUE"     /* 菜单引擎 (menu_engine)          */
#define TAG_HDBG    "HDBG"      /* HardFault 调试                  */

/* ========== 测试 ========== */
#define TAG_TEST    "TEST"      /* I2C / OLED 综合测试             */

#endif /* __LOG_TAGS_H */
