#include "secretscreen.h"
#include "welcomepage.h"
#include "welcomepageanim.h"
#include "ble_nav.h"
#include "gps_render.h"

// Phone → BLE nav string (updated only from loop(), never from the BLE callback).
String g_mapsDirectionText = "Waiting for Maps notification...";
volatile bool newMapsDataAvailable = false;
static uint32_t lastNavUpdateMs = 0;
static uint32_t lastGpsRefreshMs = 0;

void setup() {
  Serial.begin(115200);

  gfx->begin();
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  touchSetup();
  drawSplash();
  Serial.println("Splash shown");

  bleNavSetup();
  Serial.println("Ready for nav dashboard");
}

void loop() {
  touchHandleSwitch();

  const bool connectionChanged = bleNavConnectionChanged();
  if (connectionChanged) {
    Serial.printf("BLE link state: %s\n", bleNavIsConnected() ? "CONNECTED" : "DISCONNECTED");
  }

  if (bleNavHasUpdate()) {
    const uint32_t rxSeq = bleNavGetRxSequence();
    const char *incoming = bleNavGetText();
    if (incoming && incoming[0] != '\0') {
      g_mapsDirectionText = incoming;
      lastNavUpdateMs = millis();
      newMapsDataAvailable = true;
      Serial.printf("NAV APPLY seq=%lu len=%u: %s\n",
                    static_cast<unsigned long>(rxSeq),
                    static_cast<unsigned>(g_mapsDirectionText.length()),
                    g_mapsDirectionText.c_str());
    }
    bleNavClearUpdate();
  }

  static ScreenType lastScreen = SCREEN_SPLASH;
  ScreenType currentScreen = getCurrentScreen();
  const bool onGps = (currentScreen == SCREEN_GPS);
  const uint32_t nowMs = millis();
  const char *navCStr = g_mapsDirectionText.c_str();
  const uint32_t gpsIntervalMs =
      (onGps && gpsNavInstructionWantsMarquee(navCStr)) ? 120u : 1000u;

  if (onGps && currentScreen != lastScreen) {
    const uint32_t ageSec = (lastNavUpdateMs == 0) ? 0 : (nowMs - lastNavUpdateMs) / 1000;
    drawGpsScreen(navCStr, ageSec, bleNavIsConnected(),
                  bleNavGetRxSequence(),
                  static_cast<uint32_t>(bleNavGetLastRxLength()));
    lastGpsRefreshMs = nowMs;
    newMapsDataAvailable = false;
  } else if (onGps &&
             (newMapsDataAvailable || connectionChanged || (nowMs - lastGpsRefreshMs >= gpsIntervalMs))) {
    const uint32_t ageSec = (lastNavUpdateMs == 0) ? 0 : (nowMs - lastNavUpdateMs) / 1000;
    drawGpsScreen(navCStr, ageSec, bleNavIsConnected(),
                  bleNavGetRxSequence(),
                  static_cast<uint32_t>(bleNavGetLastRxLength()));
    lastGpsRefreshMs = nowMs;
    newMapsDataAvailable = false;
  }

  lastScreen = currentScreen;
  
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.printf("Uptime: %lu s\n", millis() / 1000);
    bleNavDebugPrintLastRx();
  }
}
