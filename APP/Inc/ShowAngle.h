#ifndef _SHOWANGLE_H_
#define _SHOWANGLE_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "Key.h"
#include <math.h>
#include "MPU6050.h"
#include "ShowFrames.h"
#include "TranAnime.h"
#include "common/app_handles.h"

typedef struct{
	float Roll;
	float Pitch;
	float Yaw;
}Angle;//ŷ���ǽṹ��


uint8_t MPU6050(void);
Angle MPU6050_Cauculate(void);

#endif
