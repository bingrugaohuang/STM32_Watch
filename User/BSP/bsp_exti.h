#ifndef BSP_EXTI_H
#define BSP_EXTI_H

#include <stdint.h>

void bsp_exti_register_callback(void (*callback)(void));

#endif