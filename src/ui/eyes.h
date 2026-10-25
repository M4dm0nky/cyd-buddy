#pragma once
#include <stdint.h>
#include "../hw/display.h"
#include "../logic/eye_model.h"

// Augen + Overlays im Bereich 320x200 (oben). Mit Sprite (8 Bit, RGB332, 64 000 B) flimmerfrei,
// ohne Sprite (zu wenig Speicher oder während OTA freigegeben) direkt aufs Display (langsamer).
constexpr int EYES_W = 320;
constexpr int EYES_H = 200;

bool eyes_begin(LGFX &lcd);   // false = kein Speicher -> direkt zeichnen
void eyes_free();             // für OTA
bool eyes_ready();
void eyes_draw(LGFX &lcd, const EyeShape &shape, Rgb color, uint32_t overlays, uint32_t now_ms);
// Zeichnet ein Augenpaar ohne Overlays auf ein beliebiges Ziel (Spiele, OTA); (cx, cy) = Mitte zwischen den Augen.
void eyes_draw_pair(LovyanGFX &d, int cx, int cy, const EyeShape &shape, Rgb color, float scale);
