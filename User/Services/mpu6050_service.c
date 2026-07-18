#include <stdint.h>
#include "bsp_exti.h"
#include "osal.h"
#include "log.h"
#include "common_macro.h"
#include "mpu6050_driver.h"
#include "task_service.h"
#include "mpu_common.h"
#include "mpu6050_service.h"

// 根据初始化时写入 MPU6050_GYRO_CONFIG 寄存器的值来决定
#define MPU6050_ACCEL_FS_SEL 0     //  mpu加速度计量程选择，0: ±2g, 1: ±4g, 2: ±8g, 3: ±16g
#define MPU6050_QUEUE_LENGTH 5     // 队列长度，决定了 MPU6050 处理任务能缓存多少个 Attitude_t 数据包
#define MPU6050_ERROR_INTERVAL 220 //快速唤醒间隔，5Hz为220,20Hz为60,10Hz为120,50Hz为12

#if (MPU6050_ACCEL_FS_SEL == 0)
  #define ACCEL_SCALE 16384.0f
#elif (MPU6050_ACCEL_FS_SEL == 1)
  #define ACCEL_SCALE 8192.0f
#elif (MPU6050_ACCEL_FS_SEL == 2)
  #define ACCEL_SCALE 4096.0f
#elif (MPU6050_ACCEL_FS_SEL == 3)
  #define ACCEL_SCALE 2048.0f
#endif

extern void button_serve_start_timer(void); // 声明外部函数，用于启动按键扫描定时器

/* ========================静态变量=====================*/
// MPU6050 队列句柄
static osal_queue_handle_t mpu6050queue_handle = NULL;

// MPU6050 处理任务句柄
static osal_task_handle_t mpu6050task_handle = NULL;

// MPU6050 原始数据结构体
static MPU_RawData_t mpu_raw_data = {0};
static Attitude_t mpu_attitude = {0};

/* =======================静态函数===================== */

// MPU6050 中断回调函数
static void mpu6050_callback(void)
{
    // 在中断回调中使用任务通知来唤醒 MPU6050 处理任务
    if (mpu6050task_handle != NULL)
    {
        BaseType_t osal_higher_priority_task_woken = pdFALSE;
        osal_task_notify_from_isr(mpu6050task_handle, 0, 0, OSAL_NOTIFY_NO_ACTION, &osal_higher_priority_task_woken);
        portYIELD_FROM_ISR(osal_higher_priority_task_woken);
    }
}

// MPU6050 加速度数据处理函数，将原始加速度值转换为 g
static void mpu6050_accel_data_process(MPU_RawData_t *raw_data) {
    // 将原始加速度值转换为 g
    raw_data->Accel_X = (float)raw_data->Accel_X_RAW / ACCEL_SCALE;
    raw_data->Accel_Y = (float)raw_data->Accel_Y_RAW / ACCEL_SCALE;
    raw_data->Accel_Z = (float)raw_data->Accel_Z_RAW / ACCEL_SCALE;
}

// MPU6050 队列发送函数
static uint8_t mpu6050_sendqueue(Attitude_t *attitude){
    if(mpu6050queue_handle) {
        if(osal_queue_send(mpu6050queue_handle, attitude, 0) == pdTRUE) {
            LOG_D(TAG_MPU,"Queue send success");
            return COMMON_ERR_OK;
        } else {
            LOG_W(TAG_MPU,"Queue send failed");
            return COMMON_ERR_TIMEOUT;
        }
    } else {
        LOG_E(TAG_MPU,"Queue handle is NULL");
        return COMMON_ERR_NULL;
    }
}

// MPU6050 处理任务函数
static void mpu6050task(void* pvParameters)
{
    (void)pvParameters; // 避免未使用参数的编译警告
    bsp_exti_register_callback(mpu6050_callback); // 注册中断回调函数

    // 初始化步数检测器
    step_detector_init();

    static uint32_t last_wake_time = 0; // 上次唤醒时间戳
    static uint32_t rapid_count = 0; // 连续唤醒计数器

    while(1)
    {
       if(osal_task_notify_wait(0, 0xFFFFFFFF, NULL, OSAL_WAIT_FOREVER) == pdTRUE) // 等待任务通知
       {
            // 唤醒后先检查并退出运动检测，
            // 因为tickless函数无法保证唤醒后立即退出运动检测再进入mpu任务函数
            if(mpu6050_is_in_motion_detection_mode()) {
                mpu6050_exit_motion_detection_mode();
                LOG_I(TAG_MPU, "MPU6050 exited motion detection mode");
            }

            uint32_t now = osal_get_tick();
            uint32_t interval = now - last_wake_time;
            last_wake_time = now;

            // 检测连续唤醒的情况，如果间隔大于60ms，认为是快速唤醒
            // 因为快速唤醒说明无法读取mpu的数据
            // 此时Hal库的硬件i2c读写会出现一个25msBUSY超时等待，三次重发就会超过60ms
            if(interval > MPU6050_ERROR_INTERVAL) {
                rapid_count++;
                LOG_D(TAG_MPU, "MPU6050 rapid wakeup detected, interval: %lu ms, count: %lu", interval, rapid_count);
            }else{
                rapid_count = 0;
                LOG_D(TAG_MPU, "MPU6050 wakeup interval: %lu ms", interval);
            }
            // 如果连续唤醒次数超过3次，说明可能存在异常情况，可以在这里进行处理，比如延时一段时间，或者记录日志
            if(rapid_count >= 3){
                LOG_E(TAG_MPU, "MPU6050 rapid wakeup detected, count: %lu", rapid_count);
                osal_task_delay(100); // 延时100ms，避免过快唤醒
                MPU6050_Reset_i2c(); // 复位 MPU6050 I2C 驱动，重新初始化 I2C 驱动
                rapid_count = 0; // 重置计数器
            }
            
            LOG_D(TAG_MPU, "MPU6050 waked up");
           // 处理 MPU6050 数据
           if(MPU_IsDataReady()) {
                LOG_D(TAG_MPU, "MPU6050 Data Ready");
                if(MPU_Get_RawData(&mpu_raw_data) == COMMON_ERR_OK) {
                    LOG_D(TAG_MPU, "MPU6050 Read Accel Data Success");
                    LOG_D(TAG_MPU, "MPU6050 Raw Data: X=%d, Y=%d, Z=%d", mpu_raw_data.Accel_X_RAW, mpu_raw_data.Accel_Y_RAW, mpu_raw_data.Accel_Z_RAW);
                    mpu6050_accel_data_process(&mpu_raw_data);
                    // 获取当前时间戳
                    uint32_t current_time = osal_get_tick();
                    // 调用步数检测更新函数
                    mpu_attitude.step_cnt = step_detector_update(&mpu_raw_data, current_time);
                    LOG_D(TAG_MPU, "Step Count: %lu", mpu_attitude.step_cnt);
                    // 调用角度计算函数
                    mpu_angle_calulate(&mpu_raw_data, &mpu_attitude);
                    LOG_D(TAG_MPU, "Attitude: Roll=%.2f, Pitch=%.2f", mpu_attitude.roll, mpu_attitude.pitch);
                    // 将计算后的角度和步数发送到队列
                    mpu6050_sendqueue(&mpu_attitude);
                }
            }else{
                // 由于INT引脚与BTN_CFM共用PA0，可能会导致误读，因此在此处不打印错误日志，也不进行处理
                LOG_D(TAG_MPU, "MPU6050 Data Not Ready");
            }
            //测试
            //osal_task_delay(pdMS_TO_TICKS(20));

       }

#if STACK_MONITOR_ENABLE

        static uint32_t last_monitor_time = 0;
        uint32_t current_time = osal_get_tick();
        if (current_time - last_monitor_time >= 5000) { // 每5秒打印一次
            stackmonitor(mpu6050task_handle, "MPU6050Task");
            last_monitor_time = current_time;
        }    
       // 监控任务栈高水位标记

#endif
    }
}

/* =======================外部接口===================== */
// MPU6050 队列获取函数
uint8_t mpu6050_getqueue(Attitude_t *attitude){ 
    if(mpu6050queue_handle) {
        if(osal_queue_receive(mpu6050queue_handle, attitude, 0) == pdTRUE) {
            LOG_D(TAG_MPU,"Queue receive success");
            return COMMON_ERR_OK;
        } else {
            LOG_D(TAG_MPU,"Queue receive failed");
            return COMMON_ERR_TIMEOUT;
        }
    } else {
        LOG_E(TAG_MPU,"Queue handle is NULL");
        return COMMON_ERR_NULL;
    }
}

// 初始化 MPU6050 队列
void mpu6050_queue_init(void) {
    mpu6050queue_handle = osal_queue_create(MPU6050_QUEUE_LENGTH, sizeof(Attitude_t));
    if(!mpu6050queue_handle) {
        LOG_E(TAG_MPU,"Queue creation failed");
        Error_Handler();
    }
    LOG_I(TAG_MPU,"Queue creation completed");
}

// MPU6050 处理任务初始化函数
void mpu6050task_init(void){
    mpu6050task_handle = osal_task_create("MPU6050Task", mpu6050task, MPU6050TASK_STACK, NULL, MPU6050TASK_PRIORITY);
    if(!mpu6050task_handle) {
        LOG_E(TAG_MPU,"Initialization failed");
        Error_Handler();
    }
    LOG_I(TAG_MPU,"Initialization completed");
}
