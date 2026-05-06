#include "CardStore.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

#include <ctime>

#include "CardBridgePath.h"
#include "CardBridgeTokens.h"

CardStore CardStore::instance_;

CardStore& CardStore::getInstance() {
  return instance_;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

std::string isoTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buf[26];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &timeinfo);
  return std::string(buf);
}

// Recursively search dir for a file named `filename`.
// Returns full path on found, empty string otherwise.
std::string findFileInDir(const std::string& dir, const std::string& filename) {
  FsFile root = Storage.open(dir.c_str());
  LOG_DBG("CS", "findFileInDir: dir=%s open=%s isDir=%s", dir.c_str(), root ? "OK" : "FAIL", (root && root.isDirectory()) ? "YES" : "NO");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return "";
  }

  char name[256];
  FsFile entry = root.openNextFile();
  while (entry) {
    entry.getName(name, sizeof(name));
    std::string childName(name);
    std::string childPath = dir + "/" + childName;

    if (entry.isDirectory()) {
      entry.close();
      std::string found = findFileInDir(childPath, filename);
      if (!found.empty()) {
        root.close();
        return found;
      }
    } else {
      if (childName == filename) {
        entry.close();
        root.close();
        return childPath;
      }
      entry.close();
    }
    entry = root.openNextFile();
  }
  root.close();
  return "";
}

}  // namespace

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::string CardStore::generateId() {
  return "card_" + CardBridgeTokens::generate().substr(0, 8);
}

std::string CardStore::findCardPath(const std::string& id) {
  if (!CardBridgePath::validateId(id)) return "";
  std::string filename = id + ".json";

  // Search in /cards/boxes/
  std::string found = findFileInDir("/cards/boxes", filename);
  if (!found.empty()) return found;

  // Search in /cards/trash/
  std::string trashPath = "/cards/trash/" + filename;
  if (Storage.exists(trashPath.c_str())) return trashPath;

  return "";
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::string CardStore::createCard(const std::string& folderPath, const std::string& type,
                                  const std::string& dataJson) {
  if (!CardBridgePath::validate(folderPath)) {
    LOG_ERR("CS", "createCard: invalid folderPath: %s", folderPath.c_str());
    return "";
  }

  const std::string id = generateId();
  const std::string ts = isoTimestamp();

  // Parse the provided data JSON
  JsonDocument dataDoc;
  if (deserializeJson(dataDoc, dataJson) != DeserializationError::Ok) {
    LOG_ERR("CS", "createCard: invalid dataJson");
    return "";
  }

  // Build card JSON
  JsonDocument doc;
  doc["schema_version"] = "1.0";
  doc["id"] = id.c_str();
  doc["created_at"] = ts.c_str();
  doc["updated_at"] = ts.c_str();
  doc["type"] = type.c_str();
  doc["folder_path"] = folderPath.c_str();
  doc["data"].set(dataDoc.as<JsonVariant>());

  String content;
  serializeJson(doc, content);

  // Atomic write: write to .tmp then rename
  const std::string tmpPath = folderPath + "/" + id + ".json.tmp";
  const std::string finalPath = folderPath + "/" + id + ".json";

  if (!Storage.writeFile(tmpPath.c_str(), content)) {
    LOG_ERR("CS", "createCard: writeFile failed: %s", tmpPath.c_str());
    return "";
  }
  if (!Storage.rename(tmpPath.c_str(), finalPath.c_str())) {
    Storage.remove(tmpPath.c_str());
    LOG_ERR("CS", "createCard: rename failed: %s -> %s", tmpPath.c_str(), finalPath.c_str());
    return "";
  }

  LOG_DBG("CS", "Card created: %s in %s", id.c_str(), folderPath.c_str());
  return id;
}

std::string CardStore::getCard(const std::string& id) {
  if (!CardBridgePath::validateId(id)) return "";
  const std::string path = findCardPath(id);
  if (path.empty()) return "";
  String content = Storage.readFile(path.c_str());
  return content.c_str();
}

bool CardStore::updateCard(const std::string& id, const std::string& dataJson) {
  LOG_DBG("CS", "updateCard: id=%s", id.c_str());
  std::string path = findCardPath(id);
  LOG_DBG("CS", "updateCard: findCardPath result='%s'", path.c_str());

  // findCardPath が失敗した場合、デフォルトフォルダで直接試みる
  if (path.empty()) {
    const std::string directPath = "/cards/boxes/inbox/" + id + ".json";
    bool ex = Storage.exists(directPath.c_str());
    LOG_DBG("CS", "updateCard: direct exists(%s)=%d", directPath.c_str(), ex ? 1 : 0);
    if (ex) {
      path = directPath;
    }
  }

  if (path.empty()) {
    LOG_ERR("CS", "updateCard: card not found: %s", id.c_str());
    return false;
  }

  String existing = Storage.readFile(path.c_str());
  LOG_DBG("CS", "updateCard: readFile len=%d", (int)existing.length());
  if (existing.isEmpty()) {
    LOG_ERR("CS", "updateCard: readFile returned empty for path=%s", path.c_str());
    return false;
  }

  JsonDocument doc;
  if (deserializeJson(doc, existing) != DeserializationError::Ok) {
    LOG_ERR("CS", "updateCard: deserialize existing failed");
    return false;
  }

  JsonDocument dataDoc;
  if (deserializeJson(dataDoc, dataJson) != DeserializationError::Ok) {
    LOG_ERR("CS", "updateCard: deserialize dataJson failed");
    return false;
  }

  doc["data"].set(dataDoc.as<JsonVariant>());
  doc["updated_at"] = isoTimestamp().c_str();

  String newContent;
  serializeJson(doc, newContent);

  const std::string tmpPath = path + ".tmp";
  bool writeOk = Storage.writeFile(tmpPath.c_str(), newContent);
  LOG_DBG("CS", "updateCard: writeFile(%s)=%d", tmpPath.c_str(), writeOk ? 1 : 0);
  if (!writeOk) return false;
  // SdFat では既存ファイルが存在すると rename が失敗するため、先に削除する
  if (Storage.exists(path.c_str())) {
    if (!Storage.remove(path.c_str())) {
      LOG_ERR("CS", "updateCard: failed to remove existing file before rename: %s", path.c_str());
      Storage.remove(tmpPath.c_str());
      return false;
    }
  }
  bool renameOk = Storage.rename(tmpPath.c_str(), path.c_str());
  LOG_DBG("CS", "updateCard: rename=%d", renameOk ? 1 : 0);
  if (!renameOk) {
    Storage.remove(tmpPath.c_str());
    return false;
  }
  return true;
}

std::vector<std::string> CardStore::listCards(const std::string& folderPath) {
  if (!CardBridgePath::validate(folderPath)) return {};

  FsFile dir = Storage.open(folderPath.c_str());
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return {};
  }

  std::vector<std::string> ids;
  char name[256];
  FsFile entry = dir.openNextFile();
  while (entry) {
    entry.getName(name, sizeof(name));
    std::string fname(name);

    if (!entry.isDirectory()) {
      // Match card_*.json but not card_*.json.tmp
      if (fname.size() > 10 && fname.substr(0, 5) == "card_" &&
          fname.size() >= 5 && fname.substr(fname.size() - 5) == ".json" &&
          fname.find(".json.tmp") == std::string::npos) {
        // ID is filename without ".json"
        ids.push_back(fname.substr(0, fname.size() - 5));
      }
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return ids;
}

bool CardStore::moveCard(const std::string& id, const std::string& newFolderPath) {
  if (!CardBridgePath::validate(newFolderPath)) return false;
  if (!CardBridgePath::validateId(id)) return false;

  const std::string oldPath = findCardPath(id);
  if (oldPath.empty()) return false;

  String existing = Storage.readFile(oldPath.c_str());
  if (existing.isEmpty()) return false;

  JsonDocument doc;
  if (deserializeJson(doc, existing) != DeserializationError::Ok) return false;

  doc["folder_path"] = newFolderPath.c_str();
  doc["updated_at"] = isoTimestamp().c_str();

  String newContent;
  serializeJson(doc, newContent);

  const std::string newPath = newFolderPath + "/" + id + ".json";
  const std::string tmpPath = newPath + ".tmp";

  if (!Storage.writeFile(tmpPath.c_str(), newContent)) return false;
  if (!Storage.rename(tmpPath.c_str(), newPath.c_str())) {
    Storage.remove(tmpPath.c_str());
    return false;
  }

  // Remove original only if it differs
  if (oldPath != newPath) {
    Storage.remove(oldPath.c_str());
  }
  return true;
}

bool CardStore::trashCard(const std::string& id) {
  if (!CardBridgePath::validateId(id)) return false;

  const std::string oldPath = findCardPath(id);
  if (oldPath.empty()) return false;

  // Don't trash something already in trash
  if (oldPath.find("/cards/trash/") != std::string::npos) return false;

  String existing = Storage.readFile(oldPath.c_str());
  if (existing.isEmpty()) return false;

  JsonDocument doc;
  if (deserializeJson(doc, existing) != DeserializationError::Ok) return false;

  const std::string originalFolder = doc["folder_path"] | "/cards/boxes/inbox";

  JsonObject trash = doc["trash"].to<JsonObject>();
  trash["original_folder_path"] = originalFolder.c_str();
  trash["trashed_at"] = isoTimestamp().c_str();
  doc["folder_path"] = "/cards/trash";

  String newContent;
  serializeJson(doc, newContent);

  const std::string trashPath = "/cards/trash/" + id + ".json";
  const std::string tmpPath = trashPath + ".tmp";

  if (!Storage.writeFile(tmpPath.c_str(), newContent)) return false;
  if (!Storage.rename(tmpPath.c_str(), trashPath.c_str())) {
    Storage.remove(tmpPath.c_str());
    return false;
  }

  // Remove original
  Storage.remove(oldPath.c_str());
  return true;
}

bool CardStore::restoreCard(const std::string& id) {
  if (!CardBridgePath::validateId(id)) return false;

  const std::string trashPath = "/cards/trash/" + id + ".json";
  if (!Storage.exists(trashPath.c_str())) return false;

  String existing = Storage.readFile(trashPath.c_str());
  if (existing.isEmpty()) return false;

  JsonDocument doc;
  if (deserializeJson(doc, existing) != DeserializationError::Ok) return false;

  std::string restoreFolder = "/cards/boxes/inbox";
  if (doc["trash"]["original_folder_path"].is<const char*>()) {
    const char* orig = doc["trash"]["original_folder_path"].as<const char*>();
    if (orig && strlen(orig) > 0) {
      restoreFolder = orig;
    }
  }

  // Remove trash metadata and update folder_path
  doc.remove("trash");
  doc["folder_path"] = restoreFolder.c_str();
  doc["updated_at"] = isoTimestamp().c_str();

  String newContent;
  serializeJson(doc, newContent);

  const std::string newPath = restoreFolder + "/" + id + ".json";
  const std::string tmpPath = newPath + ".tmp";

  if (!Storage.writeFile(tmpPath.c_str(), newContent)) return false;
  if (!Storage.rename(tmpPath.c_str(), newPath.c_str())) {
    Storage.remove(tmpPath.c_str());
    return false;
  }

  // Remove from trash
  Storage.remove(trashPath.c_str());
  return true;
}
