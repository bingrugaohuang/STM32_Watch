#ifndef SERVICE_LOG_H
#define SERVICE_LOG_H

#include <stdarg.h>
#include <stdint.h>

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_OFF
} LogLevel;

typedef void (*LogBackend)(LogLevel level, const char *tag, const char *fmt, va_list args);

/*
 * 函数功能：初始化日志模块，设置默认日志等级与后端
 * 入口参数：无
 * 返回值  ：无
 */
void Log_Init(void);

/*
 * 函数功能：设置全局日志输出等级，低于该等级的日志将被过滤
 * 入口参数：level 目标日志等级
 * 返回值  ：无
 */
void Log_SetLevel(LogLevel level);

/*
 * 函数功能：获取当前全局日志输出等级
 * 入口参数：无
 * 返回值  ：当前日志等级
 */
LogLevel Log_GetLevel(void);

/*
 * 函数功能：注册日志后端回调，用于自定义输出（串口、文件等）
 * 入口参数：backend 后端回调指针
 * 返回值  ：无
 */
void Log_RegisterBackend(LogBackend backend);

/*
 * 函数功能：输出一条日志，支持可变参数
 * 入口参数：level 日志等级；tag 模块标签；fmt 格式化字符串；... 可变参数
 * 返回值  ：无
 */
void Log_Print(LogLevel level, const char *tag, const char *fmt, ...);

#define LOG_E(tag, fmt, ...) Log_Print(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) Log_Print(LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) Log_Print(LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) Log_Print(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_T(tag, fmt, ...) Log_Print(LOG_LEVEL_TRACE, tag, fmt, ##__VA_ARGS__)

#endif /* SERVICE_LOG_H */
