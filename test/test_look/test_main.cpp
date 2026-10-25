#include <unity.h>
#include "logic/look.h"
void setUp() {}
void tearDown() {}

void test_center_maps_to_zero() {
  LookState l{};
  look_at(l, 160, 100, 1000);
  float gx = 99, gy = 99;
  look_target(l, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, gy);
}
void test_corners_clamped() {
  LookState l{};
  float gx, gy;
  look_at(l, 0, 0, 0);
  look_target(l, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -30.0f, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -15.0f, gy);
  look_at(l, 319, 239, 0);   // y 239 wäre +20.85 -> geklemmt
  look_target(l, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 30.0f, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, gy);
  look_at(l, 240, 150, 0);
  look_target(l, &gx, &gy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15.0f, gx);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.5f, gy);
}
void test_expires_after_3s() {
  LookState l{};
  TEST_ASSERT_FALSE(look_active(l, 0));
  look_at(l, 10, 10, 5000);
  TEST_ASSERT_TRUE(look_active(l, 5000));
  TEST_ASSERT_TRUE(look_active(l, 7999));
  TEST_ASSERT_FALSE(look_active(l, 8000));
  look_at(l, 10, 10, 7000);   // erneut: verlängert
  TEST_ASSERT_TRUE(look_active(l, 9999));
}
void test_highfive_corners() {
  TEST_ASSERT_TRUE(highfive_hit(10, 10));
  TEST_ASSERT_TRUE(highfive_hit(63, 55));
  TEST_ASSERT_TRUE(highfive_hit(300, 20));
  TEST_ASSERT_TRUE(highfive_hit(257, 0));
  TEST_ASSERT_FALSE(highfive_hit(64, 20));    // Mitte oben
  TEST_ASSERT_FALSE(highfive_hit(256, 20));
  TEST_ASSERT_FALSE(highfive_hit(160, 10));
  TEST_ASSERT_FALSE(highfive_hit(10, 56));    // unten links
  TEST_ASSERT_FALSE(highfive_hit(300, 200));  // unten rechts
  TEST_ASSERT_FALSE(highfive_hit(160, 100));  // Mitte
}
int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_center_maps_to_zero);
  RUN_TEST(test_corners_clamped);
  RUN_TEST(test_expires_after_3s);
  RUN_TEST(test_highfive_corners);
  return UNITY_END();
}
