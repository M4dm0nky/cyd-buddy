#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Uhr für 15 s: große Uhrzeit, Datum, Wetter; die Augen schauen kurz von unten herein.
void clock_open(LGFX &lcd, uint32_t now_ms);
bool clock_update(LGFX &lcd, uint32_t now_ms);   // false = 15 s vorbei
