#pragma once
#include <stdint.h>
#include "mood.h"
#include "phrases.h"

// Füttern (F3). Reine Logik (Host-Test test/test_care).
enum class Food : uint8_t { APPLE, PIZZA, CAKE, COFFEE, COUNT };
struct FoodEffect { float satiety, fun, energy; };

inline FoodEffect food_effect(Food f) {
  switch (f) {
    case Food::APPLE: return {15, 0, 0};
    case Food::PIZZA: return {35, 0, 0};
    case Food::CAKE: return {25, 5, 0};
    case Food::COFFEE: return {5, 0, 15};
    default: return {0, 0, 0};
  }
}

constexpr float FOOD_FULL_ABOVE = 95;

// Wirkung anwenden und Reaktion setzen; liefert die Kategorie der Sprechblase.
// Lieblingsessen: Zuneigung +5, Unterhaltung +5, EXCITED 3 s; sonst HAPPY 2 s. Sättigung danach > 95: FULL 5 s (Bauchweh).
inline Say food_feed(MoodState &s, Food f, Food favorite, uint32_t now_ms) {
  mood_touch(s, now_ms);
  const FoodEffect e = food_effect(f);
  s.n.satiety += e.satiety;
  s.n.fun += e.fun;
  s.n.energy += e.energy;
  Say say = Say::LECKER;
  if (f == favorite) {
    s.n.affection += 5;
    s.n.fun += 5;
    mood_set_transient(s, Emotion::EXCITED, now_ms, 3000);
    say = Say::LIEBLINGSESSEN;
  } else {
    mood_set_transient(s, Emotion::HAPPY, now_ms, 2000);
  }
  const bool full = s.n.satiety > FOOD_FULL_ABOVE;
  mood_clamp(s.n);
  if (full) {
    mood_set_transient(s, Emotion::FULL, now_ms, 5000);
    say = Say::SATT;
  }
  return say;
}
