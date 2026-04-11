#include "ShowAngle.h"

static int16_t ax, ay, az, gx, gy, gz;//MPU6050测得的三轴加速度和角速度
float Roll_g, Pitch_g, Yaw_g;//陀螺仪解算的欧拉角
float Roll_a, Pitch_a, Yaw_a;//加速度计解算的欧拉角
static float a = 0.63;//互补滤波系数
float Roll, Pitch, Yaw;//互补铝箔后的欧拉角
static float Delta_t = 0.005;//采样周期
static double pi = 3.14159265;

Angle MPU6050_Cauculate(void)
{
	MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
	
	//陀螺仪解算
	Roll_g = Roll + Delta_t * (float)gx;
	Pitch_g = Pitch + Delta_t * (float)gy;
	Yaw_g = Yaw + Delta_t * (float)gz;

	//加速度计解算
	Roll_a = atan2((-1) * ax, az) * 180 / pi;
	Pitch_a = atan2( ay, az) * 180 / pi;
	
	//互补滤波
	Roll = a * Roll_g + (1 - a) * Roll_a;
	Pitch = a * Pitch_g + (1 - a) * Pitch_a;
	Yaw = a * Yaw_g;

	Angle angle = {Roll, Pitch, Yaw};
	return angle;
}

static void ShowUI_MPU6050(void)
{
	OLED_Clear();
	
	OLED_ShowImage(0, 0, 16, 16, Return);
	OLED_ReverseArea(0, 0, 16, 16);
	OLED_Printf(0, 16, OLED_8X16,  "Roll:%.2f", Roll);
	OLED_Printf(0, 32, OLED_8X16, "Pitch:%.2f", Pitch);
	OLED_Printf(0, 48, OLED_8X16, "Yaw:%.2f", Yaw);
	ShowFrames();
	OLED_Update();
}

uint8_t MPU6050(void)
{
	uint8_t Key;

	while(1)
	{
		MPU6050_Cauculate();
		Key = Key_GetNum();
		if(Key == KEY_CONFIRM)
		{
			TranAnime(TRAN_DIR_RIGHT);//退出MPU6050界面，右移
			return 0;
		}
		ShowUI_MPU6050();
		vTaskDelay(5);
	}
}
