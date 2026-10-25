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
  // Spec Gesicht: das 3. Streicheln innerhalb 20 s kichert (GIGGLE) -> drittes erst nach 20 s
  mood_event(s, MoodEvent::STROKE, 40000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 66.0f, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)mood_emotion(s, 12, 40500));
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

// Nachtschlaf: nachts (22-7 Uhr) nach 10 min ohne Berührung einschlafen (der Lichtsensor des CYD taugt dafür nicht)
void test_night_idle_sleeps_after_10min() {
  fresh(50, 50, 50);
  mood_touch(s, 0);
  mood_night_check(s, 23, 599999);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 599999));
  mood_night_check(s, 23, 600001);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 600001));
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPING, (int)mood_emotion(s, 23, 600001));
}
void test_day_idle_stays_awake() {
  fresh(50, 50, 50);
  mood_touch(s, 0);
  mood_night_check(s, 14, 7200000UL);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 7200000UL));
  mood_night_check(s, -1, 7200000UL);   // Uhrzeit unbekannt: nie Nachtschlaf
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 7200000UL));
}
void test_touch_wakes_from_night_sleep() {
  fresh(50, 50, 50);
  mood_night_check(s, 2, 700000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 700000));
  mood_touch(s, 700100);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 700100));
  mood_night_check(s, 2, 700200);   // gerade berührt: bleibt wach
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 700200));
}
void test_morning_wakes_with_greeting() {
  fresh(50, 50, 50);
  mood_night_check(s, 6, 700000);
  TEST_ASSERT_TRUE(mood_is_sleeping(s, 700000));
  mood_night_check(s, 7, 800000);
  TEST_ASSERT_FALSE(mood_is_sleeping(s, 800000));
  TEST_ASSERT_EQUAL((int)Emotion::GREETING, (int)mood_emotion(s, 7, 801000));
}

// ---- Neue Ausloeser (Spec Gesicht) ----
static int E(Emotion e) { return (int)e; }

void test_giggle_third_stroke_in_20s() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);
  mood_event(s, MoodEvent::STROKE, 1500);   // Cooldown: zaehlt trotzdem
  TEST_ASSERT_EQUAL(E(Emotion::LOVE), E(mood_emotion(s, 12, 2000)));
  mood_event(s, MoodEvent::STROKE, 3000);
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 3500)));
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 5999)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 6001)));
}
void test_no_giggle_when_spread_over_20s() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::STROKE, 0);
  mood_event(s, MoodEvent::STROKE, 15000);
  mood_event(s, MoodEvent::STROKE, 30000);
  TEST_ASSERT_EQUAL(E(Emotion::LOVE), E(mood_emotion(s, 12, 30500)));
}
void test_kiss_when_affection_over_90() {
  fresh(85, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);   // 93
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 1500)));
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 3999)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 4001)));
}
void test_giggle_beats_kiss_beats_love() {
  fresh(95, 50, 50);
  mood_event(s, MoodEvent::STROKE, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::KISS), E(mood_emotion(s, 12, 1100)));
  mood_event(s, MoodEvent::STROKE, 3500);
  mood_event(s, MoodEvent::STROKE, 6000);
  TEST_ASSERT_EQUAL(E(Emotion::GIGGLE), E(mood_emotion(s, 12, 6100)));
}
void test_double_tap_confused() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 100);   // erster Tipp: nie verwirrt
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 200)));
  mood_event(s, MoodEvent::TAP, 1000);
  mood_event(s, MoodEvent::TAP, 1400);   // < 500 ms
  TEST_ASSERT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 1500)));
  TEST_ASSERT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 3899)));
  TEST_ASSERT_NOT_EQUAL(E(Emotion::CONFUSED), E(mood_emotion(s, 12, 3901)));
}
void test_slow_taps_stay_surprised() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  mood_event(s, MoodEvent::TAP, 1500);   // genau 500 ms: noch nicht verwirrt
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 1600)));
}
void test_poke_when_sad_is_scared() {
  fresh(10, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000, 12);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.0f, s.n.affection);
  TEST_ASSERT_EQUAL(E(Emotion::SCARED), E(mood_emotion(s, 12, 5999)));
  TEST_ASSERT_EQUAL(E(Emotion::SAD), E(mood_emotion(s, 12, 6001)));
}
void test_poke_when_not_sad_still_annoyed() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::POKE, 1000, 12);
  TEST_ASSERT_EQUAL(E(Emotion::ANNOYED), E(mood_emotion(s, 12, 2000)));
}
void test_game_won_then_proud() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  TEST_ASSERT_EQUAL(E(Emotion::EXCITED), E(mood_emotion(s, 12, 5999)));
  TEST_ASSERT_EQUAL(E(Emotion::PROUD), E(mood_emotion(s, 12, 6001)));
  TEST_ASSERT_EQUAL(E(Emotion::PROUD), E(mood_emotion(s, 12, 9999)));
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 10001)));
}
void test_new_transient_cancels_proud_follow() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_WON, 0);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 7000)));
}
void test_game_lost_skeptical() {
  fresh(50, 50, 50);
  mood_event(s, MoodEvent::GAME_LOST, 0);
  TEST_ASSERT_EQUAL(E(Emotion::SKEPTICAL), E(mood_emotion(s, 12, 3999)));
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, 4001)));
}
void test_base_emotion_ignores_transient() {
  fresh(10, 50, 50);
  mood_event(s, MoodEvent::TAP, 1000);
  TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, 1100)));
  TEST_ASSERT_EQUAL(E(Emotion::SAD), E(mood_base_emotion(s, 12)));
}

// ---- Eigenarten ----
// Laeuft Frames im 1-s-Takt; liefert Zeitpunkt des ersten Auftretens von `want` (oder -1) und merkt sich weitere.
static long first_seen(Emotion want, int hour, uint32_t until_ms) {
  for (uint32_t t = 0; t <= until_ms; t += 1000) {
    mood_quirks(s, hour, t);
    if (mood_emotion(s, hour, t) == want) return (long)t;
  }
  return -1;
}
void test_quirk_yawn_when_sleepy_within_5min_for_3s() {
  fresh(50, 50, 20);   // SLEEPY
  mood_seed(s, 12345);
  long t0 = first_seen(Emotion::YAWN, 12, 300000);
  TEST_ASSERT_TRUE(t0 >= 120000);
  TEST_ASSERT_EQUAL(E(Emotion::YAWN), E(mood_emotion(s, 12, t0 + 2999)));
  TEST_ASSERT_EQUAL(E(Emotion::SLEEPY), E(mood_emotion(s, 12, t0 + 3001)));
}
void test_quirk_whistle_when_bored_within_3min_for_5s() {
  fresh(50, 10, 50);   // BORED
  mood_seed(s, 777);
  long t0 = first_seen(Emotion::WHISTLE, 12, 180000);
  TEST_ASSERT_TRUE(t0 >= 60000);
  TEST_ASSERT_EQUAL(E(Emotion::WHISTLE), E(mood_emotion(s, 12, t0 + 4999)));
  TEST_ASSERT_EQUAL(E(Emotion::BORED), E(mood_emotion(s, 12, t0 + 5001)));
}
void test_quirk_happy_gives_mischievous_or_cheeky_both_occur() {
  bool seen_m = false, seen_c = false;
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 50);   // HAPPY
    mood_seed(s, seed);
    bool got = false;
    for (uint32_t t = 0; t <= 360000; t += 1000) {
      mood_quirks(s, 12, t);
      const Emotion e = mood_emotion(s, 12, t);
      if (e == Emotion::MISCHIEVOUS) { seen_m = true; got = true; break; }
      if (e == Emotion::CHEEKY) { seen_c = true; got = true; break; }
    }
    TEST_ASSERT_TRUE(got);
  }
  TEST_ASSERT_TRUE(seen_m && seen_c);
}
void test_quirk_none_when_sleeping() {
  fresh(50, 50, 20);
  mood_seed(s, 12345);
  s.napping = true;
  for (uint32_t t = 0; t <= 900000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::SLEEPING), E(mood_emotion(s, 12, t)));
  }
  TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(s.transient));
}
void test_quirk_none_during_other_transient() {
  fresh(50, 10, 50);   // BORED
  mood_seed(s, 777);
  mood_set_transient(s, Emotion::SURPRISED, 0, 1000000);
  for (uint32_t t = 0; t <= 400000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::SURPRISED), E(mood_emotion(s, 12, t)));
  }
}
void test_quirk_none_for_other_base_emotions() {
  fresh(50, 50, 50);   // CONTENT
  mood_seed(s, 5);
  for (uint32_t t = 0; t <= 900000; t += 1000) {
    mood_quirks(s, 12, t);
    TEST_ASSERT_EQUAL(E(Emotion::CONTENT), E(mood_emotion(s, 12, t)));
  }
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_giggle_third_stroke_in_20s);
  RUN_TEST(test_no_giggle_when_spread_over_20s);
  RUN_TEST(test_kiss_when_affection_over_90);
  RUN_TEST(test_giggle_beats_kiss_beats_love);
  RUN_TEST(test_double_tap_confused);
  RUN_TEST(test_slow_taps_stay_surprised);
  RUN_TEST(test_poke_when_sad_is_scared);
  RUN_TEST(test_poke_when_not_sad_still_annoyed);
  RUN_TEST(test_game_won_then_proud);
  RUN_TEST(test_new_transient_cancels_proud_follow);
  RUN_TEST(test_game_lost_skeptical);
  RUN_TEST(test_base_emotion_ignores_transient);
  RUN_TEST(test_quirk_yawn_when_sleepy_within_5min_for_3s);
  RUN_TEST(test_quirk_whistle_when_bored_within_3min_for_5s);
  RUN_TEST(test_quirk_happy_gives_mischievous_or_cheeky_both_occur);
  RUN_TEST(test_quirk_none_when_sleeping);
  RUN_TEST(test_quirk_none_during_other_transient);
  RUN_TEST(test_quirk_none_for_other_base_emotions);
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
  RUN_TEST(test_night_idle_sleeps_after_10min);
  RUN_TEST(test_day_idle_stays_awake);
  RUN_TEST(test_touch_wakes_from_night_sleep);
  RUN_TEST(test_morning_wakes_with_greeting);
  return UNITY_END();
}
