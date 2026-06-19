#include "serial.h"
#include "usart.h"
#include "osal.h"
#include "main.h"           /* Error_Handler */
#include <string.h>
#include "common_macro.h"

/* ---------- 发送任务栈与优先级 ---------- */
#define SERIAL_TX_TASK_STACK_SIZE   256
#define SERIAL_TX_TASK_PRIORITY     5

/* ---------- 队列长度 ---------- */
#define SERIAL_TX_QUEUE_LENGTH      16    /*考虑到队列扩展后可能用于发送控制指令，因此比内存池大小大 */

/* ---------- 内存池配置 ---------- */
#define SERIAL_BUF_SIZE             256   /* 每个数据块的大小（根据最大帧长调整） */
#define SERIAL_NUM_BUFS             8     /* 池中块的总数，决定最大并发发送数 */

/* ---------- 数据包结构体 ---------- */
typedef struct {
    uint8_t *pData;     /* 动态分配的缓冲区指针 */
    size_t   size;      /* 有效数据长度 */
} SerialTxMsg_t;

/* ---------- 内存池管理 ---------- */
typedef struct FreeBlock {
    struct FreeBlock *next;   /* 下一个空闲块指针，存储在块头部 */
} FreeBlock_t;

static uint8_t              s_pool_buf[SERIAL_NUM_BUFS][SERIAL_BUF_SIZE]
                            __attribute__((aligned(sizeof(FreeBlock_t)))); /* 静态池空间 */
static volatile FreeBlock_t *s_free_list = NULL;  /* 空闲链表栈顶 */

/* ---------- 静态变量 ---------- */
static osal_queue_handle_t     s_tx_queue;
static osal_task_handle_t      s_tx_task;
static osal_semaphore_handle_t s_buf_sem;          /* 计数信号量，管理池中可用块数 */
static volatile uint8_t        s_dma_busy = 0;
static uint8_t *volatile       s_current_dma_buffer;

/* ---------- 私有函数声明 ---------- */
static void serial_tx_task(void *pvParameters);
static void serial_start_dma(const uint8_t *pData, size_t size);
static void serial_dma_cleanup(uint8_t *pBuffer);

/* 内存池操作（任务上下文与中断上下文分离） */
static void serial_pool_init(void);
static uint8_t *serial_buf_alloc_from_task(void);
static uint8_t *serial_buf_alloc_from_isr(BaseType_t *pxHigherPriorityTaskWoken);
static void serial_buf_free_from_task(uint8_t *pBuf);
static void serial_buf_free_from_isr(uint8_t *pBuf, BaseType_t *pxHigherPriorityTaskWoken);

/* ========================== 半主机模式禁用与标准库重定向 ========================== */
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#if (__ARMCC_VERSION >= 6000000)
    __asm(".global __use_no_semihosting\n\t");
#else
    #pragma import(__use_no_semihosting)
#endif

#include <stdio.h>
struct __FILE { int handle; };
FILE __stdout;
FILE __stdin;
FILE __stderr;

void _sys_exit(int x) { x = x; while(1); }
void _ttywrch(int ch) { ch = ch; }
char *_sys_command_string(char *cmd, int len) { return NULL; }
int fputc(int ch, FILE *f) {
    while (!(USART1->SR & USART_SR_TXE)); // 等待发送寄存器为空
    USART1->DR = ch;                // 把字符丢给串口 1
    return ch;
}
int fgetc(FILE *f) { return EOF; }
int ferror(FILE *f) { return EOF; }
#endif

/* ========================== 公共函数 ========================== */

/**
  * 函    数：串口初始化
  */
void serial_init(void)
{
    /* 初始化内存池（必须在创建队列/任务之前） */
    serial_pool_init();

    s_tx_queue = osal_queue_create(SERIAL_TX_QUEUE_LENGTH, sizeof(SerialTxMsg_t));
    if (s_tx_queue == NULL) {
        Error_Handler();
    }

    s_tx_task = osal_task_create("serial_tx", serial_tx_task,
                                 SERIAL_TX_TASK_STACK_SIZE, NULL,
                                 SERIAL_TX_TASK_PRIORITY);
    if (s_tx_task == NULL) {
        Error_Handler();
    }
}

/**
  * 函    数：任务上下文异步发送
  */
int serial_send_async(const uint8_t *pData, size_t size)
{
    if (pData == NULL || size == 0 || size > SERIAL_BUF_SIZE) {
        return COMMON_ERR_PARAM;
    }

    uint8_t *buf = serial_buf_alloc_from_task();
    if (buf == NULL) {
        return COMMON_ERR_MEM;
    }
    memcpy(buf, pData, size);

    SerialTxMsg_t msg;
    msg.pData = buf;
    msg.size  = size;

    if (osal_queue_send(s_tx_queue, &msg, OSAL_NO_WAIT) != OSAL_OK) {
        serial_buf_free_from_task(buf);
        return COMMON_ERR_QUEUE_FULL;
    }
    return COMMON_ERR_OK;
}

/**
  * 函    数：中断上下文异步发送
  */
int serial_send_async_from_isr(const uint8_t *pData, size_t size,
                               BaseType_t *pxHigherPriorityTaskWoken)
{
    if (pData == NULL || size == 0 || size > SERIAL_BUF_SIZE) {
        return COMMON_ERR_PARAM;
    }

    uint8_t *buf = serial_buf_alloc_from_isr(pxHigherPriorityTaskWoken);
    if (buf == NULL) {
        return COMMON_ERR_MEM;
    }
    memcpy(buf, pData, size);

    SerialTxMsg_t msg;
    msg.pData = buf;
    msg.size  = size;

    if (osal_queue_send_from_isr(s_tx_queue, &msg, pxHigherPriorityTaskWoken) != OSAL_OK) {
        serial_buf_free_from_isr(buf, pxHigherPriorityTaskWoken);
        return COMMON_ERR_QUEUE_FULL;
    }
    return COMMON_ERR_OK;
}

/**
  * 函    数：紧急轮询发送（与之前相同）
  */
void serial_send_blocking(const uint8_t *pData, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        while (!(USART1->SR & USART_SR_TXE)) {}
        USART1->DR = pData[i];
    }
    while (!(USART1->SR & USART_SR_TC)) {}
}

/* ========================== 私有函数 ========================== */

/**
  * 函    数：串口发送任务
  */
static void serial_tx_task(void *pvParameters)
{
    (void)pvParameters;

    while (1) {
        SerialTxMsg_t msg;
        if (osal_queue_receive(s_tx_queue, &msg, OSAL_WAIT_FOREVER) != OSAL_OK) {
            continue;
        }

        /* 等待上一次 DMA 完成 */
        while (s_dma_busy) {
            uint32_t notified_value;
            if (osal_task_notify_wait(0, 0xFFFFFFFF,
                                      &notified_value, OSAL_WAIT_FOREVER) == OSAL_OK) {
                serial_dma_cleanup((uint8_t *)notified_value);
                s_dma_busy = 0;   // 任务负责清零
            }
        }

        s_dma_busy = 1;
        serial_start_dma(msg.pData, msg.size);
    }
}

/*
 * 函    数：启动 DMA 发送
 */
static void serial_start_dma(const uint8_t *pData, size_t size)
{
    s_current_dma_buffer = (uint8_t *)pData;
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)pData, size);
}

/*
 * 函    数：DMA 发送完成后的清理函数
 */
static void serial_dma_cleanup(uint8_t *pBuffer)
{
    serial_buf_free_from_task(pBuffer);
}

/*
 * 函    数：串口空闲中断回调
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        
        osal_task_notify_from_isr(s_tx_task,
                                  0,
                                  (uint32_t)s_current_dma_buffer,
                                  OSAL_NOTIFY_SET_VALUE_OVERWRITE,
                                  &xHigherPriorityTaskWoken);
        
        s_current_dma_buffer = NULL;
        
        /* 只在最后统一调度 */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* ========================== 内存池实现 ========================== */

/**
  * 函    数：初始化内存池：将所有块串成单向链表，并创建计数信号量
  */
static void serial_pool_init(void)
{
    /* 创建计数信号量，最大和初始计数值均为池中块的总数 */
    s_buf_sem = osal_semaphore_create(SERIAL_NUM_BUFS, SERIAL_NUM_BUFS);
    if (s_buf_sem == NULL) {
        Error_Handler();
    }

    /* 将所有块插入空闲链表 */
    for (int i = 0; i < SERIAL_NUM_BUFS; i++) {
        FreeBlock_t *block = (FreeBlock_t *)s_pool_buf[i];
        block->next = (FreeBlock_t *)s_free_list;
        s_free_list = block;
    }
}

/**
  * 函    数：从池中分配一个块（任务上下文）
  *           先获取计数信号量，再从空闲链表中取块。
  */
static uint8_t *serial_buf_alloc_from_task(void)
{
    /* 尝试立即获得信号量（不可用时返回 NULL） */
    if (osal_semaphore_take(s_buf_sem, OSAL_NO_WAIT) != OSAL_OK) {
        return NULL;
    }

    uint8_t *buf = NULL;
    taskENTER_CRITICAL();

    /* 因信号量保证，空闲链表此时一定非空 */
    buf = (uint8_t *)s_free_list;
    s_free_list = s_free_list->next;

    taskEXIT_CRITICAL();
    return buf;
}

/**
  * 函    数：从池中分配一个块（中断上下文）
  */
static uint8_t *serial_buf_alloc_from_isr(BaseType_t *pxHigherPriorityTaskWoken)
{
    if (osal_semaphore_take_from_isr(s_buf_sem, pxHigherPriorityTaskWoken) != OSAL_OK) {
        return NULL;
    }

    uint8_t *buf = NULL;
    UBaseType_t uxSavedInterruptStatus;
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    buf = (uint8_t *)s_free_list;
    s_free_list = s_free_list->next;

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    return buf;
}

/**
  * 函    数：归还一个块到池中（任务上下文）
  */
static void serial_buf_free_from_task(uint8_t *pBuf)
{
    if (pBuf == NULL) return;

    FreeBlock_t *block = (FreeBlock_t *)pBuf;

    taskENTER_CRITICAL();

    block->next = (FreeBlock_t *)s_free_list;
    s_free_list = block;

    taskEXIT_CRITICAL();

    /* 归还块后释放信号量 */
    osal_semaphore_give(s_buf_sem);
}

/**
  * 函    数：归还一个块到池中（中断上下文）
  */
static void serial_buf_free_from_isr(uint8_t *pBuf, BaseType_t *pxHigherPriorityTaskWoken)
{
    if (pBuf == NULL) return;

    FreeBlock_t *block = (FreeBlock_t *)pBuf;

    UBaseType_t uxSavedInterruptStatus;
    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    block->next = (FreeBlock_t *)s_free_list;
    s_free_list = block;

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);

    osal_semaphore_give_from_isr(s_buf_sem, pxHigherPriorityTaskWoken);
}