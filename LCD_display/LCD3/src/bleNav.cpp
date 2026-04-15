#include "ble_nav.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ctype.h>
#include <cstring>

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

static void logRxPayload(const char *source, const uint8_t *data, size_t len) {
  Serial.printf("NAV RX [%s] len=%u", source, static_cast<unsigned>(len));
  if (len == 0 || data == nullptr) {
    Serial.println(" text=\"\" hex=(empty)");
    return;
  }

  Serial.print(" text=\"");
  for (size_t i = 0; i < len; ++i) {
    const char c = static_cast<char>(data[i]);
    Serial.print(isprint(static_cast<unsigned char>(c)) ? c : '.');
  }
  Serial.print("\" hex=");
  for (size_t i = 0; i < len; ++i) {
    Serial.printf("%02X", static_cast<unsigned>(data[i]));
    if (i + 1 < len) {
      Serial.print(' ');
    }
  }
  Serial.println();
}

static void commitStagingToNavIfDirty() {
  portENTER_CRITICAL(&g_navMux);
  if (g_stagingDirty) {
    memcpy(g_navText, g_rxStaging, kMaxNavText + 1);
    g_stagingDirty = false;
    g_navUpdated = true;
  }
  portEXIT_CRITICAL(&g_navMux);
}

// Fallback for cases where BLE callback is not fired reliably on some stack/board
// combinations: poll the RX characteristic value and treat changes as new packets.
static void pollRxCharacteristicFallback() {
  if (!g_bleConnected || g_rxChar == nullptr) {
    return;
  }
  const uint32_t now = millis();
  if (now - g_lastPollMs < 50) {
    return;
  }
  g_lastPollMs = now;

  const std::string polled = g_rxChar->getValue();
  if (polled.empty() || polled == g_lastPolledValue) {
    return;
  }
  g_lastPolledValue = polled;

  const size_t n = polled.size() > kMaxNavText ? kMaxNavText : polled.size();
  portENTER_CRITICAL(&g_navMux);
  if (n > 0) {
    memcpy(g_rxStaging, polled.data(), n);
    g_rxStaging[n] = '\0';
    g_stagingDirty = true;
    g_navUpdated = true;
    ++g_rxSeq;
    g_lastRxLen = n;
  }
  portEXIT_CRITICAL(&g_navMux);
  logRxPayload("poll", reinterpret_cast<const uint8_t *>(polled.data()), n);
}

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
    portENTER_CRITICAL(&g_navMux);
    if (n > 0) {
      memcpy(g_rxStaging, value.data(), n);
    }
    g_rxStaging[n] = '\0';
    g_stagingDirty = true;
    if (n > 0) {
      g_navUpdated = true;
      ++g_rxSeq;
      g_lastRxLen = n;
    }
    portEXIT_CRITICAL(&g_navMux);
    logRxPayload("onWrite", reinterpret_cast<const uint8_t *>(value.data()), n);
  }
};

void bleNavSetup() {
  // Visible BLE name on the phone.
  BLEDevice::init("Electrium-NAV");
  // Allow larger payloads so long nav strings are not dropped (client must request MTU).
  BLEDevice::setMTU(185);

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
  pollRxCharacteristicFallback();
  commitStagingToNavIfDirty();
  return g_navUpdated;
}

const char *bleNavGetText() {
  pollRxCharacteristicFallback();
  commitStagingToNavIfDirty();
  return g_navText;
}

void bleNavClearUpdate() {
  portENTER_CRITICAL(&g_navMux);
  g_navUpdated = false;
  portEXIT_CRITICAL(&g_navMux);
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

uint32_t bleNavGetRxSequence() {
  return g_rxSeq;
}

size_t bleNavGetLastRxLength() {
  return g_lastRxLen;
}

void bleNavDebugPrintLastRx() {
  const uint32_t seq = g_rxSeq;
  const size_t len = g_lastRxLen;
  const char *text = bleNavGetText();
  Serial.printf("NAV DEBUG seq=%lu len=%u text=\"%s\"\n",
                static_cast<unsigned long>(seq),
                static_cast<unsigned>(len),
                (text != nullptr) ? text : "");
}
