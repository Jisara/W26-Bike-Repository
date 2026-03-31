#include "gps_render.h"
#include "welcomepage.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool parseDistanceKm(const char *text, float &distanceKm) {
  if (!text) return false;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (!(isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.')) continue;
    char *end = nullptr;
    const float value = strtof(&text[i], &end);
    if (!end || end == &text[i]) continue;
    while (*end == ' ') ++end;

    if (strncmp(end, "km", 2) == 0) {
      distanceKm = value;
      return true;
    }
    if (strncmp(end, "m", 1) == 0) {
      distanceKm = value / 1000.0f;
      return true;
    }
  }
  return false;
}

static bool parseEtaMinutes(const char *text, int &minutes) {
  if (!text) return false;

  int hours = -1;
  int mins = -1;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (!isdigit(static_cast<unsigned char>(text[i]))) continue;
    char *end = nullptr;
    const int value = static_cast<int>(strtol(&text[i], &end, 10));
    if (!end || end == &text[i]) continue;
    while (*end == ' ') ++end;

    if (strncmp(end, "h", 1) == 0 || strncmp(end, "hr", 2) == 0) {
      hours = value;
    } else if (strncmp(end, "min", 3) == 0) {
      mins = value;
    }
  }

  if (hours >= 0 || mins >= 0) {
    minutes = (hours > 0 ? hours * 60 : 0) + (mins > 0 ? mins : 0);
    if (minutes == 0 && mins == 0) {
      minutes = 0;
    }
    return true;
  }
  return false;
}

static void drawWrappedText(const char *text, int x, int y, int maxWidth, int lineHeight, int maxLines) {
  if (!text || text[0] == '\0') {
    gfx->setCursor(x, y);
    gfx->print("Waiting for Google Maps...");
    return;
  }

  const char *p = text;
  for (int line = 0; line < maxLines && *p; ++line) {
    const char *lineStart = p;
    const char *lastSpace = nullptr;
    int count = 0;
    while (*p && *p != '\n') {
      ++count;
      if (*p == ' ') {
        lastSpace = p;
      }

      char probe[128];
      const int n = count >= static_cast<int>(sizeof(probe)) ? static_cast<int>(sizeof(probe)) - 1 : count;
      memcpy(probe, lineStart, n);
      probe[n] = '\0';
      if (textWidth(probe, 2) > maxWidth) {
        if (lastSpace && lastSpace > lineStart) {
          p = lastSpace;
        }
        break;
      }
      ++p;
    }

    int n = static_cast<int>(p - lineStart);
    while (n > 0 && lineStart[n - 1] == ' ') {
      --n;
    }

    char out[128];
    if (n >= static_cast<int>(sizeof(out))) {
      n = static_cast<int>(sizeof(out)) - 1;
    }
    memcpy(out, lineStart, n);
    out[n] = '\0';
    gfx->setCursor(x, y + (line * lineHeight));
    gfx->print(out);

    while (*p == ' ') ++p;
    if (*p == '\n') ++p;
  }
}

void drawGpsScreen(const char *navText, uint32_t secondsSinceUpdate, bool bleConnected) {
  gfx->fillScreen(C_BLACK);

  for (int i = 0; i < 2; i++) {
    gfx->drawRect(i, i, SCREEN_WIDTH - (i * 2), SCREEN_HEIGHT - (i * 2), C_CYAN);
  }

  const char *title = "MAPS NAV";
  gfx->setTextColor(C_CYAN);
  gfx->setTextSize(3);
  gfx->setCursor((SCREEN_WIDTH - textWidth(title, 3)) / 2, 10);
  gfx->print(title);

  // BLE status badge (source link status, not notification freshness)
  const uint16_t linkColor = bleConnected ? C_GREEN : C_RED;
  const char *linkText = bleConnected ? "BLE: CONNECTED" : "BLE: DISCONNECTED";
  gfx->fillRoundRect(10, 40, 150, 14, 4, linkColor);
  gfx->setTextColor(C_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(16, 43);
  gfx->print(linkText);

  int etaMinutes = -1;
  float distanceKm = -1.0f;
  const bool hasEta = parseEtaMinutes(navText, etaMinutes);
  const bool hasDistance = parseDistanceKm(navText, distanceKm);

  float speedKmh = -1.0f;
  if (hasEta && hasDistance && etaMinutes > 0 && distanceKm > 0.0f) {
    speedKmh = distanceKm / (static_cast<float>(etaMinutes) / 60.0f);
  }

  gfx->drawRoundRect(10, 58, 122, 58, 6, C_GRAY);
  gfx->drawRoundRect(140, 58, 122, 58, 6, C_GRAY);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(18, 66);
  gfx->print("SPEED");
  gfx->setCursor(148, 66);
  gfx->print("TIME");

  gfx->setTextSize(2);
  gfx->setTextColor(C_YELLOW);
  gfx->setCursor(18, 84);
  if (speedKmh >= 0.0f) {
    char speedBuf[24];
    snprintf(speedBuf, sizeof(speedBuf), "%.1f km/h", speedKmh);
    gfx->print(speedBuf);
  } else {
    gfx->print("--");
  }

  gfx->setTextColor(C_GREEN);
  gfx->setCursor(148, 84);
  if (hasEta) {
    char etaBuf[24];
    snprintf(etaBuf, sizeof(etaBuf), "%d min", etaMinutes);
    gfx->print(etaBuf);
  } else {
    gfx->print("--");
  }

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(12, 128);
  gfx->print("TURN INSTRUCTION");
  gfx->drawRoundRect(10, 138, SCREEN_WIDTH - 20, 266, 6, C_GRAY);

  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  drawWrappedText(navText, 18, 154, SCREEN_WIDTH - 36, 22, 10);

  gfx->setTextColor(C_GRAY);
  gfx->setTextSize(1);
  char updatedBuf[40];
  snprintf(updatedBuf, sizeof(updatedBuf), "Updated %lu s ago", static_cast<unsigned long>(secondsSinceUpdate));
  gfx->setCursor(12, SCREEN_HEIGHT - 24);
  gfx->print(updatedBuf);

  const char *hint = "Touch to cycle screens";
  gfx->setCursor(SCREEN_WIDTH - textWidth(hint, 1) - 10, SCREEN_HEIGHT - 12);
  gfx->print(hint);

  gfx->flush();
}
