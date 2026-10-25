#include <unity.h>
#include "logic/mood.h"

static MoodState s;
static void fresh(float a, float f, float e) { mood_init(s, Needs{a, f, e}); }
void setUp() {}
void tearDown() {}

void test_decay_awake_one_hour() {
  fresh(50, 50, 50);
  mood_tick(s, 3600000UL, 3600000UL);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 38.0f, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 30.2f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 39.8f, s.n.energy);
}
void test_sleep_regenerates_and_clamps() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 23, 0);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 5000));
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 10001));
  mood_tick(s, 3600000UL, 3610000UL);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, s.n.energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, s.n.affection);   // im Schlaf kein Abbau
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50.0f, s.n.fun);
}
void test_stroke_cooldown() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 10000);
  mood_event(s, MoodEvent::STROKE, 11000);   // < 2 s: zählt nicht
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 58.0f, s.n.affection);
  mood_event(s, MoodEvent::STROKE, 12500);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 66.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)mood_emotion(s, 12, 13000));
}
void test_affection_clamped_at_100() {
  fresh(99, 50, 50);
  mood_event(s, MoodEvent::STROKE, 5000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100.0f, s.n.affection);
}
void test_poke_annoyed_20s() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 47.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::ANNOYED, (int)mood_emotion(s, 12, 20999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 21001));
}
void test_transient_beats_sad() {
  fresh(10, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL((int)Emotion::SURPRISED, (int)mood_emotion(s, 12, 2000));
  TEST_ASSERT_EQUAL((int)Emotion::SAD, (int)mood_emotion(s, 12, 2600));
}
void test_priorities() {
  fresh(50, 50, 50);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 23, 0));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, -1, 0));
  fresh(10, 10, 50);
  TEST_ASSERT_EQUAL((int)Emotion::SAD, (int)mood_emotion(s, 12, 0));
  fresh(50, 10, 50);
  TEST_ASSERT_EQUAL((int)Emotion::BORED, (int)mood_emotion(s, 12, 0));
  fresh(80, 70, 50);
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 0));
  fresh(50, 50, 20);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 12, 0));
}
void test_nap_and_wake_by_touch() {
  fresh(50, 50, 5);
  mood_tick(s, 1000, 1000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 1000));
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPING, (int)mood_emotion(s, 12, 1000));
  mood_event(s, MoodEvent::TAP, 2000);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 2000));
}
void test_nap_ends_above_60() {
  fresh(50, 50, 5);
  mood_tick(s, 1000, 1000);
  mood_tick(s, 56UL * 60000UL, 57UL * 60000UL);   // +56 Energie -> ~61
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 57UL * 60000UL));
}
void test_greeting_when_light_returns_daytime() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 9, 0);
  mood_set_dark(s, false, 9, 20000);
  TEST_ASSERT_EQUAL((int)Emotion::GREETING, (int)mood_emotion(s, 9, 21000));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 9, 23001));
}
void test_no_greeting_at_night() {
  fresh(50, 50, 50);
  mood_set_dark(s, true, 2, 0);
  mood_set_dark(s, false, 2, 20000);
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_emotion(s, 2, 21000));
}
void test_game_won() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 75.0f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(s, 12, 5999));
}
void test_elapsed_counts_as_sleep() {
  fresh(50, 40, 20);
  mood_apply_elapsed(s, 30UL * 60UL);   // ausgeschaltet = geschlafen: nur Energie steigt
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 40.0f, s.n.fun);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f, s.n.energy);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_decay_awake_one_hour);
  RUN_TEST(test_sleep_regenerates_and_clamps);
  RUN_TEST(test_stroke_cooldown);
  RUN_TEST(test_affection_clamped_at_100);
  RUN_TEST(test_poke_annoyed_20s);
  RUN_TEST(test_transient_beats_sad);
  RUN_TEST(test_priorities);
  RUN_TEST(test_nap_and_wake_by_touch);
  RUN_TEST(test_nap_ends_above_60);
  RUN_TEST(test_greeting_when_light_returns_daytime);
  RUN_TEST(test_no_greeting_at_night);
  RUN_TEST(test_game_won);
  RUN_TEST(test_elapsed_counts_as_sleep);
  return UNITY_END();
}
