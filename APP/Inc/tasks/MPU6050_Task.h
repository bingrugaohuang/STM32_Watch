#ifndef _MPU6050_TASK_H_
#define _MPU6050_TASK_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "Key.h"
#include <math.h>
#include "MPU6050.h"
#include "ShowFrames.h"
#include "ShowAngle.h"
#include "Gradienter.h"
#include "TranAnime.h"
#include "TaskMgr.h"

void Show_MPU6050(void);

#endif
