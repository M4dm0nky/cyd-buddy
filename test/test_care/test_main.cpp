#include <unity.h>
#include "logic/mood.h"
#include "logic/food.h"
#include "logic/character.h"
#include "logic/speech.h"
void setUp() {}
void tearDown() {}

static constexpr uint32_t MIN = 60000UL;
static MoodState s;
static void fresh(float a, float f, float e, float sat) { mood_init(s, Needs{a, f, e, sat}); }

// ---- Sättigung ----
void test_satiety_start_and_decay() {
  mood_init(s, Needs{50, 50, 50});
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 70.0f, s.n.satiety);   // Startwert
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 70.0f - 7.2f, s.n.satiety);   // wach -0,12/min
  fresh(50, 50, 50, 50);
  s.night_sleep = true;
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f - 2.4f, s.n.satiety);   // schlafend -0,04/min
  fresh(50, 50, 50, 0.1f);
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, s.n.satiety);          // geklemmt
}
void test_hungry_rank() {
  fresh(50, 50, 50, 24);
  TEST_ASSERT_EQUAL((int)Emotion::HUNGRY, (int)mood_base_emotion(s, 12));
  fresh(10, 10, 50, 24);   // traurig und gelangweilt: Hunger geht vor
  TEST_ASSERT_EQUAL((int)Emotion::HUNGRY, (int)mood_base_emotion(s, 12));
  fresh(50, 50, 20, 24);   // müde geht vor Hunger
  TEST_ASSERT_EQUAL((int)Emotion::SLEEPY, (int)mood_base_emotion(s, 12));
  fresh(50, 50, 50, 25);
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_base_emotion(s, 12));
}

// ---- Krankheit ----
void test_sick_after_2h_uninterrupted() {
  fresh(10, 50, 100, 10);
  for (uint32_t t = 1000; t <= 119 * MIN; t += MIN) { mood_tick(s, MIN, t); s.n.energy = 100; s.n.affection = 10; s.n.satiety = 10; }
  TEST_ASSERT_FALSE(s.sick);
  for (uint32_t t = 120 * MIN; t <= 122 * MIN; t += MIN) { mood_tick(s, MIN, t); s.n.energy = 100; s.n.affection = 10; s.n.satiety = 10; }
  TEST_ASSERT_TRUE(s.sick);
  TEST_ASSERT_EQUAL((int)Emotion::SICK, (int)mood_base_emotion(s, 12));
  TEST_ASSERT_EQUAL((int)Emotion::SICK, (int)mood_base_emotion(s, 23));   // Rang vor SLEEPY
}
void test_sick_needs_both_conditions_and_resets() {
  fresh(10, 50, 100, 10);
  for (uint32_t t = 1000; t <= 100 * MIN; t += MIN) { mood_tick(s, MIN, t); s.n.energy = 100; s.n.affection = 10; s.n.satiety = 10; }
  s.n.affection = 30;   // Unterbrechung
  mood_tick(s, MIN, 101 * MIN);
  s.n.affection = 10;
  for (uint32_t t = 102 * MIN; t <= 200 * MIN; t += MIN) { mood_tick(s, MIN, t); s.n.energy = 100; s.n.affection = 10; s.n.satiety = 10; }
  TEST_ASSERT_FALSE(s.sick);   // erst 98 min seit der Unterbrechung
  fresh(50, 50, 100, 10);       // nur hungrig, Zuneigung ok
  for (uint32_t t = 1000; t <= 300 * MIN; t += MIN) { mood_tick(s, MIN, t); s.n.energy = 100; s.n.satiety = 10; s.n.affection = 50; }
  TEST_ASSERT_FALSE(s.sick);
}
void test_sick_double_energy_decay() {
  fresh(50, 50, 50, 50);
  s.sick = true;
  mood_tick(s, 60 * MIN, 60 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 50.0f - 2 * 10.2f, s.n.energy);
}
void test_heal_needs_satiety_then_3_strokes() {
  fresh(50, 50, 80, 30);
  s.sick = true;
  for (int i = 0; i < 3; i++) mood_event(s, MoodEvent::STROKE, 1000 + i * 3000, 12);
  TEST_ASSERT_TRUE(s.sick);              // Sättigung noch zu niedrig: Streicheln zählt nicht
  s.n.satiety = 61;
  mood_tick(s, 100, 20000);
  mood_event(s, MoodEvent::STROKE, 21000, 12);
  mood_event(s, MoodEvent::STROKE, 24000, 12);
  TEST_ASSERT_TRUE(s.sick);
  mood_event(s, MoodEvent::STROKE, 27000, 12);
  TEST_ASSERT_FALSE(s.sick);
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 29999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 30001));
}
void test_sneeze_every_1_to_3_min_only_when_sick() {
  fresh(50, 50, 80, 50);
  mood_seed(s, 42);
  for (uint32_t t = 0; t < 10 * MIN; t += 1000) TEST_ASSERT_FALSE(mood_sneeze_due(s, t));
  s.sick = true;
  uint32_t last = 10 * MIN, n = 0;
  for (uint32_t t = 10 * MIN; t < 40 * MIN; t += 1000) {
    if (mood_sneeze_due(s, t)) {
      TEST_ASSERT_TRUE(t - last >= 1 * MIN && t - last <= 3 * MIN + 1000);
      last = t; n++;
    }
  }
  TEST_ASSERT_TRUE(n >= 10);
}

// ---- Füttern ----
void test_food_effects() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15, food_effect(Food::APPLE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 35, food_effect(Food::PIZZA).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 25, food_effect(Food::CAKE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5, food_effect(Food::CAKE).fun);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 5, food_effect(Food::COFFEE).satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 15, food_effect(Food::COFFEE).energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0, food_effect(Food::APPLE).fun);
}
void test_feed_normal() {
  fresh(50, 50, 50, 40);
  TEST_ASSERT_EQUAL((int)Say::LECKER, (int)food_feed(s, Food::PIZZA, Food::APPLE, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 75, s.n.satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 50, s.n.affection);
  TEST_ASSERT_EQUAL((int)Emotion::HAPPY, (int)mood_emotion(s, 12, 2999));
  TEST_ASSERT_EQUAL((int)Emotion::CONTENT, (int)mood_emotion(s, 12, 3001));
}
void test_feed_favorite() {
  fresh(50, 50, 50, 40);
  TEST_ASSERT_EQUAL((int)Say::LIEBLINGSESSEN, (int)food_feed(s, Food::CAKE, Food::CAKE, 1000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 65, s.n.satiety);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 55, s.n.affection);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 60, s.n.fun);   // Kuchen +5, Liebling +5
  TEST_ASSERT_EQUAL((int)Emotion::EXCITED, (int)mood_emotion(s, 12, 3999));
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 4001) == Emotion::EXCITED);
}
void test_feed_full_strictly_above_95() {
  fresh(50, 50, 50, 70);
  food_feed(s, Food::CAKE, Food::APPLE, 1000);   // 95: genau, noch nicht satt
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 2000) == Emotion::FULL);
  fresh(50, 50, 50, 81);
  TEST_ASSERT_EQUAL((int)Say::SATT, (int)food_feed(s, Food::APPLE, Food::CAKE, 1000));   // 96
  TEST_ASSERT_EQUAL((int)Emotion::FULL, (int)mood_emotion(s, 12, 5999));
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 6001) == Emotion::FULL);
  fresh(50, 50, 50, 90);
  food_feed(s, Food::PIZZA, Food::APPLE, 1000);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 100, s.n.satiety);   // geklemmt
}
void test_coffee_energy() {
  fresh(50, 50, 50, 40);
  food_feed(s, Food::COFFEE, Food::APPLE, 0);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 65, s.n.energy);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 45, s.n.satiety);
}
void test_speech_for_hungry_and_sick() {
  TEST_ASSERT_EQUAL((int)Say::HUNGRIG, (int)speech_for_base(Emotion::HUNGRY));
  TEST_ASSERT_EQUAL((int)Say::KRANK, (int)speech_for_base(Emotion::SICK));
}

// ---- Charakter ----
void test_character_changes_and_clamp() {
  Character c{};
  char_stroke(c); char_stroke(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, c.cuddly);
  char_poke(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -0.5f, c.cuddly);
  char_game(c); char_highfive(c);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.3f, c.playful);
  char_bored(c, 10 * MIN);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.1f, c.playful);
  for (int i = 0; i < 1000; i++) { char_poke(c); char_game(c); }
  TEST_ASSERT_FLOAT_WITHIN(0.001f, -100.0f, c.cuddly);
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, c.playful);
}
void test_character_effects() {
  Character c{};
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, char_cheeky_prob(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_love_scale(c));
  c.playful = 100; c.cuddly = 100;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, char_cheeky_prob(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, char_love_scale(c));
  c.playful = -100; c.cuddly = -100;
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, char_quirk_scale(c));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, char_cheeky_prob(c));
}
// Verspielter Charakter: Eigenarten (HAPPY: 2–6 min) kommen nach höchstens 3 min.
void test_character_quirk_interval() {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 80, 70);
    mood_seed(s, seed);
    Character c{0, 100};
    mood_set_character(s, c);
    uint32_t first = 0;
    for (uint32_t t = 0; t <= 7 * MIN; t += 1000) {
      mood_quirks(s, 12, t);
      const Emotion e = mood_emotion(s, 12, t);
      if (e == Emotion::MISCHIEVOUS || e == Emotion::CHEEKY) { first = t; break; }
    }
    TEST_ASSERT_TRUE(first >= 1 * MIN && first <= 3 * MIN + 1000);
  }
}
// Sehr verschmust: nie CHEEKY, immer MISCHIEVOUS; LOVE dauert 1,5× so lang.
void test_character_cheeky_and_love() {
  for (uint32_t seed = 1; seed <= 30; seed++) {
    fresh(80, 70, 80, 70);
    mood_seed(s, seed);
    mood_set_character(s, Character{100, 0});
    for (uint32_t t = 0; t <= 7 * MIN; t += 1000) {
      mood_quirks(s, 12, t);
      TEST_ASSERT_FALSE(mood_emotion(s, 12, t) == Emotion::CHEEKY);
    }
  }
  fresh(50, 50, 50, 70);
  mood_set_character(s, Character{100, 0});
  mood_event(s, MoodEvent::STROKE, 1000, 12);
  TEST_ASSERT_EQUAL((int)Emotion::LOVE, (int)mood_emotion(s, 12, 1000 + 5999));
  TEST_ASSERT_FALSE(mood_emotion(s, 12, 1000 + 6001) == Emotion::LOVE);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_satiety_start_and_decay);
  RUN_TEST(test_hungry_rank);
  RUN_TEST(test_sick_after_2h_uninterrupted);
  RUN_TEST(test_sick_needs_both_conditions_and_resets);
  RUN_TEST(test_sick_double_energy_decay);
  RUN_TEST(test_heal_needs_satiety_then_3_strokes);
  RUN_TEST(test_sneeze_every_1_to_3_min_only_when_sick);
  RUN_TEST(test_food_effects);
  RUN_TEST(test_feed_normal);
  RUN_TEST(test_feed_favorite);
  RUN_TEST(test_feed_full_strictly_above_95);
  RUN_TEST(test_coffee_energy);
  RUN_TEST(test_speech_for_hungry_and_sick);
  RUN_TEST(test_character_changes_and_clamp);
  RUN_TEST(test_character_effects);
  RUN_TEST(test_character_quirk_interval);
  RUN_TEST(test_character_cheeky_and_love);
  return UNITY_END();
}
