#pragma once

/*******************************************************************************************************************************
 * @file   touch.h
 *
 * @brief  Header file for the screen responsiveness module
 *
 * @date   2026-04-DD
 * @author _____
 *******************************************************************************************************************************/

/* Standard library Headers */
#include <cstddef>

/* Inter-component Headers */
#include "welcomePage.h"
#include "secretScreen.h"
#include "hardware.h"
#include "SPI.h"

/* Intra-component Headers */

/*******************************************************************************************************************************
 * Private defines and enums
 *******************************************************************************************************************************/

enum ScreenType { 
    SCREEN_SPLASH = 0, 
    SCREEN_SECRET = 1, 
    SCREEN_GPS = 2 
};

/*******************************************************************************************************************************
 * Variables
 *******************************************************************************************************************************/

static ScreenType currentScreen = SCREEN_SPLASH;
static uint32_t TOUCH_DEBOUNCE_MS = 15;

/*******************************************************************************************************************************
 * Function declarations
 *******************************************************************************************************************************/

// Touch Responsive Helper Functions
static uint16_t xpt2046Read(uint8_t cmd);

// Touch Responsive Functions
static bool isTouched();

void touchSetup();
void touchHandleSwitch();
bool touchEarlyExit();

// Touch coordinate reading
bool getTouchCoordinates(uint16_t &x, uint16_t &y);

// Get current screen type
ScreenType getCurrentScreen();

/** @} */



