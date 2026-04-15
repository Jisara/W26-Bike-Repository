#include "touch.h"

// ── Raw XPT2046 read ──
static uint16_t xpt2046Read(uint8_t cmd) {
  digitalWrite(TOUCH_CS, LOW);
  touchSPI.transfer(cmd);
  uint16_t hi = touchSPI.transfer(0x00);
  uint16_t lo = touchSPI.transfer(0x00);
  digitalWrite(TOUCH_CS, HIGH);
  return ((hi << 8 | lo) >> 3) & 0xFFF;
}

// ── Call once from setup() ──
void touchSetup() {
  pinMode(TOUCH_CS,  OUTPUT);
  pinMode(TOUCH_IRQ, INPUT);
  digitalWrite(TOUCH_CS, HIGH);
  touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touchSPI.setFrequency(2000000);
  Serial.println("Touch ready");
}

static bool isTouched() {
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  return xpt2046Read(0xB0) > 200;
}

// Get calibrated touch coordinates (272x480 display)
// Returns raw coordinates from XPT2046
bool getTouchCoordinates(uint16_t &x, uint16_t &y) {
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  
  // XPT2046 commands for X and Y axis readings
  uint16_t raw_x = xpt2046Read(0xD0);  // Y-coordinate (will be swapped)
  uint16_t raw_y = xpt2046Read(0x90);  // X-coordinate (will be swapped)
  
  // Simple calibration mapping (adjust based on your screen)
  // Assuming 4096 raw points map to display resolution
  x = (raw_y * 272) / 4096;
  y = (raw_x * 480) / 4096;
  
  return true;
}


// ── Call every loop() ──
void touchHandleSwitch() {
  uint32_t now = millis();

  if (currentScreen == SCREEN_SECRET && (now - lastSecretAnimTime >= SECRET_ANIM_INTERVAL_MS)) {
    lastSecretAnimTime = now;
    secretAnimFrame++;
    drawSecretScreen();
  }

  if (!isTouched()) return;

  if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) return;
  
  lastTouchTime = now;

  // Get touch coordinates
  uint16_t touch_x, touch_y;
  if (!getTouchCoordinates(touch_x, touch_y)) {
    return;
  }

  Serial.printf("Touch: x=%u, y=%u, screen=%d\n", touch_x, touch_y, (int)currentScreen);

  // Otherwise, cycle through screens
  switch (currentScreen) {
    case SCREEN_SPLASH:
      currentScreen = SCREEN_SECRET;
      secretAnimFrame = 0;
      lastSecretAnimTime = now;
      secretStaticDrawn = false;
      drawSecretScreen();
      Serial.println("-> Secret Screen");
      break;
    case SCREEN_SECRET:
      currentScreen = SCREEN_GPS;
      Serial.println("-> GPS Screen");
      break;
    case SCREEN_GPS:
      currentScreen = SCREEN_SPLASH;
      drawSplash();
      Serial.println("-> Splash");
      break;
  }
}

bool touchEarlyExit() {
  if (digitalRead(TOUCH_IRQ) == HIGH) return false;
  uint32_t now = millis();
  if (now - lastTouchTime < TOUCH_DEBOUNCE_MS) return false;
  lastTouchTime = now;
  currentScreen = SCREEN_SECRET;
  return true;
}

ScreenType getCurrentScreen() {
  return currentScreen;
}
