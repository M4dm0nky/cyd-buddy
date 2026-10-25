#pragma once
#include <stdint.h>
#include "eye_model.h"

// Spiel „Fang mich“: kleines Auge springt herum, 20 s Runde, ab 8 Treffern gewonnen. Reine Logik (Host-Test test/test_games).
constexpr uint32_t CATCH_ROUND_MS = 20000;
constexpr uint16_t CATCH_WIN_HITS = 8;
constexpr uint8_t CATCH_R = 22;
constexpr int16_t CATCH_W = 320, CATCH_H = 200;
constexpr uint8_t CATCH_HIT_SLACK = 8;
struct CatchGame { int16_t x, y; uint16_t hits; uint32_t start_ms, next_jump_ms, seed; bool over; };

// Je Treffer 80 ms schneller, höchstens bis 450 ms.
inline uint32_t catch_jump_interval(uint16_t hits) {
  const int32_t v = 1200 - 80 * (int32_t)hits;
  return v < 450 ? 450 : (uint32_t)v;
}
inline void catch_jump(CatchGame &g, uint32_t now_ms) {
  g.x = (int16_t)rng_range(g.seed, CATCH_R, CATCH_W - CATCH_R);
  g.y = (int16_t)rng_range(g.seed, CATCH_R, CATCH_H - CATCH_R);
  g.next_jump_ms = now_ms + catch_jump_interval(g.hits);
}
inline void catch_start(CatchGame &g, uint32_t seed, uint32_t now_ms) {
  g.seed = seed ? seed : 1;
  g.hits = 0;
  g.start_ms = now_ms;
  g.over = false;
  catch_jump(g, now_ms);
}
inline void catch_tick(CatchGame &g, uint32_t now_ms) {
  if (g.over) return;
  if (now_ms - g.start_ms >= CATCH_ROUND_MS) { g.over = true; return; }
  if ((int32_t)(now_ms - g.next_jump_ms) >= 0) catch_jump(g, now_ms);
}
// Treffer: Abstand <= CATCH_R + 8; dann hits++ und sofort springen. Nach Rundenende zählt nichts.
inline bool catch_tap(CatchGame &g, int16_t x, int16_t y, uint32_t now_ms) {
  if (g.over || now_ms - g.start_ms >= CATCH_ROUND_MS) return false;
  const int32_t dx = x - g.x, dy = y - g.y, r = CATCH_R + CATCH_HIT_SLACK;
  if (dx * dx + dy * dy > r * r) return false;
  g.hits++;
  catch_jump(g, now_ms);
  return true;
}
inline bool catch_won(const CatchGame &g) { return g.over && g.hits >= CATCH_WIN_HITS; }
