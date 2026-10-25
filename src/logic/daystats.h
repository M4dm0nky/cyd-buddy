#pragma once
#include <stdint.h>
#include "mood.h"

// Tageszähler für den Rückblick (F2). Reine Logik (Host-Test test/test_speech).
struct DayStats { uint32_t day; uint16_t strokes, pokes, games, wins, meals, pomodoros; };

// Neuer Tag (YYYYMMDD) setzt alle Zähler zurück; true = zurückgesetzt. day 0 (unbekannt) ändert nichts.
inline bool daystats_roll(DayStats &d, uint32_t yyyymmdd) {
  if (yyyymmdd == 0 || d.day == yyyymmdd) return false;
  d = DayStats{};
  d.day = yyyymmdd;
  return true;
}
constexpr uint16_t DAYSTATS_MANY_STROKES = 10;
constexpr uint16_t DAYSTATS_MANY_POKES = 5;
// Gefühl im Rückblick: viel Stupsen (≥ 5 und mehr als Streicheln) → ANNOYED, viel Zuneigung (≥ 10 Streicheln) → HAPPY, sonst CONTENT.
inline Emotion daystats_mood(const DayStats &d) {
  if (d.pokes >= DAYSTATS_MANY_POKES && d.pokes > d.strokes) return Emotion::ANNOYED;
  if (d.strokes >= DAYSTATS_MANY_STROKES) return Emotion::HAPPY;
  return Emotion::CONTENT;
}
