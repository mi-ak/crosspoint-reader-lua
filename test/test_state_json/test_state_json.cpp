#include <unity.h>
#include <string>
#include "HalStorage.h"
#include "CrossPointState.h"
#include "CrossPointSettings.h"
#include "JsonSettingsIO.h"

// Reset Storage mock before each test
void setUp() {
  Storage.files.clear();
  Storage.lastWrittenPath.clear();
  Storage.lastWrittenContent.clear();
}

void tearDown() {}

// ===== CrossPointState JSON round-trip =====

void test_state_plugin_resume_roundtrip() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = true;
  s.lastPluginName = "myplugin";
  s.openEpubPath = "";
  s.lastSleepImage = 0;
  s.readerActivityLoadCount = 0;
  s.lastSleepFromReader = false;

  JsonSettingsIO::saveState(s, "/test/state.json");

  CrossPointState& loaded = CrossPointState::getInstance();
  const std::string& json = Storage.lastWrittenContent;
  loaded.lastSleepFromPlugin = false;
  loaded.lastPluginName = "";

  bool ok = JsonSettingsIO::loadState(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_TRUE(loaded.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("myplugin", loaded.lastPluginName.c_str());
}

void test_state_default_values_roundtrip() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = false;
  s.lastPluginName = "";
  s.openEpubPath = "";
  s.lastSleepImage = 2;
  s.readerActivityLoadCount = 3;
  s.lastSleepFromReader = true;

  JsonSettingsIO::saveState(s, "/test/state.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointState& loaded = CrossPointState::getInstance();
  bool ok = JsonSettingsIO::loadState(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_FALSE(loaded.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("", loaded.lastPluginName.c_str());
  TEST_ASSERT_EQUAL_UINT8(2, loaded.lastSleepImage);
  TEST_ASSERT_EQUAL_UINT8(3, loaded.readerActivityLoadCount);
  TEST_ASSERT_TRUE(loaded.lastSleepFromReader);
}

void test_state_malformed_json() {
  CrossPointState& s = CrossPointState::getInstance();
  bool ok = JsonSettingsIO::loadState(s, "not valid json{{{{");
  TEST_ASSERT_FALSE(ok);
}

void test_state_missing_fields_use_defaults() {
  const char* oldJson = R"({"openEpubPath":"","lastSleepImage":0,"readerActivityLoadCount":0,"lastSleepFromReader":false})";
  CrossPointState& s = CrossPointState::getInstance();
  bool ok = JsonSettingsIO::loadState(s, oldJson);
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_FALSE(s.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("", s.lastPluginName.c_str());
}

void test_state_openEpubPath_roundtrip() {
  CrossPointState& s = CrossPointState::getInstance();
  s.openEpubPath = "/books/mybook.epub";
  s.lastSleepFromPlugin = false;
  s.lastPluginName = "";

  JsonSettingsIO::saveState(s, "/test/state.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointState& loaded = CrossPointState::getInstance();
  loaded.openEpubPath = "";
  bool ok = JsonSettingsIO::loadState(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("/books/mybook.epub", loaded.openEpubPath.c_str());
}

// ===== CrossPointState::clearPluginResumeState =====

void test_clear_plugin_resume_state_both_set() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = true;
  s.lastPluginName = "plugin1";

  CrossPointState::clearPluginResumeState();

  TEST_ASSERT_FALSE(s.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("", s.lastPluginName.c_str());
  // saveToFile should have been called (lastWrittenPath set)
  TEST_ASSERT_EQUAL_STRING("/.crosspoint/state.json", Storage.lastWrittenPath.c_str());
}

void test_clear_plugin_resume_state_only_flag() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = true;
  s.lastPluginName = "";

  CrossPointState::clearPluginResumeState();

  TEST_ASSERT_FALSE(s.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("", s.lastPluginName.c_str());
  TEST_ASSERT_EQUAL_STRING("/.crosspoint/state.json", Storage.lastWrittenPath.c_str());
}

void test_clear_plugin_resume_state_only_name() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = false;
  s.lastPluginName = "leftover";

  CrossPointState::clearPluginResumeState();

  TEST_ASSERT_FALSE(s.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("", s.lastPluginName.c_str());
  TEST_ASSERT_EQUAL_STRING("/.crosspoint/state.json", Storage.lastWrittenPath.c_str());
}

void test_clear_plugin_resume_state_nothing_to_clear() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = false;
  s.lastPluginName = "";

  Storage.lastWrittenPath.clear();  // reset write tracker
  CrossPointState::clearPluginResumeState();

  // No save should have happened
  TEST_ASSERT_EQUAL_STRING("", Storage.lastWrittenPath.c_str());
}

// ===== CrossPointState loadFromFile =====

void test_state_load_from_file_valid_json() {
  const std::string json = R"({"openEpubPath":"/books/test.epub","lastSleepImage":1,"readerActivityLoadCount":0,"lastSleepFromReader":false,"lastSleepFromPlugin":true,"lastPluginName":"myplugin"})";
  Storage.files["/.crosspoint/state.json"] = json;

  CrossPointState& s = CrossPointState::getInstance();
  s.openEpubPath = "";
  s.lastSleepFromPlugin = false;
  s.lastPluginName = "";

  bool ok = s.loadFromFile();
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("/books/test.epub", s.openEpubPath.c_str());
  TEST_ASSERT_TRUE(s.lastSleepFromPlugin);
  TEST_ASSERT_EQUAL_STRING("myplugin", s.lastPluginName.c_str());
}

void test_state_load_from_file_no_file_returns_false() {
  // Storage.files is empty (from setUp)
  CrossPointState& s = CrossPointState::getInstance();
  bool ok = s.loadFromFile();
  TEST_ASSERT_FALSE(ok);
}

void test_state_save_to_file() {
  CrossPointState& s = CrossPointState::getInstance();
  s.lastSleepFromPlugin = true;
  s.lastPluginName = "test_plugin";

  bool ok = s.saveToFile();
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("/.crosspoint/state.json", Storage.lastWrittenPath.c_str());
  // Content should contain our values
  TEST_ASSERT_TRUE(Storage.lastWrittenContent.find("test_plugin") != std::string::npos);
  TEST_ASSERT_TRUE(Storage.lastWrittenContent.find("lastSleepFromPlugin") != std::string::npos);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  // JSON round-trip tests
  RUN_TEST(test_state_plugin_resume_roundtrip);
  RUN_TEST(test_state_default_values_roundtrip);
  RUN_TEST(test_state_malformed_json);
  RUN_TEST(test_state_missing_fields_use_defaults);
  RUN_TEST(test_state_openEpubPath_roundtrip);
  // clearPluginResumeState branch tests
  RUN_TEST(test_clear_plugin_resume_state_both_set);
  RUN_TEST(test_clear_plugin_resume_state_only_flag);
  RUN_TEST(test_clear_plugin_resume_state_only_name);
  RUN_TEST(test_clear_plugin_resume_state_nothing_to_clear);
  // loadFromFile / saveToFile tests
  RUN_TEST(test_state_load_from_file_valid_json);
  RUN_TEST(test_state_load_from_file_no_file_returns_false);
  RUN_TEST(test_state_save_to_file);
  return UNITY_END();
}
