#pragma once

#include "hardware.h"
#include "welcomePage.h"
#include <math.h>

// Canvas dimensions
static const int W = SCREEN_WIDTH;
static const int H = SCREEN_HEIGHT;

void statsInit();
void statsUpdateAndRender();

