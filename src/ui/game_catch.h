#pragma once
#include <stdint.h>
#include "game_common.h"

// „Fang mich“: kleines Auge springt herum, 20 s; Restzeit als Balken oben, Treffer als Punkte.
void game_catch_open(LGFX &lcd, uint32_t now_ms);
GameResult game_catch_update(LGFX &lcd, bool tap, int16_t x, int16_t y, uint32_t now_ms);
