#pragma once
#include <stdint.h>
#include "../hw/display.h"
#include "../logic/daystats.h"

// Tagesrückblick (F2): „Dein Tag mit mir“, Zeilen mit Symbol + Zahl, kleines Augenpaar mit Gefühl. 12 s, Tippen schließt.
void review_open(LGFX &lcd, const DayStats &d, uint32_t now_ms);
bool review_update(LGFX &lcd, uint32_t now_ms);   // false = 12 s vorbei
