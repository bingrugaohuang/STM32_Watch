/* ================== Service/serial_service.c ================== */
#include "serial_service.h"
#include "osal.h"
#include "bsp_usart.h" /* 只认识 BSP 的干净接口 */

/* --- RTOS 句柄等定义 --- */
static osal_task_handle_t s_tx_task;

/* * 1. 写一个函数来处理发送完成事件（注意：这个函数实际是在中断上下文里运行的！） 
 */
static void serial_dma_tx_done_isr(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* 通知 RTOS 任务，DMA 发完了！ */
    osal_task_notify_from_isr(s_tx_task, 
                              0, 
                              0, /* 你之前的清理逻辑可以放这里 */
                              OSAL_NOTIFY_SET_VALUE_OVERWRITE, 
                              &xHigherPriorityTaskWoken);
                              
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Serial_Service_Init(void) {
    /* ... 创建内存池、队列、任务等 ... */

    /* 2. 核心操作：去 BSP 注册回调！
       把我们刚才写的 serial_dma_tx_done_isr 的地址告诉 BSP。
       以后只要 DMA 硬件发完，BSP 就会自动调这个函数。 */
    BSP_USART_RegisterTxCallback(serial_dma_tx_done_isr);
}

static void serial_tx_task(void *pvParameters) {
    while (1) {
        /* 取队列... */
        /* 调用 BSP_USART_SendDMA() 开始发送... */
        /* 挂起任务，等待 DMA 发送完成标志... */
    }
}