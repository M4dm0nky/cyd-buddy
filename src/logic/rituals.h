#pragma once
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "weather_mood.h"

// Tägliche Rituale (F2): Morgengruß, Gute Nacht, Tagesrückblick. Stunde/Minute/Tag als Parameter (Host-Test test/test_speech).
// day = YYYYMMDD (0 = unbekannt), hour -1 = Zeit unbekannt.
struct RitualState { uint32_t morning_day, night_day, review_day; };

inline uint32_t day_key(int year, int month, int mday) { return (uint32_t)year * 10000u + (uint32_t)month * 100u + (uint32_t)mday; }

// woke = erstes Aufwachen (Ende des Nachtschlafs oder erste Berührung). Einmal pro Tag zwischen 5 und 12 Uhr.
inline bool ritual_morning(RitualState &r, bool woke, int hour, uint32_t day) {
  if (!woke || day == 0 || hour < 5 || hour >= 12 || r.morning_day == day) return false;
  r.morning_day = day;
  return true;
}
// Um 22 Uhr (erstes Bild 22:xx, in dem er wach ist), einmal pro Tag.
inline bool ritual_night(RitualState &r, bool awake, int hour, int minute, uint32_t day) {
  (void)minute;
  if (!awake || day == 0 || hour != 22 || r.night_day == day) return false;
  r.night_day = day;
  return true;
}
// Rückblick: um 20:00, wenn er wach ist; sonst bei der nächsten Berührung bis 23 Uhr. Einmal pro Tag.
inline bool ritual_review(RitualState &r, bool awake, bool touched, int hour, int minute, uint32_t day) {
  if (day == 0 || r.review_day == day || hour < 20 || hour >= 23) return false;
  const bool at_eight = hour == 20 && minute == 0 && awake;
  if (!at_eight && !touched) return false;
  r.review_day = day;
  return true;
}

inline const char *sky_word(Sky s) {
  switch (s) {
    case Sky::SUNNY: return "Sonne";
    case Sky::CLOUDY: return "Wolken";
    case Sky::FOG: return "Nebel";
    case Sky::RAIN: return "Regen";
    case Sky::SNOW: return "Schnee";
    default: return "Gewitter";
  }
}
// „Heute 14° und Regen“ (UTF-8, Grad gerundet).
inline void weather_sentence(float temp_c, Sky sky, char *out, size_t n) {
  snprintf(out, n, "Heute %ld\xC2\xB0 und %s", lroundf(temp_c), sky_word(sky));
}
