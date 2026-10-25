#pragma once
#include "../hw/display.h"

// Sprechblase oben mittig (y 2..32, Zipfel Richtung Gesicht), Text UTF-8 (Latin-1-Schrift, 1:1).
// Wird ins Augen-Sprite gezeichnet (eyes_draw), darum Ziel LovyanGFX.
void bubble_draw(LovyanGFX &d, const char *utf8);
