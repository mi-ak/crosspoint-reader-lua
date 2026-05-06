#include "CardBridgePath.h"

namespace CardBridgePath {

bool validate(const std::string& path) {
  if (path.find("..") != std::string::npos) return false;
  if (path.rfind("/cards", 0) != 0) return false;
  return true;
}

bool validateId(const std::string& id) {
  if (id.empty()) return false;
  if (id.find('/') != std::string::npos) return false;
  if (id.find("..") != std::string::npos) return false;
  return true;
}

std::string resolvePath(const std::string& path) {
  if (!validate(path)) return "";
  return path;
}

}  // namespace CardBridgePath
