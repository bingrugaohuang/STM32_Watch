#ifndef _SCREEN_BRIGHT_H
#define _SCREEN_BRIGHT_H

#include "tasks/Setting.h"

#define SCREEN_BRIGHTNESS_RAW_MIN		0x10U
#define SCREEN_BRIGHTNESS_RAW_MAX		0xFFU
#define SCREEN_BRIGHTNESS_STEP_SHORT	2
#define SCREEN_BRIGHTNESS_STEP_LONG	8

uint8_t Menu_Third_SetScreenBrightness(void);

#endif
