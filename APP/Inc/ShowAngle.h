#ifndef _SHOWANGLE_H_
#define _SHOWANGLE_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "math.h"
#include "MPU6050.h"
#include "ShowFrames.h"
#include "TranAnime.h"

extern TaskHandle_t MPU6050TaskHandle;

typedef struct{
	float Roll;
	float Pitch;
	float Yaw;
}Angle;//欧拉角结构体


uint8_t MPU6050(void);
Angle MPU6050_Cauculate(void);

#endif
