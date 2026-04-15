#pragma once

/*******************************************************************************************************************************
 * @file   navRender.h
 *
 * @brief  Header file for the navigation rendering module
 *
 * @date   2026-04-DD
 * @author _____
 *******************************************************************************************************************************/

/* Standard library Headers */
#include <inttypes.h>
#include <cstddef>

/* Inter-component Headers */
#include "welcomePage.h"

/* Intra-component Headers */

/*******************************************************************************************************************************
 * Private defines and enums
 *******************************************************************************************************************************/

enum class TurnKind {
  kStraight,
  kLeft,
  kRight,
  kUTurn
};

/*******************************************************************************************************************************
 * Variables
 *******************************************************************************************************************************/

// Bottom-half card layout on a 272x480 portrait display.
static constexpr int kMargin = 10;
static constexpr int kCardPad = 10;
static constexpr int kLineGap = 4;
static constexpr size_t kMaxNavChars = 180;


/*******************************************************************************************************************************
 * Function declarations
 *******************************************************************************************************************************/

static void drawTurnIcon(TurnKind kind, int x, int y, int w, int h, uint16_t color);

// Draws navigation text into the bottom half of the screen.
// Text is wrapped and truncated to fit the region.
void drawNavText(const char *text);

/** @} */

