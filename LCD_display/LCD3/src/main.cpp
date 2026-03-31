#include "secretscreen.h"
#include "welcomepage.h"
#include "welcomepageanim.h"
#include "ble_nav.h"
#include "gps_render.h"

#include <string.h>

static char latestNavText[181] = "Waiting for Maps notification...";
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

  // Handle BLE navigation updates
  if (bleNavHasUpdate()) {
    const char *incoming = bleNavGetText();
    if (!incoming || incoming[0] == '\0') {
      strncpy(latestNavText, "Waiting for Maps notification...", sizeof(latestNavText) - 1);
    } else {
      strncpy(latestNavText, incoming, sizeof(latestNavText) - 1);
    }
    latestNavText[sizeof(latestNavText) - 1] = '\0';
    lastNavUpdateMs = millis();
    bleNavClearUpdate();
  }
  
  // Draw GPS screen when active
  static ScreenType lastScreen = SCREEN_SPLASH;
  ScreenType currentScreen = getCurrentScreen();
  
  if (currentScreen == SCREEN_GPS && currentScreen != lastScreen) {
    const uint32_t ageSec = (lastNavUpdateMs == 0) ? 0 : (millis() - lastNavUpdateMs) / 1000;
    drawGpsScreen(latestNavText, ageSec, bleNavIsConnected());
    lastGpsRefreshMs = millis();
  } else if (currentScreen == SCREEN_GPS && ((millis() - lastGpsRefreshMs >= 1000) || connectionChanged)) {
    const uint32_t ageSec = (lastNavUpdateMs == 0) ? 0 : (millis() - lastNavUpdateMs) / 1000;
    drawGpsScreen(latestNavText, ageSec, bleNavIsConnected());
    lastGpsRefreshMs = millis();
  }
  
  lastScreen = currentScreen;
  
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.printf("Uptime: %lu s\n", millis() / 1000);
  }
}
