#pragma once
#include "../hw/display.h"

// Drei Schriftgrößen mit Latin-1-Zeichensatz (ä ö ü Ä Ö Ü ß °), Text wird 1:1 als UTF-8 gezeichnet.
// U8g2-Schriften (Helvetica Bold, BSD-Lizenz, olikraus/U8g2), eingebunden über lgfx::U8g2font.
extern const lgfx::U8g2font FONT_S;   // klein  (helvB10)
extern const lgfx::U8g2font FONT_M;   // mittel (helvB14)
extern const lgfx::U8g2font FONT_L;   // groß   (helvB24)
