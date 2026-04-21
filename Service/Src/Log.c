#include "Log.h"
#include "osal.h"

#include <stdio.h>

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

static LogLevel sLogLevel = LOG_DEFAULT_LEVEL;
static LogBackend sBackend = NULL;

/*
 * 函数功能：将日志等级枚举转换为字符串标记
 * 入口参数：level 日志等级
 * 返回值  ：等级对应的字符串
 */
static const char *Log_LevelToString(LogLevel level)
{
    switch(level)
    {
        case LOG_LEVEL_ERROR: return "E";
        case LOG_LEVEL_WARN:  return "W";
        case LOG_LEVEL_INFO:  return "I";
        case LOG_LEVEL_DEBUG: return "D";
        case LOG_LEVEL_TRACE: return "T";
        default:              return "?";
    }
}

/*
 * 函数功能：默认日志后端，使用printf按固定格式输出
 * 入口参数：level 日志等级；tag 标签；fmt 格式化字符串；args 可变参数列表
 * 返回值  ：无
 */
static void Log_DefaultBackend(LogLevel level, const char *tag, const char *fmt, va_list args)
{
    printf("[%10lu][%s][%s] ",
           (unsigned long)OSAL_GetTickCount(),
           Log_LevelToString(level),
           (tag != NULL) ? tag : "LOG");
    vprintf(fmt, args);
    printf("\r\n");
}

void Log_Init(void)
{
    sLogLevel = LOG_DEFAULT_LEVEL;
    sBackend = Log_DefaultBackend;
}

/*
 * 函数功能：设置全局日志输出等级
 * 入口参数：level 目标日志等级
 * 返回值  ：无
 */
void Log_SetLevel(LogLevel level)
{
    sLogLevel = level;
}

/*
 * 函数功能：获取当前全局日志输出等级
 * 入口参数：无
 * 返回值  ：当前日志等级
 */
LogLevel Log_GetLevel(void)
{
    return sLogLevel;
}

/*
 * 函数功能：注册日志输出后端
 * 入口参数：backend 后端回调指针
 * 返回值  ：无
 */
void Log_RegisterBackend(LogBackend backend)
{
    sBackend = backend;
}

/*
 * 函数功能：输出一条日志，符合当前等级才会输出
 * 入口参数：level 日志等级；tag 标签；fmt 格式化字符串；... 可变参数
 * 返回值  ：无
 */
void Log_Print(LogLevel level, const char *tag, const char *fmt, ...)
{
    if(level > sLogLevel || level >= LOG_LEVEL_OFF)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);

    if(sBackend != NULL)
    {
        sBackend(level, tag, fmt, args);
    }

    va_end(args);
}
