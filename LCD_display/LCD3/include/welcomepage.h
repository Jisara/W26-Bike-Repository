#pragma once

#include "welcomePageAnim.h"
#include "secretScreen.h"
#include "touch.h"
#include "stats.h"

#include <hardware.h>
#include <cstring>

// ── Utility Functions ──
int textWidth(const char *text, int textSize);

// ── Button Regions (for touch detection) ──
#define GPS_BUTTON_X 220
#define GPS_BUTTON_Y 10
#define GPS_BUTTON_W 50
#define GPS_BUTTON_H 30

// ── Main Functions ──
void drawSplash();

static const uint32_t WELCOME_DURATION_MS = 10000;
enum AppScreen { APP_WELCOME, APP_STATS, APP_GPS };
static AppScreen appScreen = APP_WELCOME;
static uint32_t appStartMs = 0;
