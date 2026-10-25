#include <unity.h>
#include "logic/eye_model.h"
void setUp() {}
void tearDown() {}

void test_table_complete() {
  for (int i = 0; i < (int)Emotion::COUNT; i++) {
    Expression x = eye_expression((Emotion)i);
    TEST_ASSERT_TRUE(x.shape.w > 0 && x.shape.h > 0);
    TEST_ASSERT_TRUE(x.shape.open >= 0 && x.shape.open <= 1);
  }
  TEST_ASSERT_EQUAL_UINT8(255, eye_expression(Emotion::ANNOYED).color.r);
  TEST_ASSERT_TRUE(eye_expression(Emotion::ANNOYED).shape.lid_top_tilt < 0);
  TEST_ASSERT_TRUE(eye_expression(Emotion::SAD).shape.lid_top_tilt > 0);
}
void test_tween_converges_and_dt0_noop() {
  EyeShape a = eye_expression(Emotion::CONTENT).shape, b = eye_expression(Emotion::SURPRISED).shape;
  EyeShape c = eye_tween(a, b, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, a.w, c.w);
  for (int i = 0; i < 75; i++) c = eye_tween(c, b, 20);   // 1,5 s
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.w, c.w);
  TEST_ASSERT_FLOAT_WITHIN(0.15f, b.h, c.h);
}
void test_rng_deterministic() {
  uint32_t s1 = 42, s2 = 42;
  for (int i = 0; i < 10; i++) TEST_ASSERT_EQUAL_UINT32(rng_next(s1), rng_next(s2));
  uint32_t s = 7;
  for (int i = 0; i < 1000; i++) { uint32_t v = rng_range(s, 3, 9); TEST_ASSERT_TRUE(v >= 3 && v <= 9); }
}
void test_rng_range_full_span_no_div0() {
  uint32_t s = 7;
  (void)rng_range(s, 0, 0xFFFFFFFFu);   // hi-lo+1 == 0 darf nicht durch 0 teilen
  TEST_ASSERT_EQUAL_UINT32(5, rng_range(s, 5, 5));
  TEST_ASSERT_EQUAL_UINT32(9, rng_range(s, 9, 3));
}
void test_blinker_closes_within_window() {
  Blinker b; blinker_init(b, 1234, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, blinker_open(b, 0));
  float minv = 1; uint32_t t_closed = 0;
  for (uint32_t t = 0; t <= 6300; t += 10) { float o = blinker_open(b, t); if (o < minv) { minv = o; t_closed = t; } }
  TEST_ASSERT_TRUE(minv < 0.1f);
  TEST_ASSERT_TRUE(t_closed >= 2000 && t_closed <= 6200);
}
void test_blinker_reopens() {
  Blinker b; blinker_init(b, 99, 0);
  uint32_t t = 0; while (blinker_open(b, t) > 0.5f && t < 7000) t += 5;
  TEST_ASSERT_TRUE(t < 7000);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, blinker_open(b, t + 250));
}
void test_gazer_ranges() {
  Gazer z; gazer_init(z, 5, 0);
  for (uint32_t t = 0; t < 600000; t += 100) {
    gazer_update(z, t);
    TEST_ASSERT_TRUE(z.gx >= -30 && z.gx <= 30);
    TEST_ASSERT_TRUE(z.gy >= -15 && z.gy <= 15);
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_table_complete);
  RUN_TEST(test_tween_converges_and_dt0_noop);
  RUN_TEST(test_rng_deterministic);
  RUN_TEST(test_rng_range_full_span_no_div0);
  RUN_TEST(test_blinker_closes_within_window);
  RUN_TEST(test_blinker_reopens);
  RUN_TEST(test_gazer_ranges);
  return UNITY_END();
}
