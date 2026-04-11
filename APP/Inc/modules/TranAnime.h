#ifndef _TRANANIME_H
#define _TRANANIME_H

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "FreeRTOS.h"
#include "task.h"

#define TRANSITION_STEP      32
#define TRANSITION_DELAY_MS  8

#define TRAN_DIR_LEFT        0
#define TRAN_DIR_RIGHT       1

void TranAnime(uint8_t dir);

#endif
