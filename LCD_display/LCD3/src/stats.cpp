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

static bool s_statsBaseDrawn = false;

struct DisplaySnapshot {
  float speedKmh;
  int gear;
  float batteryPct;
  float tripKm;
  float odoKm;
  float volts;
  float amps;
  float watts;
  float motorTempC;
  bool leftSignal;
  bool rightSignal;
  int hh;
  int mm;
};

static DisplaySnapshot s_lastDrawn = {};
static bool s_haveLastDrawn = false;

static inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }

static DisplaySnapshot captureSnapshot() {
  DisplaySnapshot snap{};
  snap.speedKmh = s.speedKmh;
  snap.gear = s.gear;
  snap.batteryPct = s.batteryPct;
  snap.tripKm = s.tripKm;
  snap.odoKm = s.odoKm;
  snap.volts = s.volts;
  snap.amps = s.amps;
  snap.watts = s.watts;
  snap.motorTempC = s.motorTempC;
  snap.leftSignal = s.leftSignal;
  snap.rightSignal = s.rightSignal;
  snap.hh = s.hh;
  snap.mm = s.mm;
  return snap;
}

static bool changed(float a, float b, float eps = 0.001f) {
  return fabsf(a - b) > eps;
}

static bool snapshotChanged(const DisplaySnapshot &a, const DisplaySnapshot &b) {
  return changed(a.speedKmh, b.speedKmh) ||
         a.gear != b.gear ||
         changed(a.batteryPct, b.batteryPct) ||
         changed(a.tripKm, b.tripKm) ||
         changed(a.odoKm, b.odoKm) ||
         changed(a.volts, b.volts) ||
         changed(a.amps, b.amps) ||
         changed(a.watts, b.watts) ||
         changed(a.motorTempC, b.motorTempC) ||
         a.leftSignal != b.leftSignal ||
         a.rightSignal != b.rightSignal ||
         a.hh != b.hh ||
         a.mm != b.mm;
}

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

static void drawDashboardStatic() {
  gfx->fillScreen(C_BLACK);
  drawScreenBorder(C_GREEN);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(22, 84);
  gfx->print("SPEED");
  gfx->setCursor(197, 84);
  gfx->print("GEAR");

  gfx->setTextSize(2);
  gfx->setCursor(98, 186);
  gfx->print("km/h");

  gfx->drawRoundRect(10, 70, 160, 140, 14, C_GRAY);
  gfx->drawRoundRect(180, 70, 82, 140, 14, C_GRAY);
  gfx->drawRoundRect(10, 230, 252, 78, 14, C_GRAY);
  gfx->drawRoundRect(10, 320, 252, 92, 14, C_GRAY);
}

static void drawDashboardDynamic() {
  // Top bar dynamic content
  gfx->fillRect(8, 8, SCREEN_WIDTH - 16, 34, C_BLACK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 14);
  gfx->printf("%.0f%%", s.batteryPct);
  drawBatteryBar(56, 13, 98, 20, s.batteryPct);
  drawPill(170, 10, 92, 28, C_WHITE, C_BLACK);
  gfx->setCursor(182, 14);
  gfx->printf("%02d:%02d", s.hh, s.mm);

  // Speed and gear values
  gfx->fillRect(20, 104, 146, 80, C_BLACK);
  gfx->fillRect(186, 104, 70, 80, C_BLACK);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(6);
  gfx->setCursor(22, 112);
  gfx->printf("%.0f", s.speedKmh);
  gfx->setCursor(196, 112);
  gfx->printf("%d", s.gear);

  // Card 1 values
  gfx->fillRect(20, 240, 236, 58, C_BLACK);
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(22, 242);
  gfx->printf("Trip: %.2f km", s.tripKm);
  gfx->setCursor(22, 272);
  gfx->printf("Odo : %.1f km", s.odoKm);

  // Card 2 values
  gfx->fillRect(20, 330, 236, 72, C_BLACK);
  gfx->setCursor(22, 332);
  gfx->printf("V: %.1fV   A: %.1fA", s.volts, s.amps);
  gfx->setCursor(22, 362);
  gfx->printf("Power: %.0f W", s.watts);
  gfx->setCursor(22, 392);
  gfx->printf("Motor: %.1f C", s.motorTempC);

  // Bottom blinking light indicators
  const int indicatorY = H - 34;
  const int indicatorClearY = indicatorY - 14;
  const int indicatorClearH = 32;
  gfx->fillRect(2, indicatorClearY, W - 4, indicatorClearH, C_BLACK);
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

  // Blink indicators in demo mode.
  const bool blink = ((now / 800) % 2) == 0;
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
  s_statsBaseDrawn = false;
  s_haveLastDrawn = false;
  drawDashboardStatic();
  s_statsBaseDrawn = true;
  drawDashboardDynamic();
  s_lastDrawn = captureSnapshot();
  s_haveLastDrawn = true;
}

void statsUpdateAndRender() {
  updateDemoStats();

  if (!s_statsBaseDrawn) {
    drawDashboardStatic();
    s_statsBaseDrawn = true;
  }

  // Change-driven redraw with a capped refresh rate for smooth live telemetry.
  static uint32_t lastDraw = 0;
  uint32_t now = millis();

  const DisplaySnapshot nowSnap = captureSnapshot();
  const bool hasChanged = !s_haveLastDrawn || snapshotChanged(nowSnap, s_lastDrawn);
  const uint32_t sinceLastDraw = now - lastDraw;
  const bool minIntervalReached = sinceLastDraw >= 120;
  const bool maxIntervalReached = sinceLastDraw >= 1000;

  if ((hasChanged && minIntervalReached) || maxIntervalReached) {
    lastDraw = now;
    drawDashboardDynamic();
    s_lastDrawn = nowSnap;
    s_haveLastDrawn = true;
  }
}
