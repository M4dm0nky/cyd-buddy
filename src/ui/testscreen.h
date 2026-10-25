#pragma once
#include "../hw/display.h"

// Bring-up-Testbild (Farbbalken, Touch-Punkte, LDR/Touch-Rohwerte seriell). Kehrt nie zurück.
// Erreichbar, wenn beim Start der Bildschirm gedrückt ist.
void testscreen_run(LGFX &tft);
