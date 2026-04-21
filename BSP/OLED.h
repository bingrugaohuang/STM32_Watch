#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>

#define OLED_8X16       8
#define OLED_6X8        6
#define OLED_12X24      12

#define OLED_UNFILLED   0
#define OLED_FILLED     1

extern uint8_t OLED_DisplayBuf[8][128];
#define OLED_Buffer OLED_DisplayBuf

void OLED_I2C_Lock(void);
void OLED_I2C_Unlock(void);

void OLED_Init(void);
void OLED_SetContrast(uint8_t Contrast);
uint8_t OLED_GetContrast(void);

void OLED_Update(void);
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/* Compatibility extensions kept for STM32_Watch app logic */
//uint8_t OLED_ShowCharWithReverse(uint8_t Line, uint8_t Column, char Char, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowStringWithReverse(uint8_t Line, uint8_t Column, char *String, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowSignedNumWithReverse(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowHexNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowBinNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode);
//uint8_t OLED_ShowCN(uint8_t Line, uint8_t Column, uint8_t Num, uint8_t mode);
//uint8_t OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t mode);
//uint8_t OLED_ShowImageWithReverse(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image, uint8_t mode);
//void OLED_ShowCharPixel(int16_t X, int16_t Y, char Char);
//void OLED_ShowNumPixel(int16_t X, int16_t Y, uint32_t Number, uint8_t Length);

#endif
