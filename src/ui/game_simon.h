#pragma once
#include <stdint.h>
#include "game_common.h"

// „Simon“: vier farbige Felder; Folge merken und nachtippen, Länge 6 = gewonnen.
void game_simon_open(LGFX &lcd, uint32_t now_ms);
GameResult game_simon_update(LGFX &lcd, bool tap, int16_t x, int16_t y, uint32_t now_ms);
