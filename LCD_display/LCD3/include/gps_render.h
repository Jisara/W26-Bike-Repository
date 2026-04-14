#ifndef GPS_RENDER_H
#define GPS_RENDER_H

#include <stdint.h>

// Draws the navigation dashboard screen from phone-provided Maps text.
// secondsSinceUpdate is used to show how fresh the instruction is.
void drawGpsScreen(
    const char *navText,
    uint32_t secondsSinceUpdate,
    bool bleConnected,
    uint32_t rxSequence,
    uint32_t lastRxLength);

// True when the turn-instruction area should use circular horizontal scrolling (LVGL-style).
bool gpsNavInstructionWantsMarquee(const char *text);

#endif
