# Electrium Nav Bridge

Automatic mode is now enabled.

## Behavior

1. Google Maps posts a navigation notification.
2. Notification listener extracts nav text.
3. Text is sanitized and deduplicated.
4. App sends the text over BLE to Electrium-NAV (NUS RX UUID).
5. If BLE is not ready, text is queued and sent after connection is established.

## Requirements

1. Notification access enabled for Electrium Nav Bridge.
2. Bluetooth enabled on phone.
3. Android 12+ BLE runtime permissions granted (SCAN and CONNECT).
4. ESP32 advertising as Electrium-NAV with NUS service.

## Setup and Verify

1. Open this project in Android Studio and run it once.
2. Tap Open Notification Access Settings and enable Electrium Nav Bridge.
3. Grant BLE permissions when prompted.
4. Start Google Maps turn-by-turn navigation.
5. Confirm app shows updated Nav Text and BLE write status.

## Notes

1. The app deduplicates unchanged text so repeated identical notifications are not resent.
2. Manual debug send buttons were intentionally removed to run fully automatic.
