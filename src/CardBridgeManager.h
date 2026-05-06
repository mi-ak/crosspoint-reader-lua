#pragma once
#include <string>

class CardBridgeManager {
 public:
  static CardBridgeManager& getInstance();

  // Initialize: create /cards/ directory structure
  void begin();

  // Is Card Bridge mode available (WiFi connected)?
  bool isAvailable() const;

 private:
  CardBridgeManager() = default;
  static CardBridgeManager instance_;
};

#define CARD_BRIDGE CardBridgeManager::getInstance()
