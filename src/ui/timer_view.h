#pragma once
#include <stdint.h>
#include "../hw/display.h"

// Timer-Bildschirm (F4): drei große Kacheln „15“, „25“, „45“ (mit Sanduhr) + Zurück; läuft schon ein Timer: Stopp + Zurück.
constexpr int TIMER_NONE = 0, TIMER_STOP = -1, TIMER_BACK = -2;
void timer_open(LGFX &lcd, bool running, uint32_t now_ms);
int timer_touch(int16_t x, int16_t y, uint32_t now_ms);   // 15/25/45 = Minuten, TIMER_STOP, TIMER_BACK, TIMER_NONE
bool timer_timed_out(uint32_t now_ms);                     // 10 s ohne Eingabe
