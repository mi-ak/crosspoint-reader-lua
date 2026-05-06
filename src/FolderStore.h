#pragma once
#include <string>
#include <vector>

// Folder management using .folder.json files on SD card.
class FolderStore {
 public:
  static FolderStore& getInstance();

  // Create a folder with optional title/description.
  // Returns true on success.
  bool createFolder(const std::string& path, const std::string& title = "");

  // List direct child folders of a path.
  std::vector<std::string> listFolders(const std::string& path);

  // Get folder metadata JSON.
  std::string getFolder(const std::string& path);

 private:
  FolderStore() = default;
  static FolderStore instance_;
};

#define FOLDER_STORE FolderStore::getInstance()
