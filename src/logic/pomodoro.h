#pragma once
#include <stdint.h>

// Pomodoro-Timer (F4): Arbeit 15/25/45 min, danach 5 min Pause, dann IDLE (nicht automatisch neu). Host-Test test/test_pomodoro.
enum class PomoState : uint8_t { IDLE, WORK, BREAK };
enum class PomoEvent : uint8_t { NONE, BREAK_START, BREAK_END };
struct Pomo { PomoState state; uint32_t start_ms; uint32_t dur_ms; uint8_t minutes; };

constexpr uint32_t POMO_BREAK_MS = 5UL * 60000UL;

inline bool pomo_start(Pomo &p, uint8_t minutes, uint32_t now_ms) {
  if (minutes != 15 && minutes != 25 && minutes != 45) return false;
  p.state = PomoState::WORK;
  p.minutes = minutes;
  p.start_ms = now_ms;
  p.dur_ms = (uint32_t)minutes * 60000UL;
  return true;
}
inline void pomo_stop(Pomo &p) { p.state = PomoState::IDLE; }

// Jedes Bild aufrufen. Die nächste Phase beginnt am Ende der vorigen (nicht beim späten Abfragen).
inline PomoEvent pomo_tick(Pomo &p, uint32_t now_ms) {
  if (p.state == PomoState::IDLE || now_ms - p.start_ms < p.dur_ms) return PomoEvent::NONE;
  if (p.state == PomoState::WORK) {
    p.start_ms += p.dur_ms;
    p.dur_ms = POMO_BREAK_MS;
    p.state = PomoState::BREAK;
    return PomoEvent::BREAK_START;
  }
  p.state = PomoState::IDLE;
  return PomoEvent::BREAK_END;
}
// Anteil der laufenden Phase 0..1 (IDLE: 0).
inline float pomo_progress(const Pomo &p, uint32_t now_ms) {
  if (p.state == PomoState::IDLE || p.dur_ms == 0) return 0;
  const uint32_t el = now_ms - p.start_ms;
  return el >= p.dur_ms ? 1.0f : (float)el / p.dur_ms;
}
// Restzeit der laufenden Phase in Sekunden, aufgerundet (IDLE: 0).
inline uint32_t pomo_remaining_s(const Pomo &p, uint32_t now_ms) {
  if (p.state == PomoState::IDLE) return 0;
  const uint32_t el = now_ms - p.start_ms;
  if (el >= p.dur_ms) return 0;
  return (p.dur_ms - el + 999) / 1000;
}
