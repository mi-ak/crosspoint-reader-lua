#include "CardBridgeTokens.h"

#include <esp_random.h>

#include <cstdint>

namespace CardBridgeTokens {

std::string generate() {
  // Collect 16 bytes using esp_random() (4 bytes per call)
  uint8_t bytes[16];
  for (int i = 0; i < 16; i += 4) {
    const uint32_t r = esp_random();
    bytes[i + 0] = static_cast<uint8_t>((r >> 24) & 0xFF);
    bytes[i + 1] = static_cast<uint8_t>((r >> 16) & 0xFF);
    bytes[i + 2] = static_cast<uint8_t>((r >> 8) & 0xFF);
    bytes[i + 3] = static_cast<uint8_t>((r >> 0) & 0xFF);
  }

  // Base64url encode (RFC 4648 §5): '+' -> '-', '/' -> '_', no padding
  static const char kTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

  std::string out;
  out.reserve(22);  // ceil(16 * 4/3) without padding = 22 chars

  for (int i = 0; i < 16; i += 3) {
    const uint32_t b0 = bytes[i];
    const uint32_t b1 = (i + 1 < 16) ? bytes[i + 1] : 0;
    const uint32_t b2 = (i + 2 < 16) ? bytes[i + 2] : 0;
    out += kTable[(b0 >> 2) & 0x3F];
    out += kTable[((b0 << 4) | (b1 >> 4)) & 0x3F];
    if (i + 1 < 16) out += kTable[((b1 << 2) | (b2 >> 6)) & 0x3F];
    if (i + 2 < 16) out += kTable[b2 & 0x3F];
  }

  return out;
}

}  // namespace CardBridgeTokens
