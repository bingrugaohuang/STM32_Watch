#include "modules/ShowFrames.h"

static uint8_t frame_cnt = 0;
static uint32_t last_ms = 0;
static uint8_t fps = 0;

void ShowFrames(void)
{
    uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;//获取当前时间（毫秒）

    frame_cnt++;//帧数加一

    if(now_ms - last_ms >= 1000)
    {
        last_ms = now_ms;
        fps = frame_cnt;
        frame_cnt = 0;
    }

    OLED_ShowNum(116, 0, fps, 2, OLED_6X8);
}

