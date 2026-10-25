#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Menü nach langem Drücken: 2x2 große, farbige Kacheln mit Symbolen; „Spielen“ öffnet eine zweite Ebene.
enum class MenuChoice : uint8_t { NONE, PLAY_CATCH, PLAY_SIMON, CLOCK, SETTINGS, CLOSE };
void menu_open(LGFX &lcd, uint32_t now_ms);
MenuChoice menu_touch(int16_t x, int16_t y, uint32_t now_ms);   // nur bei TAP aufrufen
bool menu_timed_out(uint32_t now_ms);                          // 10 s ohne Eingabe
