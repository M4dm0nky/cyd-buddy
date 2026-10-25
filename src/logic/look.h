#pragma once
#include <stdint.h>
#include "clamp.h"
#include "deadline.h"

// Blick folgt dem Finger (F1): ein Touch setzt für 3 s ein Blickziel, das den Gazer ersetzt. Reine Logik (Host-Test test/test_look).
constexpr uint32_t LOOK_HOLD_MS = 3000;
constexpr float LOOK_X_MAX = 30, LOOK_Y_MAX = 15;   // wie Gazer (GAZE_X_MAX/GAZE_Y_MAX)

struct LookState { float gx, gy; Deadline hold; };

inline void look_at(LookState &l, int16_t x, int16_t y, uint32_t now_ms) {
  l.gx = clampf((x - 160) / 160.0f * LOOK_X_MAX, -LOOK_X_MAX, LOOK_X_MAX);
  l.gy = clampf((y - 100) / 100.0f * LOOK_Y_MAX, -LOOK_Y_MAX, LOOK_Y_MAX);
  l.hold.arm(now_ms, LOOK_HOLD_MS);
}
inline bool look_active(LookState &l, uint32_t now_ms) { return l.hold.active(now_ms); }   // schaltet sich beim Ablauf selbst aus (24,8-Tage-sicher)
inline void look_target(const LookState &l, float *gx, float *gy) {
  *gx = l.gx;
  *gy = l.gy;
}

// High-Five: Tipp in einer oberen Ecke des Augen-Bildschirms.
inline bool highfive_hit(int16_t x, int16_t y) { return (x < 64 || x > 256) && y < 56; }
