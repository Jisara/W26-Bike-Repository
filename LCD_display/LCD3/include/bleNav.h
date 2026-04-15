#pragma once

/*******************************************************************************************************************************
 * @file   bleNav.h
 *
 * @brief  Header file for the BLE navigation module
 *
 * @date   2026-04-DD
 * @author _____
 *******************************************************************************************************************************/

/* Standard library Headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <cstring>

/* Inter-component Headers */
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

/* Intra-component Headers */

/*******************************************************************************************************************************
 * Private defines and enums
 *******************************************************************************************************************************/

/*******************************************************************************************************************************
 * Variables
 *******************************************************************************************************************************/

// Nordic UART Service (NUS) UUIDs.
static const char *kNusServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *kNusTxUuid      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify
static const char *kNusRxUuid      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write

static BLECharacteristic *g_txChar = nullptr;
static BLECharacteristic *g_rxChar = nullptr;
static BLEServer *g_server = nullptr;

// Staging + committed copy: onWrite only touches staging under a lock; loop() commits
// to g_navText so we never read/write the same buffer from BLE stack and main task.
static constexpr size_t kMaxNavText = 180;
static char g_navText[kMaxNavText + 1] = {0};
static char g_rxStaging[kMaxNavText + 1] = {0};
static volatile bool g_stagingDirty = false;
static volatile bool g_navUpdated = false;
static portMUX_TYPE g_navMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool g_bleConnected = false;
static volatile bool g_connChanged = false;
static volatile uint32_t g_rxSeq = 0;
static volatile size_t g_lastRxLen = 0;
static std::string g_lastPolledValue;
static uint32_t g_lastPollMs = 0;

/*******************************************************************************************************************************
 * Function declarations
 *******************************************************************************************************************************/

// Initialize BLE with a Nordic UART Service (NUS)-style GATT service.
// The phone writes navigation text to the RX characteristic.
void bleNavSetup();

// Returns true when a new navigation message has been received.
bool bleNavHasUpdate();

// Returns the latest received navigation text (null-terminated).
const char *bleNavGetText();

// Clears the update flag after the app has consumed the message.
void bleNavClearUpdate();

// Returns true when a phone is connected to the ESP32 BLE GATT server.
bool bleNavIsConnected();

// Returns true once when connection state changes, then auto-clears.
bool bleNavConnectionChanged();

// Monotonic counter incremented on every non-empty RX write.
uint32_t bleNavGetRxSequence();

// Length of last non-empty RX payload.
size_t bleNavGetLastRxLength();

// Prints the last received BLE RX payload in human-readable + hex form.
void bleNavDebugPrintLastRx();

/** @} */
