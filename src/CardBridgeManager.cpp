#include "CardBridgeManager.h"

#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>

CardBridgeManager CardBridgeManager::instance_;

CardBridgeManager& CardBridgeManager::getInstance() {
  return instance_;
}

static void mkdirIfAbsent(const char* path) {
  if (!Storage.exists(path)) {
    if (Storage.mkdir(path)) {
      LOG_DBG("CBM", "Created directory: %s", path);
    } else {
      LOG_ERR("CBM", "Failed to create directory: %s", path);
    }
  }
}

void CardBridgeManager::begin() {
  mkdirIfAbsent("/cards");
  mkdirIfAbsent("/cards/boxes");
  mkdirIfAbsent("/cards/boxes/inbox");
  mkdirIfAbsent("/cards/boxes/archive");
  mkdirIfAbsent("/cards/images");
  mkdirIfAbsent("/cards/assets");
  mkdirIfAbsent("/cards/trash");
  mkdirIfAbsent("/cards/.x4cb");
  LOG_DBG("CBM", "Card Bridge directory structure ready");
}

bool CardBridgeManager::isAvailable() const {
  return WiFi.status() == WL_CONNECTED;
}
