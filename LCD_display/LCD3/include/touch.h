#pragma once

#include "welcomePage.h"
#include "secretScreen.h"
#include "hardware.h"

#include <cstddef>
#include <SPI.h>

enum ScreenType { 
    SCREEN_SPLASH = 0, 
    SCREEN_SECRET = 1, 
    SCREEN_GPS = 2 
};

static ScreenType currentScreen = SCREEN_SPLASH;

// Touch Responsive Helper Functions
static uint16_t xpt2046Read(uint8_t cmd);

// Touch Responsive Functions
static bool isTouched();
static uint32_t TOUCH_DEBOUNCE_MS = 5;
void touchSetup();
void touchHandleSwitch();
bool touchEarlyExit();

// Touch coordinate reading
bool getTouchCoordinates(uint16_t &x, uint16_t &y);

// Get current screen type
ScreenType getCurrentScreen();

