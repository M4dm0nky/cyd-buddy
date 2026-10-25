#include <unity.h>
#include "logic/weather_mood.h"
void setUp() {}
void tearDown() {}

void test_wmo_mapping() {
  TEST_ASSERT_EQUAL((int)Sky::SUNNY, (int)weather_mood(0, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::SUNNY, (int)weather_mood(1, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::CLOUDY, (int)weather_mood(3, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::FOG, (int)weather_mood(45, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::RAIN, (int)weather_mood(61, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::RAIN, (int)weather_mood(81, 15).sky);
  TEST_ASSERT_EQUAL((int)Sky::SNOW, (int)weather_mood(73, -2).sky);
  TEST_ASSERT_EQUAL((int)Sky::SNOW, (int)weather_mood(86, 1).sky);
  TEST_ASSERT_EQUAL((int)Sky::STORM, (int)weather_mood(95, 20).sky);
  TEST_ASSERT_EQUAL((int)Sky::CLOUDY, (int)weather_mood(42, 20).sky);
}
void test_flags() {
  TEST_ASSERT_TRUE(weather_mood(0, 28.5f).hot);
  TEST_ASSERT_FALSE(weather_mood(0, 28.0f).hot);
  TEST_ASSERT_TRUE(weather_mood(3, -0.5f).cold);
  TEST_ASSERT_TRUE(weather_mood(71, 1.0f).cold);
  TEST_ASSERT_FALSE(weather_mood(3, 5.0f).cold);
}
void test_fresh() {
  TEST_ASSERT_FALSE(weather_fresh(1000, 0));
  TEST_ASSERT_TRUE(weather_fresh(1000 + WEATHER_MAX_AGE_MS - 1, 1000));
  TEST_ASSERT_FALSE(weather_fresh(1000 + WEATHER_MAX_AGE_MS + 1, 1000));
}
void test_overlays() {
  WeatherMood sun = weather_mood(0, 30);
  uint32_t o = overlays_for(Emotion::HAPPY, &sun, true);
  TEST_ASSERT_TRUE(o & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(o & OV_SWEAT);
  TEST_ASSERT_FALSE(overlays_for(Emotion::HAPPY, &sun, false) & OV_SUNGLASSES);
  TEST_ASSERT_FALSE(overlays_for(Emotion::SAD, &sun, true) & OV_SUNGLASSES);
  TEST_ASSERT_TRUE(overlays_for(Emotion::SAD, nullptr, true) & OV_TEAR);
  TEST_ASSERT_TRUE(overlays_for(Emotion::LOVE, nullptr, true) & OV_HEART);
  TEST_ASSERT_EQUAL_UINT32(OV_ZZZ, overlays_for(Emotion::SLEEPING, &sun, true));
  WeatherMood rain = weather_mood(63, 10);
  TEST_ASSERT_TRUE(overlays_for(Emotion::CONTENT, &rain, true) & OV_RAIN_TINT);
  WeatherMood storm = weather_mood(96, 10);
  TEST_ASSERT_TRUE(overlays_for(Emotion::CONTENT, &storm, true) & OV_STORM);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_wmo_mapping);
  RUN_TEST(test_flags);
  RUN_TEST(test_fresh);
  RUN_TEST(test_overlays);
  return UNITY_END();
}
