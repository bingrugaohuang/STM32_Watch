#include "mpu6050_driver.h"
#include "mpu6050_service.h"
#include "math.h"

void mpu_angle_calulate(MPU_RawData_t *raw_data, Attitude_t *attitude) {
    static float filtered_roll = 0;
    static float filtered_pitch = 0;
    float alpha = 0.75; // 新数据权重（越小越平滑，但响应越慢）
    
    // 计算俯仰角（Pitch）和横滚角（Roll）
    float denominator = sqrtf(raw_data->Accel_Y * raw_data->Accel_Y + raw_data->Accel_Z * raw_data->Accel_Z);
    if(denominator >= 0.0001f) { // 避免除以零
        attitude->pitch = atan2f(-raw_data->Accel_X, denominator) * 57.2958f; // 57.2958 = 180/π
    } else {
        attitude->pitch = raw_data->Accel_X > 0 ? -90.0f : 90.0f; // 当分母接近零时，俯仰角为 ±90°
    }

    attitude->roll = atan2f(raw_data->Accel_Y, raw_data->Accel_Z) * 57.2958f; // 57.2958 = 180/π

    // 应用低通滤波器平滑角度数据，同时添加死区
    float diff_pitch = attitude->pitch - filtered_pitch;
    float diff_roll = attitude->roll - filtered_roll;
    if(fabsf(diff_pitch) < 0.5f) { // 死区阈值为 0.5°
        attitude->pitch = filtered_pitch; // 保持上一次的平滑值
    }else{
        filtered_pitch = alpha * attitude->pitch + (1 - alpha) * filtered_pitch;
    }
    if(fabsf(diff_roll) < 0.5f) { // 死区阈值为 0.5°
        attitude->roll = filtered_roll; // 保持上一次的平滑值
    }else{
        filtered_roll = alpha * attitude->roll + (1 - alpha) * filtered_roll;
    }

    attitude->pitch = filtered_pitch;
    attitude->roll = filtered_roll;
}
