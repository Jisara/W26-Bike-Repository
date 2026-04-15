#pragma once

#include "welcomePage.h"

#include <inttypes.h>
#include <cstddef>

enum class TurnKind {
  kStraight,
  kLeft,
  kRight,
  kUTurn
};

// Bottom-half card layout on a 272x480 portrait display.
static constexpr int kMargin = 10;
static constexpr int kCardPad = 10;
static constexpr int kLineGap = 4;
static constexpr size_t kMaxNavChars = 180;

static void drawTurnIcon(TurnKind kind, int x, int y, int w, int h, uint16_t color);

// Draws navigation text into the bottom half of the screen.
// Text is wrapped and truncated to fit the region.
void drawNavText(const char *text);
