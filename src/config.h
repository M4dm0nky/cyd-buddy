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
// Gemessen 2026-10-25: bei Raumlicht 0, nur in Dämmerung kurz ~200. Die Beschaltung des CYD ist bekannt schlecht
// (Teiler für 1 V ausgelegt, das Display leuchtet den Sensor an). Darum vorsichtig: "dunkel" erst über 400, "hell" unter 250.
// Geschlafen wird vor allem nach Uhrzeit + Ruhe (mood_night_check).
#define LDR_DARK_IS_HIGH 1                // höherer Rohwert = dunkler
constexpr int LDR_DARK_THRESHOLD = 325;
constexpr int LDR_HYSTERESIS = 75;

// RGB-LED (aktiv low)
constexpr int LED_R = 4;
constexpr int LED_G = 16;
constexpr int LED_B = 17;
