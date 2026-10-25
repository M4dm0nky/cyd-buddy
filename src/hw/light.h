#pragma once
#include <stdint.h>

// Lichtsensor (LDR): Mittelwert über ~1 s (eine Messung je Aufruf, Ringpuffer 16), Schwelle mit Hysterese.
void light_begin();
bool light_update(uint32_t now_ms);   // true = dunkel; misst höchstens alle 60 ms
int light_raw();                      // letzter Mittelwert (für Info/Seriell)
