#include "common_macro.h"

#if STACK_MONITOR_ENABLE

#include "osal.h"
#include "log.h"

// 栈高水位监控函数，用于打印指定任务的栈高水位标记
void stackmonitor(osal_task_handle_t xTask, const char *taskName) {
    osal_UBaseType_t highWaterMark = osal_getstackhighwatermark(xTask);
    // ul在大多数平台都是4字节，但u不一定
    LOG_I(TAG_MONITOR, "Task '%s' stack high water mark: %lu words", taskName, (unsigned long)highWaterMark);
}
#else
// 空实现，当 STACK_MONITOR_ENABLE 为 0 时不执行任何操作
void stackmonitor(osal_task_handle_t xTask, const char *taskName) {
    // 空实现
}

#endif