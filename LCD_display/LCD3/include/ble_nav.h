#ifndef BLE_NAV_H
#define BLE_NAV_H

#include <stdbool.h>

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

#endif
