#ifndef _FLASHLIGHT_H_
#define _FLASHLIGHT_H_

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "tim.h"
#include "ShowFrames.h"

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t FlashlightTaskHandle;

void Show_Flashlight(void);

#endif
