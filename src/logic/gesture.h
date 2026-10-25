#pragma once
#include <stdint.h>
#include <math.h>

// Touch-Abtastungen -> Gesten. Reine Logik, Zeit kommt als Parameter (Host-Test test/test_gesture).
enum class Gesture : uint8_t { NONE, TAP, STROKE, POKE, LONG_PRESS };
struct GestureState {
  bool down;
  int16_t x0, y0, lx, ly;     // Startpunkt, letzte Abtastung
  uint32_t t_down;
  float max_dist;             // größter Abstand zum Startpunkt
  float path;                 // Wegsumme im laufenden Streichel-Fenster
  uint32_t win_start;
  bool long_sent;
  bool moved;                 // einmal >= GESTURE_MOVE_PX vom Start entfernt
  uint32_t taps[5];           // Zeitstempel der letzten Tipps, ältester vorne
  uint8_t ntaps;
};

constexpr uint32_t GESTURE_TAP_MS = 300;
constexpr float GESTURE_MOVE_PX = 15;
constexpr uint32_t GESTURE_LONG_MS = 1200;
constexpr float GESTURE_STROKE_PX = 60;
constexpr uint32_t GESTURE_STROKE_WIN_MS = 1500;
constexpr uint8_t GESTURE_POKE_TAPS = 5;
constexpr uint32_t GESTURE_POKE_MS = 3000;

inline void gesture_init(GestureState &g) { g = GestureState{}; }

inline float gesture_dist(int16_t ax, int16_t ay, int16_t bx, int16_t by) {
  const float dx = (float)(bx - ax), dy = (float)(by - ay);
  return sqrtf(dx * dx + dy * dy);
}

// Tipp merken; true = 5. Tipp innerhalb von 3 s (dann Zähler leer).
inline bool gesture_add_tap(GestureState &g, uint32_t now_ms) {
  if (g.ntaps == GESTURE_POKE_TAPS) {
    for (uint8_t i = 1; i < GESTURE_POKE_TAPS; i++) g.taps[i - 1] = g.taps[i];
    g.ntaps--;
  }
  g.taps[g.ntaps++] = now_ms;
  if (g.ntaps == GESTURE_POKE_TAPS && now_ms - g.taps[0] <= GESTURE_POKE_MS) {
    g.ntaps = 0;
    return true;
  }
  return false;
}

// Etwa alle 20 ms aufrufen. Beim Loslassen (pressed == false) werden x/y ignoriert.
inline Gesture gesture_feed(GestureState &g, bool pressed, int16_t x, int16_t y, uint32_t now_ms) {
  if (pressed) {
    if (!g.down) {
      g.down = true;
      g.x0 = g.lx = x;
      g.y0 = g.ly = y;
      g.t_down = now_ms;
      g.max_dist = 0;
      g.path = 0;
      g.win_start = now_ms;
      g.long_sent = false;
      g.moved = false;
      return Gesture::NONE;
    }
    g.path += gesture_dist(g.lx, g.ly, x, y);
    g.lx = x;
    g.ly = y;
    const float d = gesture_dist(g.x0, g.y0, x, y);
    if (d > g.max_dist) g.max_dist = d;
    if (g.max_dist >= GESTURE_MOVE_PX) g.moved = true;

    if (now_ms - g.win_start <= GESTURE_STROKE_WIN_MS) {
      if (g.path >= GESTURE_STROKE_PX) {
        g.path = 0;
        g.win_start = now_ms;
        return Gesture::STROKE;
      }
    } else {   // Fenster abgelaufen ohne 60 px: neu anfangen
      g.path = 0;
      g.win_start = now_ms;
    }
    if (!g.long_sent && !g.moved && now_ms - g.t_down > GESTURE_LONG_MS) {
      g.long_sent = true;
      return Gesture::LONG_PRESS;
    }
    return Gesture::NONE;
  }

  if (!g.down) return Gesture::NONE;
  g.down = false;
  if (g.long_sent || g.moved) return Gesture::NONE;
  if (now_ms - g.t_down >= GESTURE_TAP_MS) return Gesture::NONE;
  return gesture_add_tap(g, now_ms) ? Gesture::POKE : Gesture::TAP;
}
