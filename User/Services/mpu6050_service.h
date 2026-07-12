#ifndef MPU6050_SERVICE_H
#define MPU6050_SERVICE_H

#include <stdint.h>

typedef struct{
    float roll;         // 横滚角
    float pitch;        // 俯仰角
    uint32_t step_cnt;  // 步数计数
}Attitude_t;

// mpu6050队列获取函数，返回值为错误码，attitude为输出参数
uint8_t mpu6050_getqueue(Attitude_t *attitude);

#endif