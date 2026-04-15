#include "welcomepage.h"
#include "welcomepageanim.h"
#include "secretscreen.h"

// ── Bike Drawing & Animation ──

static constexpr int kBorderInset = 2;

static void fillBlackClamped(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) {
    return;
  }

  int x0 = x;
  int y0 = y;
  int x1 = x + w;
  int y1 = y + h;

  const int minX = kBorderInset;
  const int maxX = SCREEN_WIDTH - kBorderInset;
  if (x0 < minX) x0 = minX;
  if (x1 > maxX) x1 = maxX;
  if (y0 < 0) y0 = 0;
  if (y1 > SCREEN_HEIGHT) y1 = SCREEN_HEIGHT;

  if (x1 <= x0 || y1 <= y0) {
    return;
  }
  gfx->fillRect(x0, y0, x1 - x0, y1 - y0, C_BLACK);
}

static void clearBikeLane(int laneTop, int laneHeight) {
  fillBlackClamped(kBorderInset, laneTop, SCREEN_WIDTH - (kBorderInset * 2), laneHeight);
}

static void clearBikeBounds(int x, int y, int scale) {
  const int left = x - (6 * scale) - 2;
  const int top = y - (5 * scale) - 2;
  const int right = x + (36 * scale) + 2;
  const int bottom = y + (16 * scale) + 2;
  fillBlackClamped(left, top, right - left, bottom - top);
}

static void clearTextBounds(int x, int y, int width, int textSize) {
  const int textHeight = (8 * textSize) + 2;
  fillBlackClamped(x - 1, y - 1, width + 2, textHeight);
}

static bool pointInScreen(int x, int y) {
  return x >= kBorderInset && x < (SCREEN_WIDTH - kBorderInset) && y >= 0 && y < SCREEN_HEIGHT;
}

static bool circleFullyInScreen(int cx, int cy, int r) {
  return (cx - r) >= kBorderInset && (cx + r) < (SCREEN_WIDTH - kBorderInset) &&
         (cy - r) >= 0 && (cy + r) < SCREEN_HEIGHT;
}

void drawBike(int x, int y, uint16_t color, int scale) {
  const int wheelRadius = 6 * scale;
  const int rearWheelX = x;
  const int frontWheelX = x + (30 * scale);
  const int wheelY = y + (10 * scale);
  const int seatX = x + (10 * scale);
  const int seatY = y;
  const int handleX = x + (24 * scale);
  const int handleY = y - (2 * scale);

  if (circleFullyInScreen(rearWheelX, wheelY, wheelRadius)) {
    gfx->drawCircle(rearWheelX, wheelY, wheelRadius, color);
  }
  if (circleFullyInScreen(frontWheelX, wheelY, wheelRadius)) {
    gfx->drawCircle(frontWheelX, wheelY, wheelRadius, color);
  }

  if (pointInScreen(rearWheelX, wheelY) && pointInScreen(seatX, seatY)) {
    gfx->drawLine(rearWheelX, wheelY, seatX, seatY, color);
  }
  if (pointInScreen(seatX, seatY) && pointInScreen(frontWheelX, wheelY)) {
    gfx->drawLine(seatX, seatY, frontWheelX, wheelY, color);
  }
  if (pointInScreen(seatX, seatY) && pointInScreen(handleX, handleY)) {
    gfx->drawLine(seatX, seatY, handleX, handleY, color);
  }
  if (pointInScreen(handleX, handleY) && pointInScreen(frontWheelX, wheelY)) {
    gfx->drawLine(handleX, handleY, frontWheelX, wheelY, color);
  }

  const int seatBarX = seatX - (4 * scale);
  const int seatBarY = seatY - scale;
  const int seatBarW = 8 * scale;
  if (seatBarX >= kBorderInset && (seatBarX + seatBarW) < (SCREEN_WIDTH - kBorderInset) &&
      seatBarY >= 0 && seatBarY < SCREEN_HEIGHT) {
    gfx->drawFastHLine(seatBarX, seatBarY, seatBarW, color);
  }
  if (pointInScreen(handleX, handleY) && pointInScreen(handleX + (4 * scale), handleY - (3 * scale))) {
    gfx->drawLine(handleX, handleY, handleX + (4 * scale), handleY - (3 * scale), color);
  }
}

static bool bikeTouchesScreen(int x, int y, int scale) {
  const int left = x - (6 * scale) - 2;
  const int top = y - (5 * scale) - 2;
  const int right = x + (36 * scale) + 2;
  const int bottom = y + (16 * scale) + 2;
  return right > kBorderInset && left < (SCREEN_WIDTH - kBorderInset) && bottom > 0 && top < SCREEN_HEIGHT;
}

bool animateBikeAcrossBottom(uint32_t durationMs, uint16_t frameDelayMs) {
  const int bikeScale = 2;
  const int bikeWidth = 30 * bikeScale;
  const int bikeY = SCREEN_HEIGHT - 82;
  const int laneTop = bikeY - 16;
  const int laneHeight = 52;
  const int bikeStartX = -bikeWidth - 24;
  const int bikeEndX = SCREEN_WIDTH + 24;

  // insert best quote of all times 
  const char *taglineText = "Electrium Mobility's Greatest Bike";
  const int taglineSize = 1;
  const int taglineWidth = textWidth(taglineText, taglineSize);
  const int taglineY = bikeY - 30;
  const uint16_t effectiveDelayMs = (frameDelayMs > 16) ? 16 : frameDelayMs;

  const uint32_t startTime = millis();
  clearBikeLane(laneTop, laneHeight);
  gfx->setTextColor(C_YELLOW);
  gfx->setTextSize(taglineSize);
  gfx->setCursor((SCREEN_WIDTH - taglineWidth) / 2, taglineY);
  gfx->print(taglineText);
  gfx->flush();

  int prevBikeX = 0;
  bool hasPrevBike = false;

  while (true) {
    uint32_t elapsed = millis() - startTime;
    if (elapsed > durationMs) {
      break;
    }

  if (touchEarlyExit()) {
  gfx->fillScreen(C_BLACK);
  gfx->flush();
  drawSecretScreen();
  return true;  // was interrupted
}
    int bikeX = bikeStartX + static_cast<int>((static_cast<int64_t>(bikeEndX - bikeStartX) * elapsed) / durationMs);

    if (hasPrevBike) {
      clearBikeBounds(prevBikeX, bikeY, bikeScale);
    }

    if (bikeTouchesScreen(bikeX, bikeY, bikeScale)) {
      drawBike(bikeX, bikeY, C_GREEN, bikeScale);
      hasPrevBike = true;
      prevBikeX = bikeX;
    } else {
      hasPrevBike = false;
    }

    gfx->flush();
    delay(effectiveDelayMs);
  }

  if (hasPrevBike) {
    clearBikeBounds(prevBikeX, bikeY, bikeScale);
  }
  gfx->flush();
  return false;  // completed normally
}

void animateBikeToW26FromRight(const char *modelText, int modelTextY, uint32_t durationMs, uint16_t frameDelayMs) {
  const int bikeScale = 1;
  const int textSize = 2;
  const int bikeWidth = 30 * bikeScale;
  const int textWidth_val = textWidth(modelText, textSize);
  const int spacing = 10;
  const int totalWidth = textWidth_val + spacing + bikeWidth;
  
  const int bikeY = 130;
  const int laneTop = 125;
  const int laneHeight = 35;
  
  const int textStartX = (SCREEN_WIDTH - textWidth_val) / 2;
  const int textFinalX = (SCREEN_WIDTH - totalWidth) / 2;
  const int bikeFinalX = textFinalX + textWidth_val + spacing;
  const int bikeStartX = SCREEN_WIDTH + 40;
  const uint16_t effectiveDelayMs = (frameDelayMs > 16) ? 16 : frameDelayMs;

  const uint32_t startTime = millis();
  clearBikeLane(laneTop, laneHeight);

  int prevTextX = textStartX;
  int prevBikeX = bikeStartX;
  bool hasPrevFrame = false;

  while (true) {
    uint32_t elapsed = millis() - startTime;
    if (elapsed > durationMs) {
      break;
    }

    int textX = textStartX - static_cast<int>((static_cast<int64_t>(textStartX - textFinalX) * elapsed) / durationMs);
    int bikeX = bikeStartX - static_cast<int>((static_cast<int64_t>(bikeStartX - bikeFinalX) * elapsed) / durationMs);

    if (hasPrevFrame) {
      clearTextBounds(prevTextX, modelTextY, textWidth_val, textSize);
      clearBikeBounds(prevBikeX, bikeY, bikeScale);
    }

    gfx->setTextColor(C_YELLOW);
    gfx->setTextSize(textSize);
    gfx->setCursor(textX, modelTextY);
    gfx->print(modelText);

    if (bikeTouchesScreen(bikeX, bikeY, bikeScale)) {
      drawBike(bikeX, bikeY, C_GREEN, bikeScale);
    }

    prevTextX = textX;
    prevBikeX = bikeX;
    hasPrevFrame = true;

    gfx->flush();
    delay(effectiveDelayMs);
  }

  if (hasPrevFrame) {
    clearTextBounds(prevTextX, modelTextY, textWidth_val, textSize);
    clearBikeBounds(prevBikeX, bikeY, bikeScale);
  }
  gfx->setTextColor(C_YELLOW);
  gfx->setTextSize(textSize);
  gfx->setCursor(textFinalX, modelTextY);
  gfx->print(modelText);
  drawBike(bikeFinalX, bikeY, C_GREEN, bikeScale);
  gfx->flush();
}
