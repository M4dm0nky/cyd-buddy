#pragma once
#include <stdint.h>

// xorshift32; ein Zustand 0 wird vorher auf 1 gesetzt.
inline uint32_t rng_next(uint32_t &s) {
  if (s == 0) s = 1;
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}
// Gleichverteilt (bis auf Modulo-Rest) in [lo, hi].
inline uint32_t rng_range(uint32_t &s, uint32_t lo, uint32_t hi) {
  if (hi <= lo) return lo;
  return lo + rng_next(s) % (hi - lo + 1);
}
