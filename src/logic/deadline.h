#pragma once
#include <stdint.h>

// Zeitablauf für lange Lebenszeiten (Blase, Hand, LED-Blitz ...). Host-Test test/test_deadline.
// Das Muster (int32_t)(until - now) > 0 wird nach 2^31 ms (24,8 Tage) wieder wahr, wenn nie neu gesetzt wurde.
// Darum: Start + Dauer + "an"-Flag; active() schaltet sich beim Ablauf selbst aus, und selbst ohne Abfrage
// bleibt es bis 2^32 ms (49,7 Tage) nach dem Start aus. Ein nie bewaffnetes Deadline ist nie aktiv.
struct Deadline {
  uint32_t start = 0;
  uint32_t dur = 0;
  bool on = false;
  void arm(uint32_t now, uint32_t duration) { start = now; dur = duration; on = true; }
  void clear() { on = false; }
  bool active(uint32_t now) {
    if (!on) return false;
    if (now - start < dur) return true;
    on = false;
    return false;
  }
};
