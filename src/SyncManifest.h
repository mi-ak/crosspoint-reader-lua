#pragma once
#include <string>

namespace SyncManifest {

// Generate a JSON manifest of all files under /cards/.
// Returns a JSON string:
// {"entries": [{"path":"...","kind":"card"|"folder_meta"|"asset","size":N,"etag":"..."},...]}</br>
// kind:
//   ".folder.json"  → "folder_meta"
//   ".json"         → "card"
//   other           → "asset"
// etag: "{size}-{mtime_epoch}" (PoC: uses file size as simple etag)
std::string generate();

}  // namespace SyncManifest
