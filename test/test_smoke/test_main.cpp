#include <unity.h>
#include "logic/clamp.h"

void setUp() {}
void tearDown() {}

void test_clamp_inside() { TEST_ASSERT_EQUAL_FLOAT(5.0f, clampf(5.0f, 0.0f, 10.0f)); }
void test_clamp_below() { TEST_ASSERT_EQUAL_FLOAT(0.0f, clampf(-3.0f, 0.0f, 10.0f)); }
void test_clamp_above() { TEST_ASSERT_EQUAL_FLOAT(10.0f, clampf(42.0f, 0.0f, 10.0f)); }

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clamp_inside);
  RUN_TEST(test_clamp_below);
  RUN_TEST(test_clamp_above);
  return UNITY_END();
}
