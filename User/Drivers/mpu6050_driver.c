#include "mpu6050_driver.h"
#include "i2c_interface.h"
#include "log.h"
#include "osal.h"
#include "common_macro.h"
// 包含软硬件 I2C2，内部会根据 I2C2_SW_ENABLE 宏选择使用软件 I2C 或硬件 I2C
#include "bsp_i2c2.h" 

static const I2C_Driver_t *i2c_drv = NULL;

// 全局标志，表示是否处于运动检测模式
static uint8_t g_mpu_in_motion_detection_mode = 0; 

// 复位 MPU6050 I2C 驱动，重新初始化 I2C 驱动
void MPU6050_Reset_i2c(void){
    if(i2c_drv != NULL && i2c_drv->init != NULL){
        i2c_drv->init(); // 重新初始化 I2C 驱动
        LOG_I(TAG_MPU, "MPU6050 I2C re-initialized.");
    }else{
        LOG_E(TAG_MPU, "MPU6050 I2C driver not initialized.");
    }
}

static uint8_t MPU_Write_Buffer(uint8_t reg, uint8_t *buffer, uint8_t len){
    uint8_t status = 1;
    uint8_t retry = 3;
    while(retry-- && status){
        status = i2c_drv->write(MPU_ADDR, reg, buffer, len);
        if(status != 0) {
            LOG_W(TAG_MPU, "MPU6050 Write Buffer: Reg 0x%02X <= [len=%d], Status: %d, Retry: %d", reg, len, status, 3 - retry);
        }
    }
    if(status == 0){
        LOG_D(TAG_MPU, "MPU6050 Write Buffer: Reg 0x%02X <= [len=%d]", reg, len);
    }else{
        LOG_E(TAG_MPU, "MPU6050 Write Buffer Failed: Reg 0x%02X <= [len=%d]", reg, len);
    }
    return status;
}

static void MPU_Write_Byte(uint8_t reg, uint8_t data){
    uint8_t temp;
    uint8_t status = 1;
    temp = data;
    status = MPU_Write_Buffer(reg, &temp, 1);
    if(status == 0){
        LOG_D(TAG_MPU, "MPU6050 Write: Reg 0x%02X <= 0x%02X", reg, data);
    }else{
        LOG_E(TAG_MPU, "MPU6050 Write Failed: Reg 0x%02X <= 0x%02X", reg, data);
    }
}

// 读取多个字节的数据
static uint8_t MPU_Read_Buffer(uint8_t reg, uint8_t *buffer, uint8_t len){
    uint8_t status = 1;
    uint8_t retry = 3;
    while(retry-- && status){
        status = i2c_drv->read(MPU_ADDR, reg, buffer, len);
        if(status != 0) {
            LOG_W(TAG_MPU, "MPU6050 Read Buffer: Reg 0x%02X => [len=%d], Status: %d, Retry: %d", reg, len, status, 3 - retry);
        }
    }
    if(status == 0){
        LOG_D(TAG_MPU, "MPU6050 Read Buffer: Reg 0x%02X => [len=%d]", reg, len);
    }else{
        LOG_E(TAG_MPU, "MPU6050 Read Buffer Failed: Reg 0x%02X => [len=%d]", reg, len);
    }
    return status;
}

// 读取单个字节的数据
static uint8_t MPU_Read_Byte(uint8_t reg){
    uint8_t temp = 0;
    uint8_t status = 1;
    status = MPU_Read_Buffer(reg, &temp, 1);

    if(status == 0){
        LOG_D(TAG_MPU, "MPU6050 Read: Reg 0x%02X => 0x%02X", reg, temp);    
    }else{
        LOG_E(TAG_MPU, "MPU6050 Read Failed: Reg 0x%02X", reg);
    }
    return temp;
}

void MPU6050_Init(void){
    LOG_I(TAG_MPU, "MPU6050 Init start...");
#if I2C2_SW_ENABLE
    i2c_drv = I2C2_SW_GetDriver(); // 获取软件 I2C2 驱动实例
#else
    i2c_drv = I2C2_HW_GetDriver(); // 获取硬件 I2C2 驱动实例
#endif
    i2c_drv->init(); // 初始化 I2C 驱动
    LOG_I(TAG_MPU, "MPU6050 Init complete.");

    //上电前硬件复位，写入 0x80 到 PWR_MGMT_1 寄存器，触发硬件复位
    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x80); // 硬件复位
    osal_task_delay(100); // 等待 100ms 硬件复位完成

    // 唤醒 MPU6050，写入 0x00 到 PWR_MGMT_1 寄存器
    //MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x00); // 唤醒 MPU6050

    // 切换时钟源到陀螺仪 X 轴，写入 0x01 到 PWR_MGMT_1 寄存器，低功耗模式下，MPU6050 默认使用内部 8MHz 振荡器作为时钟源
    // 切换到陀螺仪 X 轴可以提高精度
    //MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x01); // 切换时钟源到陀螺仪 X 轴

    // 配置陀螺仪量程为 ±2000°/s，写入 0x18 到 GYRO_CONFIG 寄存器
    //MPU_Write_Byte(MPU_GYRO_CFG_REG, 0x18); // 配置陀螺仪量程为 ±2000°/s

    // 配置加速度计量程为 ±2g，写入 0x00 到 ACCEL_CONFIG 寄存器
    MPU_Write_Byte(MPU_ACCEL_CFG_REG, 0x00); // 配置加速度计量程为 ±2g

    // 关闭 I2C 主模式，写入 0x00 到 USER_CTRL 寄存器
    MPU_Write_Byte(MPU_USER_CTRL_REG, 0X00); //I2C主模式关闭

    // 配置 FIFO 使能，写入 0x78 到 FIFO_EN 寄存器，低功耗模式下不使用 FIFO，避免占用过多资源
    //MPU_Write_Byte(MPU_FIFO_EN_REG, 0x78); // 配置 FIFO 使能
	
	MPU_Write_Byte(MPU_FIFO_EN_REG, 0X00);	 //关闭FIFO

    // 配置采样率为 1kHz，写入 0x13 到 SMPLRT_DIV 寄存器（采样率 = Gyro Output Rate / (1 + SMPLRT_DIV)）
    // 此处设置为 1kHz / (1 + 19) = 50Hz 的采样率，不够可设置为 9 （100Hz）或 0 （1kHz）
    MPU_Write_Byte(MPU_SAMPLE_RATE_REG, 0x13); // 配置采样率为 50Hz

    // 配置低通滤波器，写入 0x04 到 CONFIG 寄存器（低通滤波器截止频率约为 20Hz）
    // 先保守一些，为避免混叠，设置为采样率的一半以下，但延迟会增加，后续可根据实际情况调整
    MPU_Write_Byte(MPU_CFG_REG, 0x04); // 配置低通滤波器

    // 使能传感器
    //MPU_Write_Byte(MPU_PWR_MGMT2_REG, 0x00); // 使能所有传感器
 
    // 开启循环模式，将时钟源切换到内部8MHz振荡器，写入 0x20 到 PWR_MGMT_1 寄存器
    // 这样可以降低功耗，同时保持传感器工作
    MPU_Write_Byte(MPU_PWR_MGMT1_REG, 0x28);
  
    // 进入低功耗，仅保留加速度计用于计步或者抬手亮屏,唤醒频率设置为5Hz
    // 0x87为20Hz
    MPU_Write_Byte(MPU_PWR_MGMT2_REG, 0x47);

    // 0x80: Active Low, Push-Pull, Pulse
    MPU_Write_Byte(MPU_INTBP_CFG_REG, 0x00); 

    // 使能数据就绪中断（每次采样完成，INT引脚拉低）
    MPU_Write_Byte(MPU_INT_EN_REG, 0x01); // MPU_INT_ENABLE_REG 定义为 0x38

    // 验证通信
    uint8_t who_am_i = MPU_Read_Byte(MPU_WHO_AM_I);
    LOG_I(TAG_MPU, "MPU6050 WHO_AM_I: 0x%02X", who_am_i);

}

// 检查 MPU6050 数据是否准备好
uint8_t MPU_IsDataReady(void) {
    uint8_t int_status = MPU_Read_Byte(MPU_INT_STA_REG); // 地址 0x3A
    return (int_status & 0x01); // BIT0 是 RAW_DATA_RDY_INT
}

// 读取 MPU6050 的原始数据
uint8_t MPU_Get_RawData(MPU_RawData_t *raw_data){
    uint8_t buffer[6] = {0};

    // 读取加速度计数据
    if(MPU_Read_Buffer(MPU_ACCEL_XOUTH_REG, buffer, 6) == I2C_OK){
        raw_data->Accel_X_RAW = (int16_t)((buffer[0] << 8) | buffer[1]);
        raw_data->Accel_Y_RAW = (int16_t)((buffer[2] << 8) | buffer[3]);
        raw_data->Accel_Z_RAW = (int16_t)((buffer[4] << 8) | buffer[5]);
        LOG_D(TAG_MPU, "MPU6050 Accel Raw: X=%d, Y=%d, Z=%d", raw_data->Accel_X_RAW, raw_data->Accel_Y_RAW, raw_data->Accel_Z_RAW);

        return COMMON_ERR_OK;
    }else{
        LOG_W(TAG_MPU, "MPU6050 Read Accel Data Failed");

        return COMMON_ERR_TIMEOUT;
    }

}

// 进入运动检测模式接口
// mot_thr: 运动阈值，单位为 mg，范围为 0~255
// mot_dur: 运动持续时间，单位为 ms，范围为 0~255
// wake_freq: 唤醒频率，单位为 Hz，范围为 0~3，对应 1.25Hz、5Hz、20Hz、40Hz
void mpu6050_enter_motion_detection_mode(uint8_t mot_thr, uint8_t mot_dur, uint8_t wake_freq){
    // 设置全局标志，表示进入运动检测模式
    g_mpu_in_motion_detection_mode = 1;

    //先关中断并清状态，避免旧中断被锁存后影响本次入睡
    MPU_Write_Byte(MPU_INT_EN_REG, 0x00);
    //读取 INT_STATUS 清除中断状态
    MPU_Read_Byte(MPU_INT_STA_REG); 

    //未检测到运动时，运动检测计数器值减1，不加唤醒延迟
    MPU_Write_Byte(MPU_MDETECT_CTRL_REG, 0x01);

    //运动阈值，检测阈值寄存器0X1F,单位1mg,寄存器值=实际检测阈值 
    MPU_Write_Byte(MPU_MOTION_DET_REG, mot_thr);	 

    //检测时间，单位1ms 寄存器0X20
    MPU_Write_Byte(MPU_MOTION_DUR_REG, mot_dur);  

    //配置高通滤波器为5Hz，满量程为±2g
    MPU_Write_Byte(MPU_ACCEL_CFG_REG, 0x01);

    //设置唤醒频率，寄存器0X1B,LP_WAKE_CTRL[1:0] = wake_freq
    //写入 LP_WAKE_CTRL
    uint8_t pwr_mgmt_2 = (wake_freq & 0x03) << 6;   
    //关闭所有陀螺仪轴
    pwr_mgmt_2 |= 0x07; 
    //写入 PWR_MGMT_2 寄存器
    MPU_Write_Byte(MPU_PWR_MGMT2_REG, pwr_mgmt_2);                            
    
    //开启运动检测中断
    MPU_Write_Byte(MPU_INT_EN_REG, 0X40);	 
}

// 退出运动检测模式接口
void mpu6050_exit_motion_detection_mode(void){
    // 清除全局标志，表示退出运动检测模式
    g_mpu_in_motion_detection_mode = 0; 

    // 禁用运动中断
    MPU_Write_Byte(MPU_INT_EN_REG, 0x00);
    // 读取 INT_STATUS 清除中断状态
    MPU_Read_Byte(MPU_INT_STA_REG); 
    
    // 恢复唤醒频率为20Hz
    MPU_Write_Byte(MPU_PWR_MGMT2_REG, 0x47);

    // 恢复加速度计配置：±2g，无高通滤波
    MPU_Write_Byte(MPU_ACCEL_CFG_REG, 0x00);

    //使能数据就绪中断（每次采样完成，INT引脚拉低）
    MPU_Write_Byte(MPU_INT_EN_REG, 0x01);
}

uint8_t mpu6050_is_in_motion_detection_mode(void){
    return g_mpu_in_motion_detection_mode;
}