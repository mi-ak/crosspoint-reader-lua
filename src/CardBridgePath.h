#pragma once
#include <string>

namespace CardBridgePath {

// Sanitize a card API path:
// - Returns false if path contains ".." (traversal attack)
// - Returns false if path does not start with "/cards/"
// - Normalizes trailing slashes
// cardRoot is always "/cards"
bool validate(const std::string& path);

// Validate that a card ID (filename) is safe (no "/" or "..")
bool validateId(const std::string& id);

// Build full filesystem path from a relative folder path like "/cards/boxes/inbox"
// Returns empty string if validation fails
std::string resolvePath(const std::string& path);

}  // namespace CardBridgePath
