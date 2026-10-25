#pragma once
#include <stdint.h>
#include "clamp.h"
#include "rng.h"

// Bedürfnisse und Gefühl des Buddys. Reine Logik, Zeit kommt als Parameter (Host-Test test/test_mood).
enum class Emotion : uint8_t { CONTENT, HAPPY, BORED, SAD, SLEEPY, SLEEPING, SURPRISED, LOVE, ANNOYED, EXCITED, GREETING,
                            CONFUSED, MISCHIEVOUS, SCARED, SKEPTICAL, PROUD, GIGGLE, YAWN, WHISTLE, KISS, CHEEKY, STARTLED, COUNT };
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
  // Auslöser (Spec Gesicht)
  uint32_t stroke_hist[3];    // Zeitpunkte der letzten 3 Streichler (auch im Cooldown)
  uint8_t stroke_n;
  uint32_t last_tap_ms;
  bool tapped_once;
  Emotion follow;             // Folge-Gefühl nach dem kurzzeitigen (CONTENT = keins)
  uint32_t follow_ms;
  // Eigenarten
  uint32_t quirk_seed;
  uint32_t quirk_next_ms;
  bool quirk_scheduled;
  bool quirk_base_known;
  Emotion quirk_base;
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
  s.follow = Emotion::CONTENT;   // ein neues Gefühl bricht die Folge ab
  s.follow_ms = 0;
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
constexpr uint32_t MOOD_GIGGLE_WINDOW_MS = 20000;
constexpr uint32_t MOOD_DOUBLE_TAP_MS = 500;

// Grundgefühl: aus den Bedürfnissen, ohne Schlaf und ohne kurzzeitige Gefühle.
inline Emotion mood_base_emotion(const MoodState &s, int hour) {
  if (s.n.energy < 30 || (hour >= 0 && (hour >= 22 || hour < 6))) return Emotion::SLEEPY;
  if (s.n.affection < 20) return Emotion::SAD;
  if (s.n.fun < 25) return Emotion::BORED;
  if (s.n.affection > 70 && s.n.fun > 60) return Emotion::HAPPY;
  return Emotion::CONTENT;
}
// hour nur für POKE nötig (bei SAD erschrickt er statt zu schimpfen); -1 = unbekannt.
inline void mood_event(MoodState &s, MoodEvent e, uint32_t now_ms, int hour = -1) {
  mood_touch(s, now_ms);   // jede Berührung weckt
  switch (e) {
    case MoodEvent::TAP: {
      const bool dbl = s.tapped_once && now_ms - s.last_tap_ms < MOOD_DOUBLE_TAP_MS;
      s.tapped_once = true;
      s.last_tap_ms = now_ms;
      s.n.fun += 1;
      if (dbl) mood_set_transient(s, Emotion::CONFUSED, now_ms, 2500);
      else mood_set_transient(s, Emotion::SURPRISED, now_ms, 1500);
      break;
    }
    case MoodEvent::STROKE: {
      // Die letzten 3 Streichler merken (auch im Cooldown): der 3. innerhalb 20 s lässt ihn kichern.
      s.stroke_hist[0] = s.stroke_hist[1];
      s.stroke_hist[1] = s.stroke_hist[2];
      s.stroke_hist[2] = now_ms;
      if (s.stroke_n < 3) s.stroke_n++;
      const bool giggle = s.stroke_n >= 3 && now_ms - s.stroke_hist[0] < MOOD_GIGGLE_WINDOW_MS;
      const bool cooldown = s.stroked_once && now_ms - s.last_stroke_ms < MOOD_STROKE_COOLDOWN_MS;
      if (!cooldown) {
        s.stroked_once = true;
        s.last_stroke_ms = now_ms;
        s.n.affection += 8;
        mood_set_transient(s, s.n.affection > 90 ? Emotion::KISS : Emotion::LOVE, now_ms, s.n.affection > 90 ? 3000 : 4000);
      }
      if (giggle) {
        s.stroke_n = 0;
        mood_set_transient(s, Emotion::GIGGLE, now_ms, 3000);
      }
      break;
    }
    case MoodEvent::POKE: {
      const bool sad = mood_base_emotion(s, hour) == Emotion::SAD;
      s.n.affection -= 3;
      if (sad) mood_set_transient(s, Emotion::SCARED, now_ms, 5000);
      else mood_set_transient(s, Emotion::ANNOYED, now_ms, 20000);
      break;
    }
    case MoodEvent::GAME_WON:
      s.n.fun += 25;
      s.n.affection += 5;
      mood_set_transient(s, Emotion::EXCITED, now_ms, 6000);
      s.follow = Emotion::PROUD;
      s.follow_ms = 4000;
      break;
    case MoodEvent::GAME_LOST: s.n.fun += 10; mood_set_transient(s, Emotion::SKEPTICAL, now_ms, 4000); break;
  }
  mood_clamp(s.n);
}
inline Emotion mood_emotion(const MoodState &s, int hour, uint32_t now_ms) {
  if (mood_is_sleeping(s, now_ms)) return Emotion::SLEEPING;
  if (s.transient != Emotion::CONTENT) {
    if ((int32_t)(s.transient_until - now_ms) > 0) return s.transient;
    if (s.follow != Emotion::CONTENT && (int32_t)(s.transient_until + s.follow_ms - now_ms) > 0) return s.follow;
  }
  return mood_base_emotion(s, hour);
}

// Zufallszustand für die Eigenarten (einmal beim Start, z. B. esp_random()).
inline void mood_seed(MoodState &s, uint32_t seed) { s.quirk_seed = seed ? seed : 1; }
// Eigenarten (Gähnen, Pfeifen, Schelmisch/Frech): jedes Bild aufrufen. Nur wenn er wach ist, kein anderes
// kurzzeitiges Gefühl läuft und das Grundgefühl SLEEPY, BORED oder HAPPY ist. Nächster Zeitpunkt wird
// beim Wechsel des Grundgefühls und nach jeder Eigenart neu gezogen.
inline void mood_quirks(MoodState &s, int hour, uint32_t now_ms) {
  if (mood_is_sleeping(s, now_ms)) return;
  const Emotion base = mood_base_emotion(s, hour);
  uint32_t lo = 0, hi = 0;
  switch (base) {
    case Emotion::SLEEPY: lo = 120000; hi = 300000; break;
    case Emotion::BORED:  lo = 60000;  hi = 180000; break;
    case Emotion::HAPPY:  lo = 120000; hi = 360000; break;
    default: break;
  }
  if (!s.quirk_base_known || base != s.quirk_base) {
    s.quirk_base_known = true;
    s.quirk_base = base;
    s.quirk_scheduled = hi != 0;
    if (hi) s.quirk_next_ms = now_ms + rng_range(s.quirk_seed, lo, hi);
  }
  if (!s.quirk_scheduled) return;
  if (mood_emotion(s, hour, now_ms) != base) return;   // anderes kurzzeitiges Gefühl läuft
  if ((int32_t)(now_ms - s.quirk_next_ms) < 0) return;
  switch (base) {
    case Emotion::SLEEPY: mood_set_transient(s, Emotion::YAWN, now_ms, 3000); break;
    case Emotion::BORED:  mood_set_transient(s, Emotion::WHISTLE, now_ms, 5000); break;
    default:
      mood_set_transient(s, (rng_next(s.quirk_seed) & 1) ? Emotion::MISCHIEVOUS : Emotion::CHEEKY, now_ms, 3000);
      break;
  }
  s.quirk_next_ms = now_ms + rng_range(s.quirk_seed, lo, hi);
}
// Zeit, in der das Gerät aus war, zählt wie Schlaf: nur Energie steigt.
inline void mood_apply_elapsed(MoodState &s, uint32_t seconds) {
  s.n.energy += MOOD_ENERGY_SLEEP_PER_MIN * (seconds / 60.0f);
  mood_clamp(s.n);
}
