#include "mpu6050_driver.h"
#include "math.h"
#include "log.h"

// 步数检测器结构体
typedef struct{
    uint32_t step_cnt;
    uint32_t last_step_time;
    float    threshould;
    float    avg_mag;
    uint8_t  state;
}Step_Detector_t;

// 步数检测器实例
static Step_Detector_t step;

// 步数检测函数
void step_detector_init(void){
    step.step_cnt = 0;
    step.last_step_time = 0;
    step.threshould = 0.01f; // 阈值，单位为 g
    step.avg_mag = 0.0f;    // 初始重力基准
    step.state = 0;         // 初始状态为等待上升沿
}

// 步数检测更新函数
uint32_t step_detector_update(MPU_RawData_t *raw_data, uint32_t cur_step_time){
    // 计算加速度向量的模长
    float mag = (raw_data->Accel_X * raw_data->Accel_X + raw_data->Accel_Y * raw_data->Accel_Y + raw_data->Accel_Z * raw_data->Accel_Z);

    // 低通滤波，更新平均加速度模长
    static float filtered_mag = 0.0f;
    if(filtered_mag == 0.0f) {
        filtered_mag = mag; // 初始化滤波器
    } else {
        filtered_mag = 0.3f * filtered_mag + 0.7f * mag; 
    }

    static float avg_mag = 1.0f; // 初始重力基准为 1g
    avg_mag = 0.999f * avg_mag + 0.001f *filtered_mag; // 更新重力基准

    // 检测步数
    float high_ratio = 1.0f + step.threshould;
    float low_ratio  = 1.0f - step.threshould;
    float high_thresh = 1.1f*high_ratio * high_ratio * avg_mag; // 上升沿阈值
    float low_thresh  = low_ratio * low_ratio * avg_mag;   // 下降沿阈值
   
    uint8_t new_state = 0;
    if(filtered_mag > high_thresh) {
        new_state = 1; // 上升沿,脚着地冲击
    } else if(filtered_mag < low_thresh) {
        new_state = 0; // 下降沿，脚抬起失重
    } else {
        new_state = step.state; // 保持当前状态
    }

    // 宽度门限，确保上升沿和下降沿之间的时间间隔合理
    static uint32_t high_start_time = 0;
    if(step.state == 0 && new_state == 1) {
        high_start_time = cur_step_time; // 记录上升沿开始时间
    }

    if(step.state == 1 && new_state == 0){
        // 计算上升沿持续时间
        uint32_t high_width = (cur_step_time - high_start_time) % 0xFFFFFFFF; 
        // 计算时间间隔，考虑时间戳溢出
        uint32_t dt = (cur_step_time - step.last_step_time) % 0xFFFFFFFF;
        if(dt > 200 && high_width >= 10 && high_width <= 1500) { // 防止误判，确保步长间隔足够
            step.step_cnt++;
            step.last_step_time = cur_step_time;
            LOG_D(TAG_MPU, "Step detected! Total steps: %lu", step.step_cnt);
        }else{
            LOG_D(TAG_MPU, "Step detected wrong: %lu ms, high_width: %lu ms", dt, high_width);
        }
    } 

    step.state = new_state; // 更新状态

    static uint32_t logtime = 0;
    uint32_t dt = (cur_step_time - logtime) % 0xFFFFFFFF; // 考虑时间戳溢出
    if(dt > 200) { 
    LOG_D(TAG_MPU, "Step Detector: filtered_mag=%.3f, avg_mag=%.3f, high_thresh=%.3f, low_thresh=%.3f, state=%d", 
          filtered_mag, avg_mag, high_thresh, low_thresh, step.state);
    logtime = cur_step_time;
    }
    return step.step_cnt;
}

// 步数获取函数（直接读取，无副作用）
uint32_t step_detector_get_count(void) {
    return step.step_cnt;
}

// 步数检测重置函数
void step_detector_reset(void){
    step.step_cnt = 0;
    step.last_step_time = 0;
    step.state = 0;
    LOG_D(TAG_MPU, "Step detector reset");
}
