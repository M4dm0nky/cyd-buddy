#pragma once
#include "../hw/display.h"

// Sprechblase oben mittig (y 2..32, Zipfel Richtung Gesicht), Text UTF-8 (wird nach ASCII gewandelt).
// Wird ins Augen-Sprite gezeichnet (eyes_draw), darum Ziel LovyanGFX.
void bubble_draw(LovyanGFX &d, const char *utf8);
