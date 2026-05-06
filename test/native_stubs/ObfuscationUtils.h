#pragma once
#include <string>
namespace obfuscation {
  inline std::string obfuscateToBase64(const std::string& s) { return s; }
  inline std::string deobfuscateFromBase64(const std::string& s, bool* ok = nullptr) {
    if (ok) *ok = true;
    return s;
  }
}
