#include "MPU6050.h"
#include "osal.h"
// MPU6050 I2C访问的互斥锁，确保多任务环境下对MPU6050的I2C访问是线程安全的
static OSAL_MutexHandle MPU6050_MutexHandle = NULL;

// MPU6050 I2C总线锁定和解锁函数，使用FreeRTOS的递归互斥锁实现线程安全的I2C访问
void MPU6050_I2C_Lock(void)
{
    configASSERT(MPU6050_MutexHandle != NULL);
    OSAL_MutexLock(MPU6050_MutexHandle, portMAX_DELAY);
}

// MPU6050 I2C总线解锁函数，释放互斥锁
void MPU6050_I2C_Unlock(void)
{
    configASSERT(MPU6050_MutexHandle != NULL);
    OSAL_MutexUnlock(MPU6050_MutexHandle);
}


/**
 * @brief 配置 MPU6050 进入低功耗循环模式并启用运动唤醒中断
 * @param mot_thr  运动检测阈值（寄存器 0x1F，单位 1mg）
 * @param mot_dur  运动持续时间（寄存器 0x20，单位 1ms）
 * @param wake_freq 低功耗唤醒频率，取值：
 *                   0 = 1.25 Hz, 1 = 5 Hz, 2 = 20 Hz, 3 = 40 Hz
 */
void MPU6050_EnableMotionWakeup(uint8_t mot_thr, uint8_t mot_dur, uint8_t wake_freq)
{
  configASSERT(wake_freq <= 3U);
  // 1. 先关中断并清状态，避免旧中断被锁存后影响本次入睡
  MPU6050_WriteReg(MPU6050_INT_ENABLE, 0x00);
  MPU6050_ReadReg(MPU6050_INT_STATUS);

  // 2. 配置中断引脚：高电平有效、推挽输出、锁存模式
  MPU6050_WriteReg(MPU6050_INT_PIN_CFG, 0x20);

  // 3. 设置运动检测参数
  MPU6050_WriteReg(MPU6050_MOT_THR, mot_thr);
  MPU6050_WriteReg(MPU6050_MOT_DUR, mot_dur);
  MPU6050_WriteReg(MPU6050_MOT_DETECT_CTRL, 0x15);

  // 4. 为运动检测切到更高灵敏度量程（±2g），并启用DHPF=5Hz供运动检测路径使用
  MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x01);

  // 5. 配置 PWR_MGMT_2：
    //    - LP_WAKE_CTRL[1:0] = wake_freq
    //    - 关闭所有陀螺仪轴（STBY_XG/YG/ZG = 1）
    //    - 保持加速度计所有轴开启（STBY_*A = 0）
    uint8_t pwr_mgmt_2 = (wake_freq & 0x03) << 6;   // 写入 LP_WAKE_CTRL
    pwr_mgmt_2 |= 0x07;                              // 关闭所有陀螺仪轴
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, pwr_mgmt_2);

  // 6. 配置 PWR_MGMT_1：
    //    - CYCLE = 1（进入循环模式，需要 SLEEP = 0）
    //    - TEMP_DIS = 1（禁用温度传感器以省电）
    //    - CLKSEL = 0（内部 8MHz 振荡器，因陀螺仪已关闭）
    //    - SLEEP = 0（确保 CYCLE 生效）
    uint8_t pwr_mgmt_1 = 0;
    pwr_mgmt_1 |= (1 << 5);   // CYCLE = 1
    pwr_mgmt_1 |= (1 << 3);   // TEMP_DIS = 1
    pwr_mgmt_1 |= 0x00;       // CLKSEL = 0（内部振荡器）
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, pwr_mgmt_1);

    // 7. 最后使能运动中断，避免在配置中间过程产生误触发
    MPU6050_WriteReg(MPU6050_INT_ENABLE, 0x40);     // MOT_EN = 1
}

/**
 * @brief 退出运动唤醒模式，恢复到正常采样模式（与 MPU6050_Init 配置一致）
 * @note  应在 Stop 唤醒后第一时间调用，确保 MPU6050 恢复正常工作状态
 */
void MPU6050_DisableMotionWakeup(void)
{
    // 1. 禁用运动中断
    MPU6050_WriteReg(MPU6050_INT_ENABLE, 0x00);
    
    // 2. 清除中断状态（读 INT_STATUS 即可清除）
    MPU6050_ReadReg(MPU6050_INT_STATUS);
    
    // 3. 恢复中断引脚配置为默认值（高有效、推挽、脉冲模式）
    MPU6050_WriteReg(MPU6050_INT_PIN_CFG, 0x00);
    
    // 4. 恢复电源管理1：取消睡眠和循环模式，选择 X 轴陀螺仪时钟
    //    (对应 Init 中的 0x01: 取消睡眠，时钟源选择 X 轴陀螺仪)
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
    
    // 5. 恢复电源管理2：所有轴不待机（对应 Init 中的 0x00）
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);
    
    // 6. 恢复采样率分频、数字低通滤波器、量程等（确保与 Init 一致）
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);   // 采样率分频（与 Init 一致）
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);       // DLPF 配置（与 Init 一致）
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);  // 陀螺仪量程 ±2000°/s
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18); // 加速度计量程 ±16g
}

// 读取MPU6050的中断状态寄存器，获取当前的中断状态
uint8_t MPU6050_ReadIntStatus(void)
{
	return MPU6050_ReadReg(MPU6050_INT_STATUS);	//读取INT_STATUS寄存器的值，返回当前的中断状态
}

// 清除MPU6050的中断状态，通过读取INT_STATUS寄存器来清除中断标志
void MPU6050_ClearIntStatus(void)
{
	MPU6050_ReadReg(MPU6050_INT_STATUS);	//通过读取INT_STATUS寄存器来清除中断状态
}

/**
  * 函    数：MPU6050写寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	configASSERT(MPU6050_MutexHandle != NULL);
	MPU6050_I2C_Lock();
	HAL_I2C_Mem_Write(&hi2c2,
					  MPU6050_ADDRESS,
					  RegAddress,
					  I2C_MEMADD_SIZE_8BIT,
					  &Data,
					  1,
					  MPU6050_I2C_TIMEOUT);
	MPU6050_I2C_Unlock();
}

/**
  * 函    数：MPU6050读寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;

	configASSERT(MPU6050_MutexHandle != NULL);
	MPU6050_I2C_Lock();
	HAL_I2C_Mem_Read(&hi2c2,
					 MPU6050_ADDRESS,
					 RegAddress,
					 I2C_MEMADD_SIZE_8BIT,
					 &Data,
					 1,
					 MPU6050_I2C_TIMEOUT);
	MPU6050_I2C_Unlock();
	
	return Data;
}

/**
  * 函    数：MPU6050初始化
  * 参    数：无
  * 返 回 值：无
  */
void MPU6050_Init(void)
{
	MPU6050_MutexHandle = OSAL_MutexCreateRecursive();
	
	MX_I2C2_Init();									//初始化硬件I2C2
	
	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x04);		//采样率分频寄存器，配置采样率
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

/**
  * 函    数：MPU6050获取ID号
  * 参    数：无
  * 返 回 值：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * 函    数：MPU6050获取数据
  * 参    数：AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 参    数：GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 返 回 值：无
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	configASSERT(AccX != NULL);
	configASSERT(AccY != NULL);
	configASSERT(AccZ != NULL);
	configASSERT(GyroX != NULL);
	configASSERT(GyroY != NULL);
	configASSERT(GyroZ != NULL);

	uint8_t DataH, DataL;								//定义数据高8位和低8位的变量
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		//读取加速度计X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		//读取加速度计X轴的低8位数据
	*AccX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		//读取加速度计Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		//读取加速度计Y轴的低8位数据
	*AccY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		//读取加速度计Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		//读取加速度计Z轴的低8位数据
	*AccZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		//读取陀螺仪X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		//读取陀螺仪X轴的低8位数据
	*GyroX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		//读取陀螺仪Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		//读取陀螺仪Y轴的低8位数据
	*GyroY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		//读取陀螺仪Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		//读取陀螺仪Z轴的低8位数据
	*GyroZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
}
