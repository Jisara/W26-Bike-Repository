#pragma once

/*******************************************************************************************************************************
 * @file   welcomePage.h
 *
 * @brief  Header file for the welcome page screen module
 *
 * @date   2026-04-DD
 * @author _____
 *******************************************************************************************************************************/

/* Standard library Headers */
#include <cstring>

/* Inter-component Headers */
#include "welcomePageAnim.h"
#include "secretScreen.h"
#include "touch.h"
#include "stats.h"
#include "hardware.h"

/* Intra-component Headers */

/*******************************************************************************************************************************
 * Private defines and enums
 *******************************************************************************************************************************/

 enum AppScreen { 
    APP_WELCOME, 
    APP_STATS, 
    APP_GPS 
};

/*******************************************************************************************************************************
 * Variables
 *******************************************************************************************************************************/

// ── Button Regions (for touch detection) ──
#define GPS_BUTTON_X 220
#define GPS_BUTTON_Y 10
#define GPS_BUTTON_W 50
#define GPS_BUTTON_H 30

static const uint32_t WELCOME_DURATION_MS = 10000;
static AppScreen appScreen = APP_WELCOME;
static uint32_t appStartMs = 0;

/*******************************************************************************************************************************
 * Function declarations
 *******************************************************************************************************************************/

int textWidth(const char *text, int textSize);
void drawSplash();

/** @} */



