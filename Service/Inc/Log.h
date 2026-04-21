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

void Log_Init(void);
void Log_SetLevel(LogLevel level);
LogLevel Log_GetLevel(void);
void Log_RegisterBackend(LogBackend backend);
void Log_Print(LogLevel level, const char *tag, const char *fmt, ...);

#define LOG_E(tag, fmt, ...) Log_Print(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) Log_Print(LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) Log_Print(LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) Log_Print(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_T(tag, fmt, ...) Log_Print(LOG_LEVEL_TRACE, tag, fmt, ##__VA_ARGS__)

#endif /* SERVICE_LOG_H */
