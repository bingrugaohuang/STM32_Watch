#ifndef MPU_COMMON_H
#define MPU_COMMON_H

#include <stdint.h>
#include "mpu6050_driver.h"
#include "mpu6050_service.h"

// 步数检测初始化
void step_detector_init(void);

// 步数检测更新函数
uint32_t step_detector_update(MPU_RawData_t *raw_data, uint32_t cur_step_time);

// 步数获取函数（直接读取，无副作用）
uint32_t step_detector_get_count(void);

// 步数检测重置函数
void step_detector_reset(void);

// 角度计算函数
void mpu_angle_calulate(MPU_RawData_t *raw_data, Attitude_t *attitude);

#endif // MPU_COMMON_H