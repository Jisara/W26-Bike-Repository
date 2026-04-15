#pragma once

/*******************************************************************************************************************************
 * @file   hardware.h
 *
 * @brief  Header file for the hardware module
 *
 * @date   2026-04-DD
 * @author _____
 *******************************************************************************************************************************/

/* Standard library Headers */

/* Inter-component Headers */
#include <SPI.h>
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* Intra-component Headers */

/*******************************************************************************************************************************
 * Private defines and enums
 *******************************************************************************************************************************/

/*******************************************************************************************************************************
 * Variables
 *******************************************************************************************************************************/

// ── XPT2046 pins for JC4827W543 (resistive touch) ──
#define TOUCH_CS    38
#define TOUCH_IRQ    3
#define TOUCH_SCK   12
#define TOUCH_MISO  13
#define TOUCH_MOSI  11
#define BL_PIN 1

/* UART PIN DEFINITIONS */
#define RX_PIN = 17
#define TX_PIN = 18

// ── Touch SPI ──
static SPIClass touchSPI(HSPI);
static uint32_t lastTouchTime = 0;
static uint32_t lastSecretAnimTime = 0;
static uint8_t secretAnimFrame = 0;
static const uint16_t SECRET_ANIM_INTERVAL_MS = 120;
static bool secretStaticDrawn = false;

// ── Screen Configuration ──
#define SCREEN_WIDTH 272
#define SCREEN_HEIGHT 480

// ── Colors ──
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_GREEN   0x07E0
#define C_GRAY    0x8410
#define C_YELLOW  0xFFE0
#define C_RED     0xF800
#define C_CYAN    0x07FF

// ── Global Graphics Objects (defined in main.cpp) ──
extern Arduino_DataBus  *bus;
extern Arduino_GFX      *panel;
extern Arduino_GFX      *gfx;

/*******************************************************************************************************************************
 * Function declarations
 *******************************************************************************************************************************/

/** @} */