#include <unity.h>
#include "CardBridgePath.h"

void setUp() {}
void tearDown() {}

void test_path_validate_valid() {
  TEST_ASSERT_TRUE(CardBridgePath::validate("/cards/boxes/inbox"));
}

void test_path_validate_valid_assets() {
  TEST_ASSERT_TRUE(CardBridgePath::validate("/cards/assets"));
}

void test_path_validate_traversal_attack() {
  TEST_ASSERT_FALSE(CardBridgePath::validate("/cards/../etc/passwd"));
}

void test_path_validate_outside_cards() {
  TEST_ASSERT_FALSE(CardBridgePath::validate("/books/something"));
}

void test_path_validate_empty() {
  TEST_ASSERT_FALSE(CardBridgePath::validate(""));
}

void test_id_validate_valid() {
  TEST_ASSERT_TRUE(CardBridgePath::validateId("card_abc12345"));
}

void test_id_validate_slash() {
  TEST_ASSERT_FALSE(CardBridgePath::validateId("abc/evil"));
}

void test_id_validate_traversal() {
  TEST_ASSERT_FALSE(CardBridgePath::validateId("../evil"));
}

void test_id_validate_empty() {
  TEST_ASSERT_FALSE(CardBridgePath::validateId(""));
}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_path_validate_valid);
  RUN_TEST(test_path_validate_valid_assets);
  RUN_TEST(test_path_validate_traversal_attack);
  RUN_TEST(test_path_validate_outside_cards);
  RUN_TEST(test_path_validate_empty);
  RUN_TEST(test_id_validate_valid);
  RUN_TEST(test_id_validate_slash);
  RUN_TEST(test_id_validate_traversal);
  RUN_TEST(test_id_validate_empty);
  UNITY_END();
  return 0;
}
