#pragma once
#include <stdint.h>
#include "../logic/eye_model.h"

// RGB-LED (aktiv low), PWM 5 kHz, 8 Bit über ledcAttach/ledcWrite (Arduino-Core 3.x).
void led_begin();
// Farbe skaliert auf max_pct %, für aktiv low invertiert; Rgb{0,0,0} = aus.
void led_set(Rgb c, uint8_t max_pct);
