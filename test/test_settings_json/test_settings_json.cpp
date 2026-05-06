#include <unity.h>
#include <string>
#include "HalStorage.h"
#include "CrossPointSettings.h"
#include "JsonSettingsIO.h"
#include "fontIds.h"

void setUp() {
  Storage.files.clear();
  Storage.lastWrittenPath.clear();
  Storage.lastWrittenContent.clear();
}
void tearDown() {}

// ===== CrossPointSettings JSON round-trip =====

void test_settings_sleep_while_powered_on() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepWhilePowered = 1;
  JsonSettingsIO::saveSettings(s, "/test/settings.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(1, loaded.sleepWhilePowered);
}

void test_settings_sleep_while_powered_off() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepWhilePowered = 0;
  JsonSettingsIO::saveSettings(s, "/test/settings.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(0, loaded.sleepWhilePowered);
}

void test_settings_timezone_roundtrip() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.timeZone = "CST-8";
  JsonSettingsIO::saveSettings(s, "/test/settings.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("CST-8", loaded.timeZone.c_str());
}

void test_settings_malformed_json() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(s, "{bad json}");
  TEST_ASSERT_FALSE(ok);
}

void test_settings_font_family_roundtrip() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.fontSize = CrossPointSettings::LARGE;
  JsonSettingsIO::saveSettings(s, "/test/settings.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::NOTOSANS, loaded.fontFamily);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::LARGE, loaded.fontSize);
}

void test_settings_orientation_roundtrip() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.orientation = CrossPointSettings::LANDSCAPE_CCW;
  JsonSettingsIO::saveSettings(s, "/test/settings.json");
  const std::string json = Storage.lastWrittenContent;

  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  bool ok = JsonSettingsIO::loadSettings(loaded, json.c_str());
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::LANDSCAPE_CCW, loaded.orientation);
}

void test_settings_load_from_file_valid_json() {
  // Save settings first to create a valid JSON
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepWhilePowered = 1;
  s.timeZone = "UTC0";
  JsonSettingsIO::saveSettings(s, "/.crosspoint/settings.json");
  // The mock's writeFile already put it in Storage.files via saveSettings

  // Now test loadFromFile
  CrossPointSettings& loaded = CrossPointSettings::getInstance();
  loaded.sleepWhilePowered = 0;
  loaded.timeZone = "JST-9";

  bool ok = loaded.loadFromFile();
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_UINT8(1, loaded.sleepWhilePowered);
  TEST_ASSERT_EQUAL_STRING("UTC0", loaded.timeZone.c_str());
}

void test_settings_load_from_file_no_file_returns_false() {
  // Storage.files is empty
  CrossPointSettings& s = CrossPointSettings::getInstance();
  bool ok = s.loadFromFile();
  TEST_ASSERT_FALSE(ok);
}

void test_settings_save_to_file() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepWhilePowered = 1;
  bool ok = s.saveToFile();
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_STRING("/.crosspoint/settings.json", Storage.lastWrittenPath.c_str());
  TEST_ASSERT_TRUE(Storage.lastWrittenContent.find("sleepWhilePowered") != std::string::npos);
}

// ===== CrossPointSettings utility methods =====

void test_get_sleep_timeout_1min() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepTimeout = CrossPointSettings::SLEEP_1_MIN;
  TEST_ASSERT_EQUAL_UINT32(60000UL, s.getSleepTimeoutMs());
}

void test_get_sleep_timeout_5min() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepTimeout = CrossPointSettings::SLEEP_5_MIN;
  TEST_ASSERT_EQUAL_UINT32(300000UL, s.getSleepTimeoutMs());
}

void test_get_sleep_timeout_10min() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepTimeout = CrossPointSettings::SLEEP_10_MIN;
  TEST_ASSERT_EQUAL_UINT32(600000UL, s.getSleepTimeoutMs());
}

void test_get_sleep_timeout_15min() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepTimeout = CrossPointSettings::SLEEP_15_MIN;
  TEST_ASSERT_EQUAL_UINT32(900000UL, s.getSleepTimeoutMs());
}

void test_get_sleep_timeout_30min() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.sleepTimeout = CrossPointSettings::SLEEP_30_MIN;
  TEST_ASSERT_EQUAL_UINT32(1800000UL, s.getSleepTimeoutMs());
}

void test_get_refresh_frequency_1() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.refreshFrequency = CrossPointSettings::REFRESH_1;
  TEST_ASSERT_EQUAL_INT(1, s.getRefreshFrequency());
}

void test_get_refresh_frequency_5() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.refreshFrequency = CrossPointSettings::REFRESH_5;
  TEST_ASSERT_EQUAL_INT(5, s.getRefreshFrequency());
}

void test_get_refresh_frequency_10() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.refreshFrequency = CrossPointSettings::REFRESH_10;
  TEST_ASSERT_EQUAL_INT(10, s.getRefreshFrequency());
}

void test_get_refresh_frequency_15() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.refreshFrequency = CrossPointSettings::REFRESH_15;
  TEST_ASSERT_EQUAL_INT(15, s.getRefreshFrequency());
}

void test_get_refresh_frequency_30() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.refreshFrequency = CrossPointSettings::REFRESH_30;
  TEST_ASSERT_EQUAL_INT(30, s.getRefreshFrequency());
}

void test_get_reader_font_id_bookerly_small() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::BOOKERLY;
  s.fontSize = CrossPointSettings::SMALL;
  TEST_ASSERT_EQUAL_INT(BOOKERLY_12_FONT_ID, s.getReaderFontId());
}

void test_get_reader_font_id_bookerly_medium() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::BOOKERLY;
  s.fontSize = CrossPointSettings::MEDIUM;
  TEST_ASSERT_EQUAL_INT(BOOKERLY_14_FONT_ID, s.getReaderFontId());
}

void test_get_reader_font_id_bookerly_large() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::BOOKERLY;
  s.fontSize = CrossPointSettings::LARGE;
  TEST_ASSERT_EQUAL_INT(BOOKERLY_16_FONT_ID, s.getReaderFontId());
}

void test_get_reader_font_id_notosans_small() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.fontSize = CrossPointSettings::SMALL;
  TEST_ASSERT_EQUAL_INT(NOTOSANS_12_FONT_ID, s.getReaderFontId());
}

void test_get_reader_font_id_notosans_medium() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.fontSize = CrossPointSettings::MEDIUM;
  TEST_ASSERT_EQUAL_INT(NOTOSANS_14_FONT_ID, s.getReaderFontId());
}

void test_get_reader_font_id_notosans_large() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.fontSize = CrossPointSettings::LARGE;
  TEST_ASSERT_EQUAL_INT(NOTOSANS_16_FONT_ID, s.getReaderFontId());
}

void test_get_reader_line_compression_bookerly_normal() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::BOOKERLY;
  s.lineSpacing = CrossPointSettings::NORMAL;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.2f, s.getReaderLineCompression());
}

void test_get_reader_line_compression_bookerly_wide() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::BOOKERLY;
  s.lineSpacing = CrossPointSettings::WIDE;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.4f, s.getReaderLineCompression());
}

void test_get_reader_line_compression_notosans_normal() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.lineSpacing = CrossPointSettings::NORMAL;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.2f, s.getReaderLineCompression());
}

void test_get_reader_line_compression_notosans_wide() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.fontFamily = CrossPointSettings::NOTOSANS;
  s.lineSpacing = CrossPointSettings::WIDE;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.4f, s.getReaderLineCompression());
}

void test_validate_front_button_no_duplicates() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.frontButtonBack = CrossPointSettings::FRONT_HW_BACK;
  s.frontButtonConfirm = CrossPointSettings::FRONT_HW_CONFIRM;
  s.frontButtonLeft = CrossPointSettings::FRONT_HW_LEFT;
  s.frontButtonRight = CrossPointSettings::FRONT_HW_RIGHT;

  CrossPointSettings::validateFrontButtonMapping(s);

  // Values should be unchanged
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_BACK, s.frontButtonBack);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_CONFIRM, s.frontButtonConfirm);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_LEFT, s.frontButtonLeft);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_RIGHT, s.frontButtonRight);
}

void test_validate_front_button_with_duplicates_resets_to_default() {
  CrossPointSettings& s = CrossPointSettings::getInstance();
  s.frontButtonBack = CrossPointSettings::FRONT_HW_BACK;
  s.frontButtonConfirm = CrossPointSettings::FRONT_HW_BACK;  // duplicate!
  s.frontButtonLeft = CrossPointSettings::FRONT_HW_LEFT;
  s.frontButtonRight = CrossPointSettings::FRONT_HW_RIGHT;

  CrossPointSettings::validateFrontButtonMapping(s);

  // Should reset to defaults
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_BACK, s.frontButtonBack);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_CONFIRM, s.frontButtonConfirm);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_LEFT, s.frontButtonLeft);
  TEST_ASSERT_EQUAL_UINT8(CrossPointSettings::FRONT_HW_RIGHT, s.frontButtonRight);
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  // JSON serialization tests
  RUN_TEST(test_settings_sleep_while_powered_on);
  RUN_TEST(test_settings_sleep_while_powered_off);
  RUN_TEST(test_settings_timezone_roundtrip);
  RUN_TEST(test_settings_malformed_json);
  RUN_TEST(test_settings_font_family_roundtrip);
  RUN_TEST(test_settings_orientation_roundtrip);
  // loadFromFile / saveToFile tests
  RUN_TEST(test_settings_load_from_file_valid_json);
  RUN_TEST(test_settings_load_from_file_no_file_returns_false);
  RUN_TEST(test_settings_save_to_file);
  // Utility method tests
  RUN_TEST(test_get_sleep_timeout_1min);
  RUN_TEST(test_get_sleep_timeout_5min);
  RUN_TEST(test_get_sleep_timeout_10min);
  RUN_TEST(test_get_sleep_timeout_15min);
  RUN_TEST(test_get_sleep_timeout_30min);
  RUN_TEST(test_get_refresh_frequency_1);
  RUN_TEST(test_get_refresh_frequency_5);
  RUN_TEST(test_get_refresh_frequency_10);
  RUN_TEST(test_get_refresh_frequency_15);
  RUN_TEST(test_get_refresh_frequency_30);
  RUN_TEST(test_get_reader_font_id_bookerly_small);
  RUN_TEST(test_get_reader_font_id_bookerly_medium);
  RUN_TEST(test_get_reader_font_id_bookerly_large);
  RUN_TEST(test_get_reader_font_id_notosans_small);
  RUN_TEST(test_get_reader_font_id_notosans_medium);
  RUN_TEST(test_get_reader_font_id_notosans_large);
  RUN_TEST(test_get_reader_line_compression_bookerly_normal);
  RUN_TEST(test_get_reader_line_compression_bookerly_wide);
  RUN_TEST(test_get_reader_line_compression_notosans_normal);
  RUN_TEST(test_get_reader_line_compression_notosans_wide);
  RUN_TEST(test_validate_front_button_no_duplicates);
  RUN_TEST(test_validate_front_button_with_duplicates_resets_to_default);
  return UNITY_END();
}
