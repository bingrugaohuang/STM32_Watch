#ifndef SERVICE_LOG_H
#define SERVICE_LOG_H

#include <stdarg.h>
#include <stdint.h>
#include "log_tags.h"

/* ---------- 日志等级（预处理器宏 — 供 #if 条件编译使用） ---------- */
#define LOG_LVL_OFF     0
#define LOG_LVL_ERROR   1
#define LOG_LVL_WARN    2
#define LOG_LVL_INFO    3
#define LOG_LVL_DEBUG   4
#define LOG_LVL_TRACE   5

/* ---------- 日志等级（枚举 — 供 C 代码运行时使用） ---------- */
typedef enum {
    LOG_LEVEL_OFF   = LOG_LVL_OFF,
    LOG_LEVEL_ERROR = LOG_LVL_ERROR,
    LOG_LEVEL_WARN  = LOG_LVL_WARN,
    LOG_LEVEL_INFO  = LOG_LVL_INFO,
    LOG_LEVEL_DEBUG = LOG_LVL_DEBUG,
    LOG_LEVEL_TRACE = LOG_LVL_TRACE,
} LogLevel;

/* 编译期最高日志等级（高于此等级的宏在编译时直接抹掉，零 ROM 开销） */
#ifndef LOG_MAX_LEVEL
#define LOG_MAX_LEVEL LOG_LVL_INFO   /* 默认：全部开启 */
#endif

/* 运行期初始日志等级（启动后可通过 log_set_level() 动态调整） */
#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

/* ---------- 后端回调类型 ---------- */
typedef void (*LogBackend)(LogLevel level,
                           const char *tag,
                           const char *fmt,
                           va_list args);

/**< 后端输出行最大长度（含前缀 + 正文 + \r\n） */
#define LOG_LINE_MAX_LEN   192

/**
  * 函    数：初始化日志模块
  * 参    数：无
  * 返 回 值：无
  * 说    明：设置默认日志等级为 LOG_LEVEL_INFO，注册默认串口后端。
  */
void log_init(void);

/**
  * 函    数：设置全局日志输出等级
  * 参    数：level - 目标日志等级，低于该等级的日志将被过滤
  * 返 回 值：无
  */
void log_set_level(LogLevel level);

/**
  * 函    数：获取当前全局日志输出等级
  * 参    数：无
  * 返 回 值：当前日志等级
  */
LogLevel log_get_level(void);

/**
  * 函    数：注册自定义日志后端
  * 参    数：backend - 后端回调函数指针，NULL 表示恢复默认串口后端
  * 返 回 值：无
  */
void log_register_backend(LogBackend backend);

/**
  * 函    数：同步日志输出
  * 参    数：level - 日志等级
  *           tag   - 模块标签字符串
  *           fmt   - 格式化字符串
  *           ...   - 可变参数
  * 返 回 值：无
  * 说    明：在当前任务上下文中格式化并输出日志，阻塞时间极短（仅内存拷贝和串口队列投递）。
  */
void log_print(LogLevel level, const char *tag, const char *fmt, ...);

/* ---------- 便捷宏（直接调用 log_print，高于 LOG_MAX_LEVEL 的宏编译期抹除）---------- */
#if LOG_MAX_LEVEL >= LOG_LVL_ERROR
#define LOG_E(tag, fmt, ...) log_print(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#define LOG_E(tag, fmt, ...) ((void)0)
#endif

#if LOG_MAX_LEVEL >= LOG_LVL_WARN
#define LOG_W(tag, fmt, ...) log_print(LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#else
#define LOG_W(tag, fmt, ...) ((void)0)
#endif

#if LOG_MAX_LEVEL >= LOG_LVL_INFO
#define LOG_I(tag, fmt, ...) log_print(LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#else
#define LOG_I(tag, fmt, ...) ((void)0)
#endif

#if LOG_MAX_LEVEL >= LOG_LVL_DEBUG
#define LOG_D(tag, fmt, ...) log_print(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#else
#define LOG_D(tag, fmt, ...) ((void)0)
#endif

#if LOG_MAX_LEVEL >= LOG_LVL_TRACE
#define LOG_T(tag, fmt, ...) log_print(LOG_LEVEL_TRACE, tag, fmt, ##__VA_ARGS__)
#else
#define LOG_T(tag, fmt, ...) ((void)0)
#endif

#endif /* SERVICE_LOG_H */