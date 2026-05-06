#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <map>

// ---- FsFile stub ----
class FsFile {
 public:
  void close() {}
  explicit operator bool() const { return false; }
  size_t print(const char* s) { return s ? strlen(s) : 0; }
  size_t write(const uint8_t* s, size_t n) { return n; }
  size_t write(uint8_t c) { return 1; }
};

// ---- String stub ----
// Must be compatible with ArduinoJson serialization target AND provide isEmpty()
class String {
 public:
  String() = default;
  String(const char* s) : _s(s ? s : "") {}
  explicit String(const std::string& s) : _s(s) {}
  bool isEmpty() const { return _s.empty(); }
  const char* c_str() const { return _s.c_str(); }
  size_t length() const { return _s.length(); }
  bool operator==(const String& o) const { return _s == o._s; }
  bool operator==(const char* o) const { return _s == o; }
  // ArduinoJson serialization target interface
  size_t write(const uint8_t* s, size_t n) {
    _s.append(reinterpret_cast<const char*>(s), n);
    return n;
  }
  size_t write(uint8_t c) {
    _s += static_cast<char>(c);
    return 1;
  }

 private:
  std::string _s;
};

// ---- HalStorage mock ----
class HalStorage {
 public:
  // Inspection
  std::string lastWrittenPath;
  std::string lastWrittenContent;
  // Virtual filesystem for tests
  std::map<std::string, std::string> files;

  bool mkdir(const char* path) { return true; }

  bool exists(const char* path) {
    return files.count(std::string(path)) > 0;
  }

  String readFile(const char* path) {
    auto it = files.find(std::string(path));
    if (it != files.end()) return String(it->second.c_str());
    return String();
  }

  bool writeFile(const char* path, const char* content) {
    lastWrittenPath = path;
    lastWrittenContent = content;
    files[std::string(path)] = content;
    return true;
  }
  bool writeFile(const char* path, const std::string& content) {
    return writeFile(path, content.c_str());
  }
  bool writeFile(const char* path, const String& content) {
    return writeFile(path, content.c_str());
  }

  bool rename(const char* from, const char* to) {
    auto it = files.find(std::string(from));
    if (it != files.end()) {
      files[std::string(to)] = it->second;
      files.erase(it);
    }
    return true;
  }

  bool openFileForRead(const char*, const char*, FsFile&) { return false; }
  bool openFileForRead(const char*, const std::string&, FsFile&) { return false; }
  bool openFileForRead(const char*, const String&, FsFile&) { return false; }
  bool openFileForWrite(const char*, const char*, FsFile&) { return false; }
  bool openFileForWrite(const char*, const std::string&, FsFile&) { return false; }
  bool openFileForWrite(const char*, const String&, FsFile&) { return false; }
};

// C++17 inline variable - single definition rule satisfied across all translation units
inline HalStorage Storage;
