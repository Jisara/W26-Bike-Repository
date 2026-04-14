// code for stats dashboard page
#include "welcomepage.h"
#include "stats.h"
#include <math.h>

// Canvas dimensions
static const int W = SCREEN_WIDTH;
static const int H = SCREEN_HEIGHT;

// ---------- Demo “stats” (frontend data model) ----------
struct Stats {
  float speedKmh = 0;
  int gear = 1;
  float batteryPct = 85;
  float tripKm = 0.00;
  float odoKm = 124.6;
  float volts = 51.2;
  float amps = 7.8;
  float watts = 0;
  float motorTempC = 42.0;

  bool leftSignal = false;
  bool rightSignal = false;
  bool headlightOn = true;

  const char *mode = "SPORT";
  int hh = 12, mm = 45;
  uint32_t tripStartMs = 0;
} s;

static inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

// ---------- UI helpers ----------
static void drawScreenBorder(uint16_t color, int thickness = 2) {
  for (int i = 0; i < thickness; i++) {
    gfx->drawRect(i, i, SCREEN_WIDTH - (i * 2), SCREEN_HEIGHT - (i * 2), color);
  }
}

void drawBatteryBar(int x, int y, int w, int h, float pct) {
  pct = clampf(pct, 0, 100);
  gfx->drawRect(x, y, w, h, C_WHITE);
  int innerW = w - 4;
  int fillW = (int)(innerW * (pct / 100.0f));
  uint16_t col = (pct > 30) ? C_GREEN : C_RED;
  gfx->fillRect(x + 2, y + 2, fillW, h - 4, col);

  // little cap nub
  gfx->drawRect(x + w, y + (h/3), 6, h/3, C_WHITE);
}

void drawPill(int x, int y, int w, int h, uint16_t border, uint16_t fill) {
  gfx->fillRoundRect(x, y, w, h, h/2, fill);
  gfx->drawRoundRect(x, y, w, h, h/2, border);
}

void drawIndicatorArrow(bool on, bool left, int y) {
  if (!on) return;
  if (left) {
    gfx->fillTriangle(10, y, 38, y - 12, 38, y + 12, C_YELLOW);
  } else {
    gfx->fillTriangle(W - 10, y, W - 38, y - 12, W - 38, y + 12, C_YELLOW);
  }
}

static void drawDashboard() {
  gfx->fillScreen(C_BLACK);
  drawScreenBorder(C_GREEN);

  // ----- Top bar: battery + clock -----
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);

  gfx->setCursor(10, 14);
  gfx->printf("%.0f%%", s.batteryPct);
  drawBatteryBar(56, 13, 98, 20, s.batteryPct);

  drawPill(170, 10, 92, 28, C_WHITE, C_BLACK);
  gfx->setCursor(182, 14);
  gfx->printf("%02d:%02d", s.hh, s.mm);

  // ----- Speed + Gear cards -----
  gfx->drawRoundRect(10, 70, 160, 140, 14, C_GRAY);
  gfx->drawRoundRect(180, 70, 82, 140, 14, C_GRAY);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(22, 84);
  gfx->print("SPEED");
  gfx->setCursor(197, 84);
  gfx->print("GEAR");

  gfx->setTextSize(6);
  gfx->setCursor(22, 112);
  gfx->printf("%.0f", s.speedKmh);
  gfx->setCursor(196, 112);
  gfx->printf("%d", s.gear);

  gfx->setTextSize(2);
  gfx->setCursor(98, 186);
  gfx->print("km/h");

  // ----- Info cards -----
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);

  // Card 1: Trip & Odo
  gfx->drawRoundRect(10, 230, 252, 78, 14, C_GRAY);
  gfx->setCursor(22, 242);
  gfx->printf("Trip: %.2f km", s.tripKm);
  gfx->setCursor(22, 272);
  gfx->printf("Odo : %.1f km", s.odoKm);

  // Card 2: Power
  gfx->drawRoundRect(10, 320, 252, 92, 14, C_GRAY);
  gfx->setCursor(22, 332);
  gfx->printf("V: %.1fV   A: %.1fA", s.volts, s.amps);
  gfx->setCursor(22, 362);
  gfx->printf("Power: %.0f W", s.watts);
  gfx->setCursor(22, 392);
  gfx->printf("Motor: %.1f C", s.motorTempC);

  // Footer: Trip time
  uint32_t t = (millis() - s.tripStartMs) / 1000;
  int minutes = t / 60;
  int seconds = t % 60;
  gfx->setTextColor(C_GRAY);
  gfx->setCursor(10, 430);

  // ----- Bottom indicators -----
  drawIndicatorArrow(s.leftSignal, true, H - 34);
  drawIndicatorArrow(s.rightSignal, false, H - 34);

  gfx->flush();
}

// ---------- Demo animation (frontend-only) ----------
static void updateDemoStats() {
  uint32_t now = millis();
  float t = now / 1000.0f;

  // static speed and gear values
  s.speedKmh = 35.0f;
  s.gear = 3;

  // static power and temp values
  s.volts = 51.2f;
  s.amps = 7.8f;
  s.watts = s.volts * s.amps;
  s.motorTempC = 42.0f;

  // static battery
  s.batteryPct = 85.0f;

  // static trip (no accumulation)
  s.tripKm = 0.0f;
  s.odoKm = 124.6f;

  // blink indicators for looks (slower blink)
  bool blink = ((now / 800) % 2) == 0;
  s.leftSignal = blink;
  s.rightSignal = blink;

  // fake mode cycling every ~12s
  int m = (int)(t / 12.0f) % 3;
  s.mode = (m == 0) ? "ECO" : (m == 1) ? "SPORT" : "TURBO";

  // simple clock tick
  static uint32_t lastClock = 0;
  if (lastClock == 0) lastClock = now;
  if (now - lastClock >= 1000) {
    lastClock += 1000;
    // just advance minutes occasionally (demo)
    static int sec = 0;
    sec++;
    if (sec >= 60) { sec = 0; s.mm++; if (s.mm >= 60) { s.mm = 0; s.hh = (s.hh + 1) % 24; } }
  }
}

void statsInit() {
  s.tripStartMs = millis();
  drawDashboard();
}

void statsUpdateAndRender() {
  updateDemoStats();

  // UI refresh rate
  static uint32_t lastDraw = 0;
  uint32_t now = millis();
  if (now - lastDraw >= 150) { // ~6-7 FPS (smooth enough, low flicker)
    lastDraw = now;
    drawDashboard();
  }
}
