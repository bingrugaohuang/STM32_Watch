#ifndef _StackMonitor_Task_H
#define _StackMonitor_Task_H

#include "main.h"
#include "FreeRtos.h"
#include "usart.h"
#include "menu.h"
#include "stdio.h"
#include "string.h"

extern TaskHandle_t StackMonitorTaskHandle;

void StackMonitor_Task(void *argument);

#endif
