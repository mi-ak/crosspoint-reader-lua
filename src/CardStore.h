#pragma once
#include <string>
#include <vector>

// Card CRUD using JSON files on SD card.
// Each card is stored as /cards/boxes/.../card_{id}.json
// Trash: /cards/trash/card_{id}.json (with trash metadata)
class CardStore {
 public:
  static CardStore& getInstance();

  // Create a new card in the given folder.
  // Returns the new card ID, or empty on failure.
  std::string createCard(const std::string& folderPath, const std::string& type, const std::string& dataJson);

  // Get a card by ID. Searches all subdirectories under /cards/boxes/ and /cards/trash/.
  // Returns JSON string, or empty on not found.
  std::string getCard(const std::string& id);

  // Update an existing card's data field.
  // Returns true on success.
  bool updateCard(const std::string& id, const std::string& dataJson);

  // List card IDs in a folder (direct children only, not recursive).
  std::vector<std::string> listCards(const std::string& folderPath);

  // Move card to a new folder.
  bool moveCard(const std::string& id, const std::string& newFolderPath);

  // Move card to trash (soft delete).
  // Saves original_folder_path in card JSON.
  bool trashCard(const std::string& id);

  // Restore card from trash to original_folder_path (or inbox if unknown).
  bool restoreCard(const std::string& id);

 private:
  CardStore() = default;
  static CardStore instance_;

  // Locate a card file path by ID (searches /cards/boxes/ and /cards/trash/)
  std::string findCardPath(const std::string& id);

  // Generate a unique card ID (card_{random8chars})
  std::string generateId();
};

#define CARD_STORE CardStore::getInstance()
