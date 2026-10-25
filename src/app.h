#pragma once
#include "hw/display.h"

// Zustandsmaschine der Bildschirme; verbindet Logik, Hardware, UI und Netz.
LGFX &app_lcd();   // das eine Display-Objekt (main.cpp ruft init() vor app_setup())
void app_setup();
void app_loop();
