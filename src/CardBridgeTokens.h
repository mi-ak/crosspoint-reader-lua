#pragma once
#include <string>

namespace CardBridgeTokens {

// Generate a random 16-byte token encoded as base64url (no padding)
// Uses esp_random() for hardware random source
std::string generate();

}  // namespace CardBridgeTokens
