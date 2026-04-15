#pragma once

#include "secretScreen.h"
#include "touch.h"
#include "welcomepage.h"

void drawBike(int x, int y, uint16_t color, int scale = 1);
bool animateBikeAcrossBottom(uint32_t durationMs = 6000, uint16_t frameDelayMs = 30);
void animateBikeToW26FromRight(const char *modelText, int modelTextY, uint32_t durationMs = 2200, uint16_t frameDelayMs = 25);
