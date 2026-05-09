#include "log.h"
#include "osal.h"
#include "serial.h"        /* 默认后端使用串口异步发送 */
#include "main.h"          /* 引入 Error_Handler 声明（若需要） */
#include <stdio.h>
#include <string.h>

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL LOG_LEVEL_INFO
#endif

/* ---------- 静态全局变量 ---------- */
static volatile LogLevel s_log_level = LOG_DEFAULT_LEVEL;
static LogBackend        s_backend   = NULL;

/* ---------- 私有函数声明 ---------- */
static const char *level_to_string(LogLevel level);
static void default_backend(LogLevel level, const char *tag, const char *fmt, va_list args);

/* ========================== 公共函数 ========================== */

/**
  * 函    数：log_init
  * 功    能：初始化日志模块，设置默认等级和后端
  */
void log_init(void)
{
    s_log_level = LOG_DEFAULT_LEVEL;
    s_backend = default_backend;   /* 默认使用串口后端 */
}

/**
  * 函    数：log_set_level
  * 功    能：设置日志输出等级阈值
  */
void log_set_level(LogLevel level)
{
    s_log_level = level;
}

/**
  * 函    数：log_get_level
  * 功    能：获取当前日志输出等级
  */
LogLevel log_get_level(void)
{
    return s_log_level;
}

/**
  * 函    数：log_register_backend
  * 功    能：注册自定义日志输出后端（若传入 NULL，则恢复默认串口后端）
  */
void log_register_backend(LogBackend backend)
{
    if (backend == NULL) {
        s_backend = default_backend;
    } else {
        s_backend = backend;
    }
}

/**
  * 函    数：log_print
  * 功    能：同步格式化并输出一条日志（阻塞极短，仅完成内存拷贝和串口队列投递）
  *          注；不要在中断上下文调用此函数，除非你确保后端支持并且不会引起问题。
  */
void log_print(LogLevel level, const char *tag, const char *fmt, ...)
{
    if (s_log_level == LOG_LEVEL_OFF || level > s_log_level) {
        return;
    }

    if (s_backend != NULL) {
        va_list args;
        va_start(args, fmt);
        s_backend(level, tag, fmt, args);
        va_end(args);
    }
}

/* ========================== 私有函数 ========================== */

/**
  * 函    数：level_to_string
  * 功    能：将日志等级转换为单字符标识
  */
static const char *level_to_string(LogLevel level)
{
    switch (level) {
        case LOG_LEVEL_ERROR: return "E";
        case LOG_LEVEL_WARN:  return "W";
        case LOG_LEVEL_INFO:  return "I";
        case LOG_LEVEL_DEBUG: return "D";
        case LOG_LEVEL_TRACE: return "T";
        default:              return "U";
    }
}

/**
  * 函    数：default_backend
  * 功    能：拼装 [时间戳][等级][标签] 前缀 + 用户文本 + "\r\n"，通过串口异步发送
  */
static void default_backend(LogLevel level, const char *tag, const char *fmt, va_list args)
{
    char buf[LOG_LINE_MAX_LEN];
    int len;

    /* 1. 拼装前缀 */
    len = snprintf(buf, sizeof(buf), "[%lu][%s][%s] ",
                   (unsigned long)osal_get_tick(),
                   level_to_string(level),
                   tag ? tag : "");
    if (len < 0 || len >= (int)sizeof(buf)) {
        len = sizeof(buf) - 3;   /* 确保有空间添加 \r\n */
    }

    /* 2. 拼装用户正文 */
    int content_len = vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    if (content_len > 0) {
        len += content_len;
    }
    if (len >= (int)sizeof(buf) - 3) {
        len = sizeof(buf) - 3;
    }

    /* 3. 追加 \r\n */
    buf[len] = '\r';
    buf[len + 1] = '\n';
    buf[len + 2] = '\0';

    /* 4. 投递到串口发送队列（非阻塞 DMA，调用者马上返回） */
    serial_send_async((uint8_t *)buf, (size_t)(len + 2));
}