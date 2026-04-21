#include "Log.h"
#include "osal.h"

#include <stdio.h>

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

static LogLevel sLogLevel = LOG_DEFAULT_LEVEL;
static LogBackend sBackend = NULL;

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

void Log_SetLevel(LogLevel level)
{
    sLogLevel = level;
}

LogLevel Log_GetLevel(void)
{
    return sLogLevel;
}

void Log_RegisterBackend(LogBackend backend)
{
    sBackend = backend;
}

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
