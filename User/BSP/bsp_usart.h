/* ================== BSP/bsp_usart.h ================== */
#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>
#include <stddef.h>
/* 绝对不包含 HAL 库头文件，也不包含 FreeRTOS！保持纯净！ */

/* 1. 定义一个函数指针类型，上层如果要接手中断，就按这个格式写函数 */
typedef void (*bsp_usart_tx_cb_t)(void);

/* 硬件操作接口 */
void BSP_USART_Init(void);
void BSP_USART_SendDMA(const uint8_t *pData, size_t size);

/* 2. 提供一个注册接口，让上层把它的函数“挂”进来 */
void BSP_USART_RegisterTxCallback(bsp_usart_tx_cb_t cb);

#endif