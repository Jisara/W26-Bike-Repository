#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// ── Screen Configuration ──
#define LCD_BL 1
#define SCREEN_WIDTH 272
#define SCREEN_HEIGHT 480

// ── Colors ──
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_GREEN   0x07E0
#define C_GRAY    0x8410
#define C_YELLOW  0xFFE0
#define C_RED     0xF800
#define C_CYAN    0x07FF

// ── Global Graphics Objects (defined in main.cpp) ──
extern Arduino_DataBus  *bus;
extern Arduino_GFX      *panel;
extern Arduino_GFX      *gfx;

// ── Utility Functions ──
int textWidth(const char *text, int textSize);

// ── Button Regions (for touch detection) ──
#define GPS_BUTTON_X 220
#define GPS_BUTTON_Y 10
#define GPS_BUTTON_W 50
#define GPS_BUTTON_H 30

// ── Main Functions ──
void drawSplash();
void drawGpsButton();  // Draw GPS button on splash screen

static const uint32_t WELCOME_DURATION_MS = 10000;
enum AppScreen { APP_WELCOME, APP_STATS, APP_GPS };
static AppScreen appScreen = APP_WELCOME;
static uint32_t appStartMs = 0;

#endif