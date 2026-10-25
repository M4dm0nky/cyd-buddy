#include <unity.h>
#include "logic/gesture.h"

static GestureState g;
void setUp() { gesture_init(g); }
void tearDown() {}

// hält an (x,y) von t0 bis t1 in 20-ms-Schritten, gibt das erste Ereignis != NONE zurück
static Gesture hold(int16_t x, int16_t y, uint32_t t0, uint32_t t1) {
  Gesture first = Gesture::NONE;
  for (uint32_t t = t0; t <= t1; t += 20) { Gesture e = gesture_feed(g, true, x, y, t); if (first == Gesture::NONE) first = e; }
  return first;
}
static Gesture release(uint32_t t) { return gesture_feed(g, false, 0, 0, t); }
static Gesture tap_at(uint32_t t) { hold(100, 100, t, t + 100); return release(t + 120); }

void test_tap() { TEST_ASSERT_EQUAL((int)Gesture::TAP, (int)tap_at(0)); }
void test_too_long_is_not_tap() {
  hold(100, 100, 0, 600);
  TEST_ASSERT_EQUAL((int)Gesture::NONE, (int)release(620));
}
void test_long_press_once_then_release_none() {
  TEST_ASSERT_EQUAL((int)Gesture::LONG_PRESS, (int)hold(100, 100, 0, 1400));
  TEST_ASSERT_EQUAL((int)Gesture::NONE, (int)hold(100, 100, 1420, 2000));
  TEST_ASSERT_EQUAL((int)Gesture::NONE, (int)release(2020));
}
void test_stroke() {
  Gesture first = Gesture::NONE;
  for (int i = 0; i <= 20; i++) {   // 20 Schritte à 5 px = 100 px in 400 ms
    Gesture e = gesture_feed(g, true, 50 + i * 5, 100, i * 20);
    if (first == Gesture::NONE) first = e;
  }
  TEST_ASSERT_EQUAL((int)Gesture::STROKE, (int)first);
  TEST_ASSERT_EQUAL((int)Gesture::NONE, (int)release(500));
}
void test_slow_drag_no_stroke() {
  Gesture any = Gesture::NONE;
  for (int i = 0; i <= 100; i++) {   // 1 px pro 40 ms: in 1,5 s nur ~37 px
    Gesture e = gesture_feed(g, true, 50 + i, 100, i * 40);
    if (e != Gesture::NONE) any = e;
  }
  TEST_ASSERT_EQUAL((int)Gesture::NONE, (int)any);
}
void test_poke_on_fifth_tap() {
  for (int i = 0; i < 4; i++) TEST_ASSERT_EQUAL((int)Gesture::TAP, (int)tap_at(i * 400));
  TEST_ASSERT_EQUAL((int)Gesture::POKE, (int)tap_at(1600));
  TEST_ASSERT_EQUAL((int)Gesture::TAP, (int)tap_at(2000));   // Zähler wieder leer
}
void test_slow_taps_no_poke() {
  for (int i = 0; i < 6; i++) TEST_ASSERT_EQUAL((int)Gesture::TAP, (int)tap_at(i * 800));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_tap);
  RUN_TEST(test_too_long_is_not_tap);
  RUN_TEST(test_long_press_once_then_release_none);
  RUN_TEST(test_stroke);
  RUN_TEST(test_slow_drag_no_stroke);
  RUN_TEST(test_poke_on_fifth_tap);
  RUN_TEST(test_slow_taps_no_poke);
  return UNITY_END();
}
