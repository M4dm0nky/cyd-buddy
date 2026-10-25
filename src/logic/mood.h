#pragma once
#include <stdint.h>
#include "clamp.h"

// Bedürfnisse und Gefühl des Buddys. Reine Logik, Zeit kommt als Parameter (Host-Test test/test_mood).
enum class Emotion : uint8_t { CONTENT, HAPPY, BORED, SAD, SLEEPY, SLEEPING, SURPRISED, LOVE, ANNOYED, EXCITED, GREETING, COUNT };
enum class MoodEvent : uint8_t { TAP, STROKE, POKE, GAME_WON, GAME_LOST };
struct Needs { float affection, fun, energy; };
struct MoodState {
  Needs n;
  Emotion transient;          // CONTENT = keins
  uint32_t transient_until;
  uint32_t last_stroke_ms;
  bool stroked_once;
  bool napping;               // Nickerchen wegen Energie < 10
  bool dark;
  uint32_t dark_since;
  bool night_sleep;           // Nachtschlaf: nachts nach MOOD_NIGHT_IDLE_MS ohne Berührung
  uint32_t last_touch_ms;
};

constexpr float MOOD_AFFECTION_PER_MIN = -0.2f;
constexpr float MOOD_FUN_PER_MIN = -0.33f;
constexpr float MOOD_ENERGY_AWAKE_PER_MIN = -0.17f;
constexpr float MOOD_ENERGY_SLEEP_PER_MIN = 1.0f;
constexpr uint32_t MOOD_DARK_SLEEP_MS = 10000;
constexpr uint32_t MOOD_STROKE_COOLDOWN_MS = 2000;
constexpr uint32_t MOOD_NIGHT_IDLE_MS = 10UL * 60UL * 1000UL;

inline void mood_clamp(Needs &n) {
  n.affection = clampf(n.affection, 0, 100);
  n.fun = clampf(n.fun, 0, 100);
  n.energy = clampf(n.energy, 0, 100);
}
inline void mood_init(MoodState &s, Needs start) {
  s = MoodState{};
  s.n = start;
  s.transient = Emotion::CONTENT;
  mood_clamp(s.n);
}
inline bool mood_daytime(int hour) { return hour < 0 || (hour >= 7 && hour < 22); }
inline bool mood_is_sleeping(const MoodState &s, uint32_t now_ms) {
  return s.napping || s.night_sleep || (s.dark && now_ms - s.dark_since > MOOD_DARK_SLEEP_MS);
}
inline void mood_set_transient(MoodState &s, Emotion e, uint32_t now_ms, uint32_t dur_ms) {
  s.transient = e;
  s.transient_until = now_ms + dur_ms;
}
inline void mood_set_dark(MoodState &s, bool dark, int hour, uint32_t now_ms) {
  if (dark == s.dark) return;
  const bool was_sleeping = mood_is_sleeping(s, now_ms);
  s.dark = dark;
  if (dark) s.dark_since = now_ms;
  else if (was_sleeping && mood_daytime(hour)) mood_set_transient(s, Emotion::GREETING, now_ms, 3000);
}
// Jede Berührung (auch Menü, Spiele): weckt aus Nickerchen und Nachtschlaf und startet die Ruhezeit neu.
inline void mood_touch(MoodState &s, uint32_t now_ms) {
  s.napping = false;
  s.night_sleep = false;
  s.last_touch_ms = now_ms;
}
// Jedes Bild aufrufen. Nachts (22-7 Uhr) nach 10 min Ruhe einschlafen, morgens mit Begrüßung aufwachen.
// Der Lichtsensor des CYD erkennt Dunkelheit kaum (Beschaltung, das Display leuchtet ihn an), darum zählt vor allem die Uhrzeit.
inline void mood_night_check(MoodState &s, int hour, uint32_t now_ms) {
  const bool night = hour >= 0 && (hour >= 22 || hour < 7);
  if (!night) {
    if (s.night_sleep) {
      s.night_sleep = false;
      if (!mood_is_sleeping(s, now_ms)) mood_set_transient(s, Emotion::GREETING, now_ms, 3000);
    }
    return;
  }
  if (!s.night_sleep && now_ms - s.last_touch_ms > MOOD_NIGHT_IDLE_MS) s.night_sleep = true;
}
inline void mood_tick(MoodState &s, uint32_t dt_ms, uint32_t now_ms) {
  const float min = dt_ms / 60000.0f;
  if (mood_is_sleeping(s, now_ms)) {
    s.n.energy += MOOD_ENERGY_SLEEP_PER_MIN * min;   // im Schlaf nur Erholung, kein Abbau
  } else {
    s.n.affection += MOOD_AFFECTION_PER_MIN * min;
    s.n.fun += MOOD_FUN_PER_MIN * min;
    s.n.energy += MOOD_ENERGY_AWAKE_PER_MIN * min;
  }
  mood_clamp(s.n);
  if (s.n.energy < 10) s.napping = true;
  else if (s.napping && s.n.energy > 60) s.napping = false;
}
inline void mood_event(MoodState &s, MoodEvent e, uint32_t now_ms) {
  mood_touch(s, now_ms);   // jede Berührung weckt
  switch (e) {
    case MoodEvent::TAP: s.n.fun += 1; mood_set_transient(s, Emotion::SURPRISED, now_ms, 1500); break;
    case MoodEvent::STROKE:
      if (s.stroked_once && now_ms - s.last_stroke_ms < MOOD_STROKE_COOLDOWN_MS) break;
      s.stroked_once = true;
      s.last_stroke_ms = now_ms;
      s.n.affection += 8;
      mood_set_transient(s, Emotion::LOVE, now_ms, 4000);
      break;
    case MoodEvent::POKE: s.n.affection -= 3; mood_set_transient(s, Emotion::ANNOYED, now_ms, 20000); break;
    case MoodEvent::GAME_WON: s.n.fun += 25; s.n.affection += 5; mood_set_transient(s, Emotion::EXCITED, now_ms, 6000); break;
    case MoodEvent::GAME_LOST: s.n.fun += 10; break;
  }
  mood_clamp(s.n);
}
inline Emotion mood_emotion(const MoodState &s, int hour, uint32_t now_ms) {
  if (mood_is_sleeping(s, now_ms)) return Emotion::SLEEPING;
  if (s.transient != Emotion::CONTENT && (int32_t)(s.transient_until - now_ms) > 0) return s.transient;
  if (s.n.energy < 30 || (hour >= 0 && (hour >= 22 || hour < 6))) return Emotion::SLEEPY;
  if (s.n.affection < 20) return Emotion::SAD;
  if (s.n.fun < 25) return Emotion::BORED;
  if (s.n.affection > 70 && s.n.fun > 60) return Emotion::HAPPY;
  return Emotion::CONTENT;
}
// Zeit, in der das Gerät aus war, zählt wie Schlaf: nur Energie steigt.
inline void mood_apply_elapsed(MoodState &s, uint32_t seconds) {
  s.n.energy += MOOD_ENERGY_SLEEP_PER_MIN * (seconds / 60.0f);
  mood_clamp(s.n);
}
