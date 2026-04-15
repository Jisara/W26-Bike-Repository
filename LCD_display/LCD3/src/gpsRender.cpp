#include "gpsRender.h"

struct NavDashboardData {
  char speed[24];
  char clock[16];
  char kmLeft[24];
  char eta[24];
  char turnDistance[24];
  char road[48];
  char turnText[64];
};

static void trimCopy(char *dst, size_t dstSize, const char *src) {
  if (!dst || dstSize == 0) {
    return;
  }
  dst[0] = '\0';
  if (!src) {
    return;
  }
  while (*src == ' ') {
    ++src;
  }
  size_t n = strnlen(src, dstSize - 1);
  while (n > 0 && src[n - 1] == ' ') {
    --n;
  }
  memcpy(dst, src, n);
  dst[n] = '\0';
}

static void trimInPlace(char *text) {
  if (!text) {
    return;
  }

  char *start = text;
  while (*start != '\0' && isspace(static_cast<unsigned char>(*start))) {
    ++start;
  }

  char *end = start + strlen(start);
  while (end > start && isspace(static_cast<unsigned char>(end[-1]))) {
    --end;
  }

  const size_t len = static_cast<size_t>(end - start);
  if (start != text) {
    memmove(text, start, len);
  }
  text[len] = '\0';
}

static bool isUnitTerminator(char c) {
  return c == '\0' || isspace(static_cast<unsigned char>(c)) || c == '.' || c == ',' || c == ')';
}

static bool segmentContainsIgnoreCase(const char *text, const char *needle) {
  if (!text || !needle || needle[0] == '\0') {
    return false;
  }

  const size_t needleLen = strlen(needle);
  if (needleLen == 0) {
    return false;
  }

  for (const char *p = text; *p != '\0'; ++p) {
    size_t i = 0;
    for (; i < needleLen; ++i) {
      const unsigned char c = static_cast<unsigned char>(p[i]);
      if (c == '\0' || tolower(c) != tolower(static_cast<unsigned char>(needle[i]))) {
        break;
      }
    }
    if (i == needleLen) {
      return true;
    }
  }
  return false;
}

static void stripEtaLabel(char *text) {
  if (!text) {
    return;
  }

  char *eta = strstr(text, "ETA");
  if (!eta) {
    return;
  }

  char *tail = eta;
  while (tail > text && isspace(static_cast<unsigned char>(tail[-1]))) {
    --tail;
  }
  *tail = '\0';
  trimInPlace(text);
}

static void initDashboardData(NavDashboardData &d) {
  trimCopy(d.speed, sizeof(d.speed), "--");
  trimCopy(d.clock, sizeof(d.clock), "--");
  trimCopy(d.kmLeft, sizeof(d.kmLeft), "--");
  trimCopy(d.eta, sizeof(d.eta), "--");
  trimCopy(d.turnDistance, sizeof(d.turnDistance), "--");
  trimCopy(d.road, sizeof(d.road), "--");
  trimCopy(d.turnText, sizeof(d.turnText), "Waiting for Google Maps...");
}

static bool parseTaggedPayload(const char *text, NavDashboardData &d) {
  if (!text || strncmp(text, "NAV|", 4) != 0) {
    return false;
  }
  initDashboardData(d);

  const char *p = text + 4;
  while (*p != '\0') {
    const char *sep = strchr(p, '|');
    const size_t tokenLen = sep ? static_cast<size_t>(sep - p) : strlen(p);
    if (tokenLen > 0) {
      char token[96];
      size_t n = tokenLen >= sizeof(token) ? sizeof(token) - 1 : tokenLen;
      memcpy(token, p, n);
      token[n] = '\0';
      char *eq = strchr(token, '=');
      if (eq) {
        *eq = '\0';
        const char *key = token;
        const char *value = eq + 1;
        if (strcmp(key, "SPEED") == 0) trimCopy(d.speed, sizeof(d.speed), value);
        else if (strcmp(key, "TIME") == 0) trimCopy(d.clock, sizeof(d.clock), value);
        else if (strcmp(key, "KM_LEFT") == 0) trimCopy(d.kmLeft, sizeof(d.kmLeft), value);
        else if (strcmp(key, "ETA") == 0) trimCopy(d.eta, sizeof(d.eta), value);
        else if (strcmp(key, "TURN_M") == 0) trimCopy(d.turnDistance, sizeof(d.turnDistance), value);
        else if (strcmp(key, "ROAD") == 0) trimCopy(d.road, sizeof(d.road), value);
        else if (strcmp(key, "TURN") == 0) trimCopy(d.turnText, sizeof(d.turnText), value);
      }
    }
    if (!sep) break;
    p = sep + 1;
  }
  return true;
}

static bool parseDistanceKm(const char *text, float &distanceKm) {
  if (!text) return false;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    if (!(isdigit(static_cast<unsigned char>(text[i])) || text[i] == '.')) continue;
    char *end = nullptr;
    const float value = strtof(&text[i], &end);
    if (!end || end == &text[i]) continue;
    while (*end == ' ') ++end;

    if ((end[0] == 'k' || end[0] == 'K') &&
        (end[1] == 'm' || end[1] == 'M') &&
        isUnitTerminator(end[2])) {
      distanceKm = value;
      return true;
    }
    if ((end[0] == 'm' || end[0] == 'M') && isUnitTerminator(end[1])) {
      distanceKm = value / 1000.0f;
      return true;
    }
  }
  return false;
}

static void buildAsciiParseToken(const char *src, char *dst, size_t dstSize) {
  if (!dst || dstSize == 0) {
    return;
  }
  dst[0] = '\0';
  if (!src) {
    return;
  }

  size_t j = 0;
  for (size_t i = 0; src[i] != '\0' && j + 1 < dstSize; ++i) {
    const unsigned char c = static_cast<unsigned char>(src[i]);
    if (c < 0x80) {
      dst[j++] = static_cast<char>(tolower(c));
    } else {
      // Normalize non-ASCII punctuation/spaces from notifications into separators.
      dst[j++] = ' ';
    }
  }
  dst[j] = '\0';
}

static bool parseGoogleMapsSentence(const char *text, NavDashboardData &d) {
  if (!text) {
    return false;
  }

  initDashboardData(d);

  bool foundAny = false;
  char lastToken[96] = {0};
  const char *p = text;
  while (*p != '\0') {
    const char *sep = strstr(p, "\xC2\xB7");
    const size_t tokenLen = sep ? static_cast<size_t>(sep - p) : strlen(p);
    if (tokenLen > 0) {
      char token[96];
      const size_t n = tokenLen >= sizeof(token) ? sizeof(token) - 1 : tokenLen;
      memcpy(token, p, n);
      token[n] = '\0';
      trimInPlace(token);

      if (token[0] != '\0') {
        trimCopy(lastToken, sizeof(lastToken), token);

        char parseToken[96];
        buildAsciiParseToken(token, parseToken, sizeof(parseToken));
        trimInPlace(parseToken);

        float distanceKm = -1.0f;
        if (segmentContainsIgnoreCase(parseToken, "eta") && strchr(parseToken, ':')) {
          trimCopy(d.eta, sizeof(d.eta), token);
          stripEtaLabel(d.eta);
          foundAny = true;
        } else if (parseDistanceKm(parseToken, distanceKm)) {
          if (strstr(parseToken, "km") != nullptr) {
            snprintf(d.kmLeft, sizeof(d.kmLeft), "%.1f km", distanceKm);
          } else {
            snprintf(d.turnDistance, sizeof(d.turnDistance), "%.0f m", distanceKm * 1000.0f);
          }
          foundAny = true;
        }
      }
    }
    if (!sep) {
      break;
    }
    p = sep + 2;
    while (*p == ' ') {
      ++p;
    }
  }

  // Always show the final section text in the next-turn text area.
  if (lastToken[0] != '\0') {
    trimCopy(d.road, sizeof(d.road), lastToken);
    foundAny = true;
  }

  return foundAny;
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
  (void)text;
  return false;
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

    NavDashboardData d;
    if (!parseTaggedPayload(safeNavText, d) && !parseGoogleMapsSentence(safeNavText, d)) {
      // Backward-compat fallback for older phone payloads.
      initDashboardData(d);
      int etaMinutes = -1;
      float distanceKm = -1.0f;
      const bool hasEta = parseEtaMinutes(safeNavText, etaMinutes);
      const bool hasDistance = parseDistanceKm(safeNavText, distanceKm);
      if (hasDistance && distanceKm > 0.0f) {
        snprintf(d.kmLeft, sizeof(d.kmLeft), "%.1f km", distanceKm);
      }
      if (hasEta) {
        snprintf(d.eta, sizeof(d.eta), "%d min", etaMinutes);
      }
      if (hasEta && hasDistance && etaMinutes > 0 && distanceKm > 0.0f) {
        snprintf(d.speed, sizeof(d.speed), "%.1f km/h", distanceKm / (static_cast<float>(etaMinutes) / 60.0f));
      }
      trimCopy(d.turnText, sizeof(d.turnText), safeNavText);
    }

    gfx->drawRoundRect(10, 58, 122, 52, 6, C_GRAY);
    gfx->drawRoundRect(140, 58, 122, 52, 6, C_GRAY);
    gfx->drawRoundRect(10, 118, 122, 52, 6, C_GRAY);
    gfx->drawRoundRect(140, 118, 122, 52, 6, C_GRAY);

    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(18, 66);
    gfx->print("SPEED");
    gfx->setCursor(148, 66);
    gfx->print("TIME");
    gfx->setCursor(18, 126);
    gfx->print("KM LEFT");
    gfx->setCursor(148, 126);
    gfx->print("ETA");

    gfx->setTextSize(2);
    gfx->setTextColor(C_YELLOW);
    gfx->setCursor(18, 84);
    gfx->print(d.speed);

    gfx->setTextColor(C_GREEN);
    gfx->setCursor(148, 84);
    gfx->print(d.clock);

    gfx->setTextColor(C_CYAN);
    gfx->setCursor(18, 144);
    gfx->print(d.kmLeft);
    gfx->setCursor(148, 144);
    gfx->print(d.eta);

    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(1);
    gfx->setCursor(12, 178);
    gfx->print("NEXT TURN");
    gfx->drawRoundRect(10, 188, SCREEN_WIDTH - 20, 216, 6, C_GRAY);

    gfx->fillRect(14, 192, SCREEN_WIDTH - 28, 210, C_BLACK);
    gfx->setTextColor(C_WHITE);
    gfx->setTextSize(4);
    gfx->setCursor(18, 206);
    gfx->print(d.turnDistance);

    gfx->setTextSize(2);
    gfx->setTextColor(C_YELLOW);
    gfx->setCursor(18, 252);
    gfx->print(d.road);

    gfx->setTextSize(1);
    gfx->setTextColor(C_GRAY);
    gfx->setCursor(18, 284);
    gfx->print(d.turnText);

    const char *hint = "Touch to cycle screens";
    gfx->setTextColor(C_GRAY);
    gfx->setTextSize(1);
    gfx->setCursor(SCREEN_WIDTH - textWidth(hint, 1) - 10, SCREEN_HEIGHT - 12);
    gfx->print(hint);
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
