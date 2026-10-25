#pragma once
#include <stdint.h>

// ESP32-2432S028 (CYD2USB) Pins
// Display (HSPI)
constexpr int TFT_SCLK = 14;
constexpr int TFT_MOSI = 13;
constexpr int TFT_MISO = 12;
constexpr int TFT_CS   = 15;
constexpr int TFT_DC   = 2;
constexpr int TFT_RST  = -1;
constexpr int TFT_BL   = 21;  // Backlight (PWM)

// Touch XPT2046 (VSPI)
constexpr int TOUCH_CLK  = 25;
constexpr int TOUCH_MOSI = 32;
constexpr int TOUCH_MISO = 39;
constexpr int TOUCH_CS   = 33;
constexpr int TOUCH_IRQ  = 36;

// Touch-Kalibrierung (Startwerte, am Gerät prüfen)
constexpr int TOUCH_RAW_X_MIN = 200;
constexpr int TOUCH_RAW_X_MAX = 3700;
constexpr int TOUCH_RAW_Y_MIN = 200;
constexpr int TOUCH_RAW_Y_MAX = 3700;

// Lichtsensor
constexpr int LDR_PIN = 34;
constexpr int LDR_DARK_THRESHOLD = 0;  // Platzhalter: am Gerät messen (Task 1)
constexpr int LDR_HYSTERESIS = 0;      // Platzhalter: am Gerät messen (Task 1)

// RGB-LED (aktiv low)
constexpr int LED_R = 4;
constexpr int LED_G = 16;
constexpr int LED_B = 17;
