#include <cstring>
#include "welcomepage.h"
#include "welcomepageanim.h"
#include "secretscreen.h"
#include "gps_render.h"
#include "stats.h"

// Default nav payload for GPS dashboard until phone/BLE data is wired in.
static String g_mapsDirectionText = "Waiting for Maps notification...";
static uint32_t g_lastNavUpdateMs = 0;
static uint32_t g_lastScreenTouchMs = 0;
static uint32_t g_lastGpsDrawMs = 0;

static bool didTouch(uint32_t nowMs) {
  if (digitalRead(TOUCH_IRQ) == HIGH) {
    return false;
  }
  if (nowMs - g_lastScreenTouchMs < TOUCH_DEBOUNCE_MS) {
    return false;
  }

  uint16_t touchX = 0;
  uint16_t touchY = 0;
  if (!getTouchCoordinates(touchX, touchY)) {
    return false;
  }

  g_lastScreenTouchMs = nowMs;
  return true;
}

void setup() {
  Serial.begin(115200);

  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  appStartMs = millis();

  touchSetup();
  drawSplash();
  Serial.println("Splash shown");

  Serial.println("Welcome running");
}

void loop() {
  uint32_t now = millis();

  if (appScreen == APP_WELCOME) {
    touchHandleSwitch();

    if (now - appStartMs >= WELCOME_DURATION_MS) {
      appScreen = APP_STATS;
      statsInit();
      Serial.println("-> Stats dashboard");
    }
    return;
  }

  if (appScreen == APP_STATS) {
    statsUpdateAndRender();

    if (didTouch(now)) {
      appScreen = APP_GPS;
      g_lastGpsDrawMs = 0;
      Serial.println("-> GPS dashboard");
    }
    return;
  }

  if (appScreen == APP_GPS) {
    if (g_lastGpsDrawMs == 0 || now - g_lastGpsDrawMs >= 200) {
      const uint32_t ageSec = (g_lastNavUpdateMs == 0) ? 0 : (now - g_lastNavUpdateMs) / 1000;
      drawGpsScreen(
          g_mapsDirectionText.c_str(),
          ageSec,
          false,
          0,
          static_cast<uint32_t>(g_mapsDirectionText.length()));
      g_lastGpsDrawMs = now;
    }

    if (didTouch(now)) {
      appScreen = APP_STATS;
      statsInit();
      Serial.println("-> Stats dashboard");
    }
  }
}