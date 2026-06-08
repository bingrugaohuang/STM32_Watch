/**
 * ================================================================
 *  oled_driver.c — 0.96寸 OLED 驱动（SSD1306/SH1106 兼容）
 *
 *  移植自：江协科技 OLED 驱动库 V2.0
 *  适配层：FreeRTOS + HAL + I2C_Driver_t 抽象接口
 *
 *  数据存储方式：
 *    横8点，低位在下，先从左到右，再从上到下。
 *    每一个 Bit 对应一个像素点。
 *    左上角为(0, 0)，横向为 X 轴 (0~127)，纵向为 Y 轴 (0~63)
 * ================================================================
 */

#include "oled_driver.h"
#include "i2c_interface.h"
#include "oled_common.h"
#include "log.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include "common_macro.h"

#if I2C1_SW_ENABLE
#include "bsp_i2c1_sw.h"
#else
#include "bsp_i2c1_hw.h"
#endif

/* ================== 模块标签（用于日志系统） ================== */
#define TAG "OLED"

/* ================== 静态变量 ================== */

/** OLED 显存缓冲区，所有显示操作都只对此缓冲区读写 */
uint8_t OLED_DisplayBuf[8][128];

/** I2C 驱动实例指针，初始化后指向软件 I2C1 驱动 */
static const I2C_Driver_t *i2c_drv = NULL;

/* ================== 局部工具函数（I2C 通信层） ================== */

/**
  * 函 数：发送 OLED 命令字节
  * 说 明：通过 I2C 抽象接口发送 1 字节命令
  */
static void OLED_WriteCmd(uint8_t cmd)
{
    i2c_drv->write(OLED_I2C_ADDR, OLED_CTRL_CMD, &cmd, 1);
}

/**
  * 函 数：发送 OLED 数据块
  * 说 明：通过 I2C 抽象接口发送批量数据
  */
static void OLED_WriteData(uint8_t *data, uint8_t len)
{
    i2c_drv->write(OLED_I2C_ADDR, OLED_CTRL_DATA, data, len);
}

/**
  * 函 数：设置 OLED 光标位置
  * 参 数：Page - 页地址 (0~7)，每页对应 8 个 Y 像素
  *         X    - 列地址 (0~127)
  */
static void OLED_SetCursor(uint8_t Page, uint8_t X)
{
    /* SH1106 芯片需要 X += 2，如果你的 OLED 是 SSD1306 则不需要 */
    // X += 2;

    OLED_WriteCmd(0xB0 | Page);                    /* 设置页地址         */
    OLED_WriteCmd(0x10 | ((X & 0xF0) >> 4));       /* 设置列地址高 4 位  */
    OLED_WriteCmd(0x00 | (X & 0x0F));               /* 设置列地址低 4 位  */
}

/* ================== 局部工具函数（数学辅助） ================== */

/**
  * 函 数：幂运算（整数）
  * 返 回 值：X 的 Y 次方
  */
static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) { Result *= X; }
    return Result;
}

/**
  * 函 数：PNPoly 算法 — 判断指定点是否在指定多边形内部
  * 参 数：nvert  - 多边形顶点数量
  *         vertx  - 多边形各顶点 X 坐标数组
  *         verty  - 多边形各顶点 Y 坐标数组
  *         testx, testy - 测试点坐标
  * 返 回 值：1 = 在内部，0 = 不在内部
  * 说 明：W. Randolph Franklin 算法
  */
static uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
    int16_t i, j, c = 0;
    for (i = 0, j = nvert - 1; i < nvert; j = i++)
    {
        if (((verty[i] > testy) != (verty[j] > testy)) &&
            (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
        {
            c = !c;
        }
    }
    return c;
}

/**
  * 函 数：判断指定点是否在指定角度范围内
  * 参 数：X, Y    - 指定点坐标
  *         StartAngle, EndAngle - 角度范围 (-180~180)
  *          水平向右为 0°，顺时针旋转，正角度向下
  * 返 回 值：1 = 在范围内，0 = 不在
  */
static uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
    int16_t PointAngle;
    PointAngle = atan2(Y, X) / 3.1415926535 * 180;
    if (StartAngle < EndAngle)
    {
        if (PointAngle >= StartAngle && PointAngle <= EndAngle) { return 1; }
    }
    else
    {
        if (PointAngle >= StartAngle || PointAngle <= EndAngle) { return 1; }
    }
    return 0;
}

/* ================== OLED 初始化 ================== */

/**
  * 函 数：OLED 初始化
  * 说 明：初始化 I2C 驱动、发送 OLED 初始化命令序列、清屏
  */
void OLED_Init(void)
{
    LOG_I(TAG, "OLED Init start...");

    /* 1. 获取 I2C 驱动实例 */
#if I2C1_SW_ENABLE
    LOG_I(TAG, "Using software I2C1 (PB6=SCL, PB7=SDA)");
    i2c_drv = I2C1_SW_GetDriver();
#else
    LOG_I(TAG, "Using hardware I2C1 (PB6=SCL, PB7=SDA)");
    i2c_drv = I2C1_HW_GetDriver();
#endif
    
    LOG_I(TAG, "Initializing I2C subsystem...");
    i2c_drv->init();
    LOG_I(TAG, "I2C1 initialized (PB6=SCL, PB7=SDA)");

    /* 2. 发送初始化命令序列 */
    OLED_WriteCmd(0xAE);    /* 关闭显示                                */
    OLED_WriteCmd(0xD5);    /* 设置显示时钟分频比/振荡器频率           */
    OLED_WriteCmd(0x80);    /* 推荐值                                   */
    OLED_WriteCmd(0xA8);    /* 设置多路复用率                           */
    OLED_WriteCmd(0x3F);    /* 1/64 占空比                              */
    OLED_WriteCmd(0xD3);    /* 设置显示偏移                             */
    OLED_WriteCmd(0x00);    /* 无偏移                                   */
    OLED_WriteCmd(0x40);    /* 设置显示起始行                           */
    OLED_WriteCmd(0x8D);    /* 使能充电泵                               */
    OLED_WriteCmd(0x14);    /* 充电泵设置                               */
    OLED_WriteCmd(0x20);    /* 设置内存地址模式                         */
    OLED_WriteCmd(0x00);    /* 水平寻址模式                             */
    OLED_WriteCmd(0xA1);    /* 设置段重映射 (左右翻转)                  */
    OLED_WriteCmd(0xC8);    /* 设置 COM 扫描方向 (上下翻转)             */
    OLED_WriteCmd(0xDA);    /* 设置 COM 引脚配置                        */
    OLED_WriteCmd(0x12);    /* 推荐值                                   */
    OLED_WriteCmd(0x81);    /* 设置对比度控制                           */
    OLED_WriteCmd(0xCF);    /* 推荐对比度                               */
    OLED_WriteCmd(0xD9);    /* 设置预充电周期                           */
    OLED_WriteCmd(0xF1);    /* 推荐值                                   */
    OLED_WriteCmd(0xDB);    /* 设置 VCOMH 电压倍率                      */
    OLED_WriteCmd(0x40);    /* 推荐值                                   */
    OLED_WriteCmd(0xA4);    /* 全局显示开启 (输出跟随 RAM 内容)         */
    OLED_WriteCmd(0xA6);    /* 正常显示 (非反色)                        */
    OLED_WriteCmd(0xAF);    /* 打开显示                                 */

    OLED_Clear();
    OLED_Update();

    LOG_I(TAG, "OLED Init complete");
}

/* ================== 更新函数 ================== */

/**
  * 函 数：全屏刷新 — 将整个显存缓冲区发送到 OLED 硬件
  * 说 明：遍历 8 个页，每页发送 128 字节
  */
void OLED_Update(void)
{
    uint8_t j;
    for (j = 0; j < 8; j++)
    {
        OLED_SetCursor(j, 0);
        OLED_WriteData(OLED_DisplayBuf[j], 128);
    }
}

/**
  * 函 数：局部刷新 — 仅更新指定区域
  * 说 明：跨页时会刷新整页范围内的数据
  */
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t j;
    int16_t Page, Page1;

    Page  = Y / 8;
    Page1 = (Y + Height - 1) / 8 + 1;
    if (Y < 0) { Page -= 1; Page1 -= 1; }

    for (j = Page; j < Page1; j++)
    {
        if (X >= 0 && X <= 127 && j >= 0 && j <= 7)
        {
            OLED_SetCursor(j, X);
            OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
        }
    }
}

/* ================== 清屏 / 反色 ================== */

void OLED_Clear(void)
{
    memset(OLED_DisplayBuf, 0x00, sizeof(OLED_DisplayBuf));
}

void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));
            }
        }
    }
}

void OLED_Reverse(void)
{
    uint8_t i, j;
    for (j = 0; j < 8; j++)
        for (i = 0; i < 128; i++)
            OLED_DisplayBuf[j][i] ^= 0xFF;
}

void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);
            }
        }
    }
}

/* ================== 字符显示 ================== */

/**
  * 函 数：显示一个 ASCII 字符
  * 说 明：根据字体大小从对应的字库数组中取出字模，调用 OLED_ShowImage 绘制
  */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
    if (FontSize == OLED_8X16)
        OLED_ShowImage(X, Y, 8, 16, OLED_F8x16[Char - ' ']);
    else if (FontSize == OLED_6X8)
        OLED_ShowImage(X, Y, 6, 8, OLED_F6x8[Char - ' ']);
    else if (FontSize == OLED_12X24)
        OLED_ShowImage(X, Y, 12, 24, OLED_F12x24[Char - ' ']);
}

/**
  * 函 数：显示字符串（支持 ASCII 和中文字符混合）
  * 说 明：自动识别 UTF-8 多字节字符，中文从 OLED_CF16x16 字库查找
  */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
    uint16_t i = 0;
    char     SingleChar[5];
    uint8_t  CharLength = 0;
    uint16_t XOffset = 0;
    uint16_t pIndex;

    while (String[i] != '\0')
    {
#ifdef OLED_CHARSET_UTF8
        /* ---- UTF-8 解码 ---- */
        if ((String[i] & 0x80) == 0x00)            /* 1-byte: 0xxxxxxx */
        {
            CharLength = 1;
            SingleChar[0] = String[i++]; SingleChar[1] = '\0';
        }
        else if ((String[i] & 0xE0) == 0xC0)       /* 2-byte: 110xxxxx */
        {
            CharLength = 2;
            SingleChar[0] = String[i++]; if (String[i] == '\0') break;
            SingleChar[1] = String[i++]; SingleChar[2] = '\0';
        }
        else if ((String[i] & 0xF0) == 0xE0)       /* 3-byte: 1110xxxx */
        {
            CharLength = 3;
            SingleChar[0] = String[i++]; if (String[i] == '\0') break;
            SingleChar[1] = String[i++]; if (String[i] == '\0') break;
            SingleChar[2] = String[i++]; SingleChar[3] = '\0';
        }
        else if ((String[i] & 0xF8) == 0xF0)       /* 4-byte: 11110xxx */
        {
            CharLength = 4;
            SingleChar[0] = String[i++]; if (String[i] == '\0') break;
            SingleChar[1] = String[i++]; if (String[i] == '\0') break;
            SingleChar[2] = String[i++]; if (String[i] == '\0') break;
            SingleChar[3] = String[i++]; SingleChar[4] = '\0';
        }
        else { i++; continue; }
#endif

#ifdef OLED_CHARSET_GB2312
        /* ---- GB2312 解码 ---- */
        if ((String[i] & 0x80) == 0x00)            /* 1-byte ASCII */
        {
            CharLength = 1;
            SingleChar[0] = String[i++]; SingleChar[1] = '\0';
        }
        else                                       /* 2-byte GB2312 */
        {
            CharLength = 2;
            SingleChar[0] = String[i++]; if (String[i] == '\0') break;
            SingleChar[1] = String[i++]; SingleChar[2] = '\0';
        }
#endif

        /* ---- Display ---- */
        if (CharLength == 1)
        {
            OLED_ShowChar(X + XOffset, Y, SingleChar[0], FontSize);
            XOffset += FontSize;
        }
        else
        {
            for (pIndex = 0; strcmp(OLED_CF16x16[pIndex].Index, "") != 0; pIndex++)
            {
                if (strcmp(OLED_CF16x16[pIndex].Index, SingleChar) == 0) break;
            }
            if (FontSize == OLED_8X16)
            {
                OLED_ShowImage(X + XOffset, Y, 16, 16, OLED_CF16x16[pIndex].Data);
                XOffset += 16;
            }
            else if (FontSize == OLED_6X8)
            {
                OLED_ShowChar(X + XOffset, Y, '?', OLED_6X8);
                XOffset += OLED_6X8;
            }
        }
    }
}

/* ================== 数字显示 ================== */

void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(X + i * FontSize, Y,
                      Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0) { OLED_ShowChar(X, Y, '+', FontSize); Number1 = Number; }
    else             { OLED_ShowChar(X, Y, '-', FontSize); Number1 = -Number; }
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(X + (i + 1) * FontSize, Y,
                      Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        OLED_ShowChar(X + i * FontSize, Y,
                      SingleNumber < 10 ? SingleNumber + '0' : SingleNumber - 10 + 'A', FontSize);
    }
}

void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(X + i * FontSize, Y,
                      Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
    }
}

void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number,
                        uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
    uint32_t PowNum, IntNum, FraNum;
    if (Number >= 0) { OLED_ShowChar(X, Y, '+', FontSize); }
    else             { OLED_ShowChar(X, Y, '-', FontSize); Number = -Number; }

    IntNum = (uint32_t)Number;
    Number -= IntNum;
    PowNum = OLED_Pow(10, FraLength);
    FraNum = (uint32_t)(Number * PowNum + 0.5);  /* 四舍五入 */
    IntNum += FraNum / PowNum;                    /* 处理进位 */

    OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);
    OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);
    OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

/* ================== 图像显示 ================== */

/**
  * 函 数：显示图像（位图）
  * 说 明：将 Image 数据写入显存缓冲区指定位置，支持跨页绘制
  */
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;

    OLED_ClearArea(X, Y, Width, Height);

    for (j = 0; j < (Height - 1) / 8 + 1; j++)
    {
        for (i = 0; i < Width; i++)
        {
            if (X + i >= 0 && X + i <= 127)
            {
                Page  = Y / 8;
                Shift = Y % 8;
                if (Y < 0) { Page -= 1; Shift += 8; }

                if (Page + j >= 0 && Page + j <= 7)
                    OLED_DisplayBuf[Page + j][X + i] |= Image[j * Width + i] << Shift;
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7)
                    OLED_DisplayBuf[Page + j + 1][X + i] |= Image[j * Width + i] >> (8 - Shift);
            }
        }
    }
}

/* ================== OLED_Printf ================== */

void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
    char    String[256];
    va_list arg;
    va_start(arg, format);
    vsnprintf(String, sizeof(String), format, arg);
    va_end(arg);
    OLED_ShowString(X, Y, String, FontSize);
}

/* ================== 点操作 ================== */

void OLED_DrawPoint(int16_t X, int16_t Y)
{
    if (X >= 0 && X <= 127 && Y >= 0 && Y <= 63)
        OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
}

uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
    if (X >= 0 && X <= 127 && Y >= 0 && Y <= 63)
    {
        if (OLED_DisplayBuf[Y / 8][X] & (0x01 << (Y % 8))) return 1;
    }
    return 0;
}

/* ================== Bresenham 直线 ================== */

void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    int16_t x, y, dx, dy, d, incrE, incrNE, temp;
    int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
    uint8_t yflag = 0, xyflag = 0;

    if (y0 == y1)  /* 水平线 */
    {
        if (x0 > x1) { temp = x0; x0 = x1; x1 = temp; }
        for (x = x0; x <= x1; x++) OLED_DrawPoint(x, y0);
    }
    else if (x0 == x1)  /* 垂直线 */
    {
        if (y0 > y1) { temp = y0; y0 = y1; y1 = temp; }
        for (y = y0; y <= y1; y++) OLED_DrawPoint(x0, y);
    }
    else  /* Bresenham 算法 */
    {
        if (x0 > x1) { temp = x0; x0 = x1; x1 = temp; temp = y0; y0 = y1; y1 = temp; }
        if (y0 > y1) { y0 = -y0; y1 = -y1; yflag = 1; }
        if (y1 - y0 > x1 - x0) { temp = x0; x0 = y0; y0 = temp; temp = x1; x1 = y1; y1 = temp; xyflag = 1; }

        dx = x1 - x0; dy = y1 - y0;
        incrE  = 2 * dy;
        incrNE = 2 * (dy - dx);
        d = 2 * dy - dx;
        x = x0; y = y0;

        /* 绘制起点 */
        if (yflag && xyflag) OLED_DrawPoint(y, -x);
        else if (yflag)      OLED_DrawPoint(x, -y);
        else if (xyflag)     OLED_DrawPoint(y, x);
        else                 OLED_DrawPoint(x, y);

        while (x < x1)
        {
            x++;
            if (d < 0) d += incrE;
            else       { y++; d += incrNE; }
            if (yflag && xyflag) OLED_DrawPoint(y, -x);
            else if (yflag)      OLED_DrawPoint(x, -y);
            else if (xyflag)     OLED_DrawPoint(y, x);
            else                 OLED_DrawPoint(x, y);
        }
    }
}

/* ================== 矩形 ================== */

void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    int16_t i, j;
    if (!IsFilled)
    {
        for (i = X; i < X + Width; i++)  { OLED_DrawPoint(i, Y); OLED_DrawPoint(i, Y + Height - 1); }
        for (i = Y; i < Y + Height; i++) { OLED_DrawPoint(X, i); OLED_DrawPoint(X + Width - 1, i); }
    }
    else
    {
        for (i = X; i < X + Width; i++)
            for (j = Y; j < Y + Height; j++) OLED_DrawPoint(i, j);
    }
}

/* ================== 三角形 ================== */

void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1,
                        int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
    int16_t i, j;
    int16_t vx[] = {X0, X1, X2};
    int16_t vy[] = {Y0, Y1, Y2};

    if (!IsFilled)
    {
        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X0, Y0, X2, Y2);
        OLED_DrawLine(X1, Y1, X2, Y2);
    }
    else
    {
        if (X1 < minx) minx = X1;  if (X2 < minx) minx = X2;
        if (Y1 < miny) miny = Y1;  if (Y2 < miny) miny = Y2;
        if (X1 > maxx) maxx = X1;  if (X2 > maxx) maxx = X2;
        if (Y1 > maxy) maxy = Y1;  if (Y2 > maxy) maxy = Y2;
        for (i = minx; i <= maxx; i++)
            for (j = miny; j <= maxy; j++)
                if (OLED_pnpoly(3, vx, vy, i, j)) OLED_DrawPoint(i, j);
    }
}

/* ================== Bresenham 圆 ================== */

void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
    int16_t x, y, d, j;
    d = 1 - Radius;
    x = 0;
    y = Radius;

    OLED_DrawPoint(X + x, Y + y); OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X + y, Y + x); OLED_DrawPoint(X - y, Y - x);
    if (IsFilled) { for (j = -y; j < y; j++) OLED_DrawPoint(X, Y + j); }

    while (x < y)
    {
        x++;
        if (d < 0) d += 2 * x + 1;
        else       { y--; d += 2 * (x - y) + 1; }
        OLED_DrawPoint(X + x, Y + y); OLED_DrawPoint(X + y, Y + x);
        OLED_DrawPoint(X - x, Y - y); OLED_DrawPoint(X - y, Y - x);
        OLED_DrawPoint(X + x, Y - y); OLED_DrawPoint(X + y, Y - x);
        OLED_DrawPoint(X - x, Y + y); OLED_DrawPoint(X - y, Y + x);
        if (IsFilled)
        {
            for (j = -y; j < y; j++) { OLED_DrawPoint(X + x, Y + j); OLED_DrawPoint(X - x, Y + j); }
            for (j = -x; j < x; j++) { OLED_DrawPoint(X - y, Y + j); OLED_DrawPoint(X + y, Y + j); }
        }
    }
}

/* ================== Bresenham 椭圆 ================== */

void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    int16_t x, y, j;
    int16_t a = A, b = B;
    float   d1, d2;
    x = 0; y = b;
    d1 = b * b + a * a * (-b + 0.5f);
    if (IsFilled) { for (j = -y; j < y; j++) { OLED_DrawPoint(X, Y + j); } }
    OLED_DrawPoint(X + x, Y + y); OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X - x, Y + y); OLED_DrawPoint(X + x, Y - y);

    while (b * b * (x + 1) < a * a * (y - 0.5f))
    {
        if (d1 <= 0) d1 += b * b * (2 * x + 3);
        else         { d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2); y--; }
        x++;
        if (IsFilled) { for (j = -y; j < y; j++) { OLED_DrawPoint(X + x, Y + j); OLED_DrawPoint(X - x, Y + j); } }
        OLED_DrawPoint(X + x, Y + y); OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y); OLED_DrawPoint(X + x, Y - y);
    }

    d2 = b * b * (x + 0.5f) * (x + 0.5f) + a * a * (y - 1) * (y - 1) - a * a * b * b;
    while (y > 0)
    {
        if (d2 <= 0) { d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3); x++; }
        else         { d2 += a * a * (-2 * y + 3); }
        y--;
        if (IsFilled) { for (j = -y; j < y; j++) { OLED_DrawPoint(X + x, Y + j); OLED_DrawPoint(X - x, Y + j); } }
        OLED_DrawPoint(X + x, Y + y); OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y); OLED_DrawPoint(X + x, Y - y);
    }
}

/* ================== 圆弧 ================== */

void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius,
                   int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    int16_t x, y, d, j;
    d = 1 - Radius; x = 0; y = Radius;

    if (OLED_IsInAngle( x,  y, StartAngle, EndAngle)) OLED_DrawPoint(X + x, Y + y);
    if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) OLED_DrawPoint(X - x, Y - y);
    if (OLED_IsInAngle( y,  x, StartAngle, EndAngle)) OLED_DrawPoint(X + y, Y + x);
    if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) OLED_DrawPoint(X - y, Y - x);
    if (IsFilled) { for (j = -y; j < y; j++)
        { if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) OLED_DrawPoint(X, Y + j); } }

    while (x < y)
    {
        x++;
        if (d < 0) d += 2 * x + 1;
        else       { y--; d += 2 * (x - y) + 1; }
        if (OLED_IsInAngle( x,  y, StartAngle, EndAngle)) OLED_DrawPoint(X + x, Y + y);
        if (OLED_IsInAngle( y,  x, StartAngle, EndAngle)) OLED_DrawPoint(X + y, Y + x);
        if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) OLED_DrawPoint(X - x, Y - y);
        if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) OLED_DrawPoint(X - y, Y - x);
        if (OLED_IsInAngle( x, -y, StartAngle, EndAngle)) OLED_DrawPoint(X + x, Y - y);
        if (OLED_IsInAngle( y, -x, StartAngle, EndAngle)) OLED_DrawPoint(X + y, Y - x);
        if (OLED_IsInAngle(-x,  y, StartAngle, EndAngle)) OLED_DrawPoint(X - x, Y + y);
        if (OLED_IsInAngle(-y,  x, StartAngle, EndAngle)) OLED_DrawPoint(X - y, Y + x);
        if (IsFilled)
        {
            for (j = -y; j < y; j++)
            {
                if (OLED_IsInAngle( x, j, StartAngle, EndAngle)) OLED_DrawPoint(X + x, Y + j);
                if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) OLED_DrawPoint(X - x, Y + j);
            }
            for (j = -x; j < x; j++)
            {
                if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) OLED_DrawPoint(X - y, Y + j);
                if (OLED_IsInAngle( y, j, StartAngle, EndAngle)) OLED_DrawPoint(X + y, Y + j);
            }
        }
    }
}
