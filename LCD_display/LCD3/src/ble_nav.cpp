#include "ble_nav.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Nordic UART Service (NUS) UUIDs.
static const char *kNusServiceUuid = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *kNusTxUuid      = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"; // Notify
static const char *kNusRxUuid      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"; // Write

static BLECharacteristic *g_txChar = nullptr;
static BLECharacteristic *g_rxChar = nullptr;
static BLEServer *g_server = nullptr;

// Keep the most recent navigation message in a fixed buffer to avoid heap churn.
static constexpr size_t kMaxNavText = 180;
static char g_navText[kMaxNavText + 1] = {0};
static volatile bool g_navUpdated = false;
static volatile bool g_bleConnected = false;
static volatile bool g_connChanged = false;

class NavServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    (void)server;
    g_bleConnected = true;
    g_connChanged = true;
    Serial.println("BLE client connected.");
  }

  void onDisconnect(BLEServer *server) override {
    g_bleConnected = false;
    g_connChanged = true;
    Serial.println("BLE client disconnected. Restarting advertising...");
    BLEAdvertising *adv = BLEDevice::getAdvertising();
    if (adv) {
      adv->start();
    }
    (void)server;
  }
};

class NavRxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    const std::string value = characteristic->getValue();
    const size_t n = value.size() > kMaxNavText ? kMaxNavText : value.size();
    if (n > 0) {
      memcpy(g_navText, value.data(), n);
    }
    g_navText[n] = '\0';
    g_navUpdated = true;

    Serial.printf("NAV RX (%u): %s\n", static_cast<unsigned>(n), g_navText);
  }
};

void bleNavSetup() {
  // Visible BLE name on the phone.
  BLEDevice::init("Electrium-NAV");

  g_server = BLEDevice::createServer();
  g_server->setCallbacks(new NavServerCallbacks());
  BLEService *service = g_server->createService(kNusServiceUuid);

  // TX = device -> phone notifications (unused for now).
  g_txChar = service->createCharacteristic(
    kNusTxUuid,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  g_txChar->addDescriptor(new BLE2902());

  // RX = phone -> device write channel for nav text.
  g_rxChar = service->createCharacteristic(
    kNusRxUuid,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  g_rxChar->setCallbacks(new NavRxCallbacks());

  service->start();

  // Start advertising the NUS service so the phone can find us.
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(kNusServiceUuid);
  adv->setScanResponse(true);
  adv->start();

  Serial.println("BLE NUS service started (Electrium-NAV).");
}

bool bleNavHasUpdate() {
  return g_navUpdated;
}

const char *bleNavGetText() {
  return g_navText;
}

void bleNavClearUpdate() {
  g_navUpdated = false;
}

bool bleNavIsConnected() {
  return g_bleConnected;
}

bool bleNavConnectionChanged() {
  if (!g_connChanged) {
    return false;
  }
  g_connChanged = false;
  return true;
}
