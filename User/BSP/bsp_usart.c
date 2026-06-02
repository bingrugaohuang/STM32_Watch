/* ================== BSP/bsp_usart.c ================== */
#include "bsp_usart.h"
#include "usart.h" /* 这里面包含了 stm32f1xx_hal.h 和 huart1 句柄，只有 bsp.c 能看 */

/* 这是一个私有的函数指针变量，用来存上层传进来的函数地址 */
static bsp_usart_tx_cb_t s_tx_cplt_cb = NULL;

void BSP_USART_SendDMA(const uint8_t *pData, size_t size) {
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)pData, size);
}

void BSP_USART_RegisterTxCallback(bsp_usart_tx_cb_t cb) {
    s_tx_cplt_cb = cb; /* 把上层传过来的函数记下来 */
}

/* * 3. 拦截 HAL 库的中断回调函数 
 * 这个函数本来是由 stm32f1xx_it.c 里的 USART1_IRQHandler 调用的
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    /* 检查是不是我们要的那个串口（如果有多个串口的话） */
    if (huart->Instance == USART1) {
        /* 如果上层注册了回调函数，就执行它 */
        if (s_tx_cplt_cb != NULL) {
            s_tx_cplt_cb(); 
        }
    }
}