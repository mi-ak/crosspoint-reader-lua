#include "FolderStore.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

#include <ctime>

#include "CardBridgePath.h"

FolderStore FolderStore::instance_;

FolderStore& FolderStore::getInstance() {
  return instance_;
}

namespace {

std::string folderIsoTimestamp() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buf[26];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S+00:00", &timeinfo);
  return std::string(buf);
}

// Extract the last path component (folder name) from a path like "/cards/boxes/inbox"
std::string baseName(const std::string& path) {
  const size_t pos = path.rfind('/');
  if (pos == std::string::npos) return path;
  return path.substr(pos + 1);
}

}  // namespace

bool FolderStore::createFolder(const std::string& path, const std::string& title) {
  if (!CardBridgePath::validate(path)) {
    LOG_ERR("FS", "createFolder: invalid path: %s", path.c_str());
    return false;
  }

  if (!Storage.mkdir(path.c_str())) {
    // mkdir returns false if directory already exists too; check
    if (!Storage.exists(path.c_str())) {
      LOG_ERR("FS", "createFolder: mkdir failed: %s", path.c_str());
      return false;
    }
  }

  const std::string ts = folderIsoTimestamp();
  const std::string name = baseName(path);

  JsonDocument doc;
  doc["schema_version"] = "1.0";
  doc["path"] = path.c_str();
  doc["name"] = name.c_str();
  doc["title"] = (!title.empty() ? title : name).c_str();
  doc["created_at"] = ts.c_str();
  doc["updated_at"] = ts.c_str();

  String content;
  serializeJson(doc, content);

  const std::string metaPath = path + "/.folder.json";
  if (!Storage.writeFile(metaPath.c_str(), content)) {
    LOG_ERR("FS", "createFolder: writeFile failed: %s", metaPath.c_str());
    return false;
  }

  LOG_DBG("FS", "Folder created: %s", path.c_str());
  return true;
}

std::vector<std::string> FolderStore::listFolders(const std::string& path) {
  if (!CardBridgePath::validate(path)) return {};

  FsFile dir = Storage.open(path.c_str());
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return {};
  }

  std::vector<std::string> folders;
  char name[256];
  FsFile entry = dir.openNextFile();
  while (entry) {
    entry.getName(name, sizeof(name));
    std::string entryName(name);
    if (entry.isDirectory() && !entryName.empty() && entryName[0] != '.') {
      folders.push_back(path + "/" + entryName);
    }
    entry.close();
    entry = dir.openNextFile();
  }
  dir.close();
  return folders;
}

std::string FolderStore::getFolder(const std::string& path) {
  if (!CardBridgePath::validate(path)) return "";
  const std::string metaPath = path + "/.folder.json";
  String content = Storage.readFile(metaPath.c_str());
  return content.c_str();
}
