#ifndef _StackMonitor_Task_H
#define _StackMonitor_Task_H

#include "main.h"
#include "FreeRTOS.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include "common/app_handles.h"

void StackMonitor_Task(void *argument);

#endif
