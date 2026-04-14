#include "gps_render.h"
#include "welcomepage.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void drawWrappedText(const char *text, int x, int y, int maxWidth, int lineHeight, int maxLines);

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

bool gpsNavInstructionWantsMarquee(const char *text) {
  if (!text || text[0] == '\0') {
    return false;
  }
  const int maxW = SCREEN_WIDTH - 36;
  const char *p = text;
  for (;;) {
    const char *nl = strchr(p, '\n');
    const size_t len = nl ? static_cast<size_t>(nl - p) : strlen(p);
    if (len > 0) {
      char line[192];
      const size_t cap = sizeof(line) - 1;
      const size_t n = len > cap ? cap : len;
      memcpy(line, p, n);
      line[n] = '\0';
      if (textWidth(line, 2) > maxW) {
        return true;
      }
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
  return strlen(text) > 90;
}

// Horizontal circular scroll (similar idea to LV_LABEL_LONG_SCROLL_CIRCULAR).
static void drawMarqueeLineCircular(const char *line, int x, int y, int w, int textSize, uint16_t fg) {
  if (!line || line[0] == '\0') {
    return;
  }
  const int charW = 6 * textSize;
  const int lineH = 8 * textSize + 2;
  char padded[256];
  snprintf(padded, sizeof(padded), "   %s   ", line);
  const int len = static_cast<int>(strlen(padded));
  const int totalW = len * charW;
  gfx->setTextColor(fg);
  gfx->setTextSize(static_cast<uint8_t>(textSize));
  gfx->fillRect(x, y, w, lineH, C_BLACK);
  if (totalW <= w) {
    gfx->setCursor(x, y);
    gfx->print(padded);
    return;
  }
  const unsigned long off = (millis() / 40) % static_cast<unsigned long>(totalW > 0 ? totalW : 1);
  for (int rep = 0; rep < 2; ++rep) {
    int pen = x - static_cast<int>(off) + rep * totalW;
    for (int i = 0; i < len; ++i) {
      if (pen + charW > x && pen < x + w) {
        gfx->setCursor(pen, y);
        gfx->print(padded[i]);
      }
      pen += charW;
    }
  }
}

static void drawTurnInstruction(const char *navText) {
  const int x = 18;
  const int maxW = SCREEN_WIDTH - 36;
  const int lineH = 22;
  const int textSize = 2;

  if (!navText || navText[0] == '\0') {
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(static_cast<uint8_t>(textSize));
    gfx->setCursor(x, 154);
    gfx->print("Waiting for Google Maps...");
    return;
  }

  if (gpsNavInstructionWantsMarquee(navText)) {
    const char *nl = strchr(navText, '\n');
    if (nl) {
      char first[192];
      size_t l = static_cast<size_t>(nl - navText);
      if (l >= sizeof(first)) {
        l = sizeof(first) - 1;
      }
      memcpy(first, navText, l);
      first[l] = '\0';
      drawMarqueeLineCircular(first, x, 154, maxW, textSize, C_WHITE);
      drawWrappedText(nl + 1, x, 154 + lineH + 4, maxW, lineH, 8);
    } else {
      drawMarqueeLineCircular(navText, x, 154, maxW, textSize, C_WHITE);
    }
  } else {
    drawWrappedText(navText, x, 154, maxW, lineH, 10);
  }
}

static bool navTextChanged(const char *a, const char *b) {
  if (a == b) {
    return false;
  }
  if (!a || !b) {
    return true;
  }
  return strncmp(a, b, 191) != 0;
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

void drawGpsScreen(
    const char *navText,
    uint32_t secondsSinceUpdate,
    bool bleConnected,
    uint32_t rxSequence,
    uint32_t lastRxLength) {
  static bool s_hasFrame = false;
  static bool s_prevBleConnected = false;
  static uint32_t s_prevRxSequence = 0;
  static uint32_t s_prevRxLength = 0;
  static uint32_t s_prevAgeSeconds = UINT32_MAX;
  static char s_prevNavText[192] = {0};

  const char *safeNavText = (navText && navText[0] != '\0') ? navText : "Waiting for Google Maps...";
  const bool marquee = gpsNavInstructionWantsMarquee(safeNavText);
  const bool navChanged = navTextChanged(safeNavText, s_prevNavText);
  const bool linkChanged = !s_hasFrame || bleConnected != s_prevBleConnected;
  const bool rxChanged = !s_hasFrame || rxSequence != s_prevRxSequence || lastRxLength != s_prevRxLength;

  // Full redraw only when static dashboard content changed.
  const bool fullRedraw = !s_hasFrame || navChanged || linkChanged || rxChanged;
  if (fullRedraw) {
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
  char rxInfo[32];
  snprintf(rxInfo, sizeof(rxInfo), "RX#%lu LEN:%lu",
           static_cast<unsigned long>(rxSequence),
           static_cast<unsigned long>(lastRxLength));
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(170, 43);
  gfx->print(rxInfo);

    int etaMinutes = -1;
    float distanceKm = -1.0f;
    const bool hasEta = parseEtaMinutes(safeNavText, etaMinutes);
    const bool hasDistance = parseDistanceKm(safeNavText, distanceKm);

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

    const char *hint = "Touch to cycle screens";
    gfx->setTextColor(C_GRAY);
    gfx->setTextSize(1);
    gfx->setCursor(SCREEN_WIDTH - textWidth(hint, 1) - 10, SCREEN_HEIGHT - 12);
    gfx->print(hint);
  }

  // Redraw the instruction text region for marquee animation or new nav text.
  if (fullRedraw || navChanged || marquee) {
    gfx->fillRect(12, 140, SCREEN_WIDTH - 24, 262, C_BLACK);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(2);
    drawTurnInstruction(safeNavText);
  }

  if (fullRedraw || secondsSinceUpdate != s_prevAgeSeconds) {
    gfx->fillRect(12, SCREEN_HEIGHT - 24, 150, 12, C_BLACK);
    gfx->setTextColor(C_GRAY);
    gfx->setTextSize(1);
    char updatedBuf[40];
    snprintf(updatedBuf, sizeof(updatedBuf), "Updated %lu s ago", static_cast<unsigned long>(secondsSinceUpdate));
    gfx->setCursor(12, SCREEN_HEIGHT - 24);
    gfx->print(updatedBuf);
  }

  gfx->flush();

  s_hasFrame = true;
  s_prevBleConnected = bleConnected;
  s_prevRxSequence = rxSequence;
  s_prevRxLength = lastRxLength;
  s_prevAgeSeconds = secondsSinceUpdate;
  strncpy(s_prevNavText, safeNavText, sizeof(s_prevNavText) - 1);
  s_prevNavText[sizeof(s_prevNavText) - 1] = '\0';
}
