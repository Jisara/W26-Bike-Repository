#include "navRender.h"

static void drawSegment(const char *start, int length, int x, int y) {
  char buf[96];
  const int n = (length >= static_cast<int>(sizeof(buf))) ? (sizeof(buf) - 1) : length;
  memcpy(buf, start, n);
  buf[n] = '\0';
  gfx->setCursor(x, y);
  gfx->print(buf);
}

static int findCaseInsensitive(const char *text, const char *needle) {
  if (!text || !needle || needle[0] == '\0') {
    return -1;
  }
  const int textLen = static_cast<int>(strlen(text));
  const int needleLen = static_cast<int>(strlen(needle));
  if (needleLen > textLen) {
    return -1;
  }

  for (int i = 0; i <= textLen - needleLen; ++i) {
    bool match = true;
    for (int j = 0; j < needleLen; ++j) {
      const char a = static_cast<char>(tolower(static_cast<unsigned char>(text[i + j])));
      const char b = static_cast<char>(tolower(static_cast<unsigned char>(needle[j])));
      if (a != b) {
        match = false;
        break;
      }
    }
    if (match) {
      return i;
    }
  }
  return -1;
}

static const char *detectBadge(const char *text, TurnKind &kind, uint16_t &badgeColor) {
  if (findCaseInsensitive(text, "u-turn") >= 0 || findCaseInsensitive(text, "uturn") >= 0) {
    kind = TurnKind::kUTurn;
    badgeColor = C_RED;
    return "U-TURN";
  }
  if (findCaseInsensitive(text, "left") >= 0) {
    kind = TurnKind::kLeft;
    badgeColor = C_GREEN;
    return "LEFT";
  }
  if (findCaseInsensitive(text, "right") >= 0) {
    kind = TurnKind::kRight;
    badgeColor = C_YELLOW;
    return "RIGHT";
  }
  kind = TurnKind::kStraight;
  badgeColor = C_GRAY;
  return "STRAIGHT";
}

static void drawTurnIcon(TurnKind kind, int x, int y, int w, int h, uint16_t color) {
  const int cx = x + (w / 2);
  const int cy = y + (h / 2);

  if (kind == TurnKind::kStraight) {
    gfx->fillRect(cx - 2, y + 8, 4, h - 16, color);
    gfx->fillTriangle(cx, y + 4, cx - 10, y + 16, cx + 10, y + 16, color);
    return;
  }

  if (kind == TurnKind::kUTurn) {
    gfx->fillRect(cx - 2, cy - 6, 4, 18, color);
    gfx->drawCircle(cx, cy - 8, 12, color);
    gfx->fillTriangle(cx - 14, cy - 8, cx - 4, cy - 16, cx - 4, cy, color);
    return;
  }

  const bool left = (kind == TurnKind::kLeft);
  const int xStem = left ? (cx + 8) : (cx - 8);
  const int xHead = left ? (cx - 12) : (cx + 12);

  gfx->fillRect(xStem - 2, y + 10, 4, 18, color);
  gfx->fillRect((left ? xHead : xStem), y + 10, (left ? (xStem - xHead) : (xHead - xStem)), 4, color);
  if (left) {
    gfx->fillTriangle(xHead - 4, y + 12, xHead + 6, y + 5, xHead + 6, y + 19, color);
  } else {
    gfx->fillTriangle(xHead + 4, y + 12, xHead - 6, y + 5, xHead - 6, y + 19, color);
  }
}

static int drawWrappedText(const char *text, int x, int y, int maxWidth, int textSize, int maxLines, uint16_t color) {
  if (!text || text[0] == '\0' || maxLines <= 0) {
    return 0;
  }

  gfx->setTextSize(textSize);
  gfx->setTextColor(color);

  const int charW = 6 * textSize;
  const int charH = 8 * textSize;
  const int maxCharsPerLine = maxWidth / charW;
  if (maxCharsPerLine <= 0) {
    return 0;
  }

  const char *p = text;
  int line = 0;
  while (*p != '\0' && line < maxLines) {
    while (*p == ' ') {
      ++p;
    }
    if (*p == '\0') {
      break;
    }

    int len = 0;
    int lastSpace = -1;
    while (p[len] != '\0' && len < maxCharsPerLine) {
      if (p[len] == ' ') {
        lastSpace = len;
      }
      ++len;
    }

    int take = len;
    if (p[len] != '\0' && lastSpace >= 0) {
      take = lastSpace;
    }

    drawSegment(p, take, x, y + (line * (charH + kLineGap)));

    p += take;
    if (*p == ' ') {
      ++p;
    }
    ++line;
  }
  return line;
}

void drawNavText(const char *text) {
  if (!gfx) {
    return;
  }

  char clipped[kMaxNavChars + 1] = {0};
  if (text == nullptr || text[0] == '\0') {
    strncpy(clipped, "Waiting for nav...", kMaxNavChars);
    clipped[kMaxNavChars] = '\0';
  } else {
    const size_t n = strnlen(text, kMaxNavChars);
    memcpy(clipped, text, n);
    clipped[n] = '\0';
  }

  // Bottom half is the navigation panel.
  const int navTop = SCREEN_HEIGHT / 2;
  const int navHeight = SCREEN_HEIGHT / 2;
  const int cardX = kMargin;
  const int cardY = navTop + kMargin;
  const int cardW = SCREEN_WIDTH - (kMargin * 2);
  const int cardH = navHeight - (kMargin * 2);

  TurnKind kind = TurnKind::kStraight;
  uint16_t badgeColor = C_GRAY;
  const char *badge = detectBadge(clipped, kind, badgeColor);

  char primary[kMaxNavChars + 1] = {0};
  char secondary[kMaxNavChars + 1] = {0};
  int split = findCaseInsensitive(clipped, " onto ");
  if (split < 0) {
    split = findCaseInsensitive(clipped, " on ");
  }

  if (split > 0) {
    const int pLen = split;
    memcpy(primary, clipped, pLen);
    primary[pLen] = '\0';

    const int offset = (findCaseInsensitive(clipped + split, " onto ") == 0) ? 6 : 4;
    strncpy(secondary, clipped + split + offset, kMaxNavChars);
    secondary[kMaxNavChars] = '\0';
  } else {
    strncpy(primary, clipped, kMaxNavChars);
    primary[kMaxNavChars] = '\0';
  }

  gfx->fillRect(0, navTop, SCREEN_WIDTH, navHeight, C_BLACK);
  gfx->drawRect(cardX, cardY, cardW, cardH, C_GRAY);

  const int headerH = 58;
  gfx->fillRect(cardX + 1, cardY + 1, cardW - 2, headerH, C_BLACK);

  const int badgeX = cardX + kCardPad;
  const int badgeY = cardY + 12;
  const int badgeW = 118;
  const int badgeH = 32;
  gfx->fillRoundRect(badgeX, badgeY, badgeW, badgeH, 8, badgeColor);
  gfx->setTextSize(2);
  gfx->setTextColor(C_BLACK);
  gfx->setCursor(badgeX + 10, badgeY + 9);
  gfx->print(badge);

  drawTurnIcon(kind, cardX + cardW - 64, cardY + 8, 44, 44, C_WHITE);

  const int textX = cardX + kCardPad;
  const int textY = cardY + headerH + 10;
  const int textW = cardW - (kCardPad * 2);
  const int linesUsed = drawWrappedText(primary, textX, textY, textW, 2, 4, C_WHITE);

  if (secondary[0] != '\0') {
    const int secondY = textY + (linesUsed * ((8 * 2) + kLineGap)) + 8;
    drawWrappedText(secondary, textX, secondY, textW, 1, 2, C_GRAY);
  }

  gfx->flush();
}
