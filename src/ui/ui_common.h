#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Gemeinsame Hilfen für die Bildschirme (Menü, Uhr, Einstellungen, Spiele, OTA).
struct UiRect { int16_t x, y, w, h; };
inline bool ui_hit(const UiRect &r, int16_t x, int16_t y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }
// Text (UTF-8, wird nach ASCII umgeschrieben) mit Datum (z. B. middle_left).
void ui_text(LovyanGFX &d, const char *utf8, int x, int y, const lgfx::IFont *font, uint32_t col, textdatum_t datum);
// Gefüllte Kachel mit abgerundeten Ecken und hellerem Rand.
void ui_tile(LovyanGFX &d, const UiRect &r, uint32_t fill);
