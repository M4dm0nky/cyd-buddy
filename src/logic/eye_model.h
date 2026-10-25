#pragma once
#include <stdint.h>
#include <math.h>
#include "mood.h"
#include "rng.h"

// Augenform je Gefühl, weiche Übergänge, Blinzeln, Blick, Zufall. Reine Logik (Host-Test test/test_eye_model).
struct EyeShape { float w, h, radius, open, lid_top_tilt, lid_top_cut, lid_bottom_cut, gaze_x, gaze_y; };
struct Rgb { uint8_t r, g, b; };
struct Expression { EyeShape shape; Rgb color; };

// lid_top_tilt > 0: äußere Ecke hängt (traurig), < 0: innere Ecke tiefer (böse). Der Renderer spiegelt links.
// Definition in face_model.h (aus der Gesichtstabelle abgeleitet); wird unten eingebunden.
inline Expression eye_expression(Emotion e);

constexpr float EYE_TWEEN_TAU_MS = 150.0f;

inline float eye_tween_k(uint32_t dt_ms) { return 1.0f - expf(-(float)dt_ms / EYE_TWEEN_TAU_MS); }
inline float eye_lerp(float a, float b, float k) { return a + (b - a) * k; }

// Exponentielle Annäherung an das Ziel, Zeitkonstante 150 ms; dt 0 ändert nichts.
inline EyeShape eye_tween(const EyeShape &cur, const EyeShape &target, uint32_t dt_ms) {
  const float k = eye_tween_k(dt_ms);
  return EyeShape{eye_lerp(cur.w, target.w, k),
                  eye_lerp(cur.h, target.h, k),
                  eye_lerp(cur.radius, target.radius, k),
                  eye_lerp(cur.open, target.open, k),
                  eye_lerp(cur.lid_top_tilt, target.lid_top_tilt, k),
                  eye_lerp(cur.lid_top_cut, target.lid_top_cut, k),
                  eye_lerp(cur.lid_bottom_cut, target.lid_bottom_cut, k),
                  eye_lerp(cur.gaze_x, target.gaze_x, k),
                  eye_lerp(cur.gaze_y, target.gaze_y, k)};
}

inline uint8_t eye_lerp_u8(uint8_t a, uint8_t b, float k) {
  const float v = eye_lerp((float)a, (float)b, k) + 0.5f;
  return (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
}
inline Rgb rgb_tween(Rgb cur, Rgb target, uint32_t dt_ms) {
  const float k = eye_tween_k(dt_ms);
  return Rgb{eye_lerp_u8(cur.r, target.r, k), eye_lerp_u8(cur.g, target.g, k), eye_lerp_u8(cur.b, target.b, k)};
}

constexpr uint32_t BLINK_CLOSE_MS = 80;
constexpr uint32_t BLINK_OPEN_MS = 120;
constexpr uint32_t BLINK_GAP_MIN_MS = 2000;
constexpr uint32_t BLINK_GAP_MAX_MS = 6000;

struct Blinker { uint32_t seed, next_ms, start_ms; bool closing; };
inline void blinker_init(Blinker &b, uint32_t seed, uint32_t now_ms) {
  b.seed = seed ? seed : 1;
  b.start_ms = now_ms;
  b.closing = false;
  b.next_ms = now_ms + rng_range(b.seed, BLINK_GAP_MIN_MS, BLINK_GAP_MAX_MS);
}
// 1 = offen … 0 = zu. Ein Blinzeln: 80 ms zu, 120 ms auf, dann nächstes in 2–6 s.
inline float blinker_open(Blinker &b, uint32_t now_ms) {
  const int32_t t = (int32_t)(now_ms - b.next_ms);
  if (t < 0) { b.closing = false; return 1.0f; }
  if (t < (int32_t)BLINK_CLOSE_MS) {
    b.closing = true;
    b.start_ms = b.next_ms;
    return 1.0f - (float)t / BLINK_CLOSE_MS;
  }
  if (t < (int32_t)(BLINK_CLOSE_MS + BLINK_OPEN_MS)) {
    b.closing = false;
    return (float)(t - (int32_t)BLINK_CLOSE_MS) / BLINK_OPEN_MS;
  }
  b.closing = false;
  b.next_ms = now_ms + rng_range(b.seed, BLINK_GAP_MIN_MS, BLINK_GAP_MAX_MS);
  return 1.0f;
}

constexpr uint32_t GAZE_GAP_MIN_MS = 1000;
constexpr uint32_t GAZE_GAP_MAX_MS = 4000;
constexpr int GAZE_X_MAX = 30;
constexpr int GAZE_Y_MAX = 15;

struct Gazer { uint32_t seed, next_ms; float gx, gy; };
inline void gazer_init(Gazer &z, uint32_t seed, uint32_t now_ms) {
  z.seed = seed ? seed : 1;
  z.gx = 0;
  z.gy = 0;
  z.next_ms = now_ms + rng_range(z.seed, GAZE_GAP_MIN_MS, GAZE_GAP_MAX_MS);
}
// Alle 1–4 s neues Blickziel, gx in [-30, 30], gy in [-15, 15].
inline void gazer_update(Gazer &z, uint32_t now_ms) {
  if ((int32_t)(now_ms - z.next_ms) < 0) return;
  z.gx = (float)rng_range(z.seed, 0, 2 * GAZE_X_MAX) - GAZE_X_MAX;
  z.gy = (float)rng_range(z.seed, 0, 2 * GAZE_Y_MAX) - GAZE_Y_MAX;
  z.next_ms = now_ms + rng_range(z.seed, GAZE_GAP_MIN_MS, GAZE_GAP_MAX_MS);
}

#include "face_model.h"
