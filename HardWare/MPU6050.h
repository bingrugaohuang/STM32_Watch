#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"
#include "i2c.h"
#include "MPU6050_Reg.h"



void MPU6050_I2C_Lock(void);
void MPU6050_I2C_Unlock(void);

uint8_t MPU6050_ReadIntStatus(void);
void MPU6050_ClearIntStatus(void);
void MPU6050_EnableMotionWakeup(uint8_t mot_thr, uint8_t mot_dur, uint8_t wake_freq);
void MPU6050_DisableMotionWakeup(void);

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);

void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);


#endif
