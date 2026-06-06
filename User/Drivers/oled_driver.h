#ifndef __OLED_DRIVER_H
#define __OLED_DRIVER_H

#include <stdint.h>
#include "oled_data.h"

/* ================================================================
 *  字体大小定义（与字库数据 OLED_F6x8/OLED_F8x16/OLED_F12x24 对应）
 * ================================================================ */
#define OLED_8X16      8   /* 8 像素宽，16 像素高 */
#define OLED_6X8       6   /* 6 像素宽，8 像素高  */
#define OLED_12X24     12  /* 12 像素宽，24 像素高 */

/* ---- 填充标志 ---- */
#define OLED_UNFILLED  0   /* 不填充（仅边框） */
#define OLED_FILLED    1   /* 填充内部        */

/* ================================================================
 *  全局变量声明
 * ================================================================ */

/**
  * OLED 显存缓冲区。
  * 所有显示操作都只对这个缓冲区进行读写，调用 OLED_Update 后才会发送到 OLED 硬件。
  */
extern uint8_t OLED_DisplayBuf[8][128];

/* ================================================================
 *  初始化函数
 * ================================================================ */

/**
  * 函 数：OLED 初始化
  * 参 数：无
  * 返 回 值：无
  * 说 明：必须在首次使用 OLED 前调用。初始化 I2C、发送 OLED 初始化命令序列并清屏。
  */
void OLED_Init(void);

/* ================================================================
 *  更新函数（将显存数据发送到 OLED 硬件）
 * ================================================================ */

/**
  * 函 数：将整个显存缓冲区刷新到 OLED 屏幕
  */
void OLED_Update(void);

/**
  * 函 数：将显存缓冲区部分区域刷新到 OLED 屏幕
  * 参 数：X - 区域左上角横坐标 (0~127)
  *         Y - 区域左上角纵坐标 (0~63)
  *         Width  - 区域宽度 (0~128)
  *         Height - 区域高度 (0~64)
  * 说 明：局部刷新，比全屏更新更快。Y 坐标跨页时仍会刷新整页。
  */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/* ================================================================
 *  清屏 / 反色函数
 * ================================================================ */

void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/* ================================================================
 *  字符 / 字符串显示函数
 * ================================================================ */

void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/* ================================================================
 *  绘制函数
 * ================================================================ */

void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

#endif /* __OLED_DRIVER_H */
