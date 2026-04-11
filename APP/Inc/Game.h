#ifndef _GAME_H
#define _GAME_H

#include "main.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "freertos.h"
#include "Key.h"
#include "dino.h"
#include "ShowFrames.h"
#include "TranAnime.h"

extern TaskHandle_t MenuTaskHandle;
extern TaskHandle_t GameTaskHandle;

void Game(void);

#endif
