#ifndef SERVICE_LOG_H
#define SERVICE_LOG_H

#include <stdarg.h>
#include <stdint.h>

/* ---------- 日志等级 ---------- */
typedef enum {
    LOG_LEVEL_ERROR = 0,   /**< 错误信息，系统异常 */
    LOG_LEVEL_WARN,        /**< 警告信息，可恢复 */
    LOG_LEVEL_INFO,        /**< 一般信息，正常运行记录 */
    LOG_LEVEL_DEBUG,       /**< 调试信息，开发阶段使用 */
    LOG_LEVEL_TRACE,       /**< 详细追踪，函数级调用 */
    LOG_LEVEL_OFF          /**< 关闭日志输出 */
} LogLevel;

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

/* ---------- 便捷宏（直接调用 log_print）---------- */
#define LOG_E(tag, fmt, ...) log_print(LOG_LEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOG_W(tag, fmt, ...) log_print(LOG_LEVEL_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOG_I(tag, fmt, ...) log_print(LOG_LEVEL_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOG_D(tag, fmt, ...) log_print(LOG_LEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#define LOG_T(tag, fmt, ...) log_print(LOG_LEVEL_TRACE, tag, fmt, ##__VA_ARGS__)

#endif /* SERVICE_LOG_H */