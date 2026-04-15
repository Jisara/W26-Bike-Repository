#ifndef SECRETSCREEN_H
#define SECRETSCREEN_H

#include <stdint.h>

// Screen types
enum ScreenType { SCREEN_SPLASH = 0, SCREEN_SECRET = 1, SCREEN_GPS = 2 };

// ── XPT2046 pins for JC4827W543 (resistive touch) ──
#define TOUCH_CS    38
#define TOUCH_IRQ    3
#define TOUCH_SCK   12
#define TOUCH_MISO  13
#define TOUCH_MOSI  11

#define TOUCH_DEBOUNCE_MS 600

void drawSecretScreen();
void touchSetup();
void touchHandleSwitch();
bool touchEarlyExit();

// Touch coordinate reading
bool getTouchCoordinates(uint16_t &x, uint16_t &y);

// Get current screen type
ScreenType getCurrentScreen();

#endif